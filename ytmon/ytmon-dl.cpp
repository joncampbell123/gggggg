
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include <json11.hpp>

#include <string>

using namespace std;
using namespace json11;

double                      duration_limit = -1;

time_t                      download_timeout_default = 5 * 60; // 5 min
time_t                      failignore_timeout = 7 * 24 * 60 * 60; // 7 days
time_t                      info_json_expire = 6 * 60 * 60;         // 6 hours

// 2020-06-29: Too much to collect, too little time. Limit to 720p to improve collection rate.
//             Back when YouTube wasn't banning everything in sight, patience and the highest quality
//             version were ideal, but no longer. Collect everything, before it is banned!
std::string                 youtube_format_spec = "-f 'bestvideo[height<=720]+bestaudio/best[height<=720]' ";

bool                        sunday_dl = false;
int                         youtube_bitrate = 2200;
int                         bitchute_bitrate = 2000;

int                         failignore_mark_counter = 0;

std::string                 api_url;
std::string                 youtube_user,youtube_pass;

char                        large_tmp[1024*1024];

bool should_stop = false;

void init_marker(void) {
    mkdir("marker",0755);
}

string get_mark_failignore_filename(const string &filename) {
    return string("marker/") + filename + ".fail-ignore";
}

void mark_failignore_file(const string &filename) {
    const string mark_filename = get_mark_failignore_filename(filename);
    int fd = open(mark_filename.c_str(),O_CREAT|O_TRUNC,0644); /* create file, truncate if exists, which updates mtime */
    if (fd >= 0) close(fd);
}

string get_mark_filename(const string &filename) {
    return string("marker/") + filename;
}

void mark_file(const string &filename) {
    const string mark_filename = get_mark_filename(filename);
    int fd = open(mark_filename.c_str(),O_CREAT|O_EXCL,0644);
    if (fd >= 0) close(fd);
}

bool download_video_youtube(const Json &video) {
    string title = video["title"].string_value();
    string url = video["url"].string_value();
    string id = video["id"].string_value();

    if (id.empty() || url.empty()) return false;
    assert(id.find_first_of(' ') == string::npos);
    assert(id.find_first_of('$') == string::npos);
    assert(id.find_first_of('\'') == string::npos);
    assert(id.find_first_of('\"') == string::npos);

    {
        struct stat st;

        string mark_filename = get_mark_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            return false; // already exists
        }
    }

    {
        time_t now = time(NULL);
        struct stat st;

        string mark_filename = get_mark_failignore_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            if ((st.st_mtime + failignore_timeout) >= now) {
                fprintf(stderr,"Ignoring '%s', failignore\n",id.c_str());
                return false; // failignore
            }
        }
    }

    sleep(1);

    string tagname = id;
    if (!title.empty()) {
        tagname += "-";
        tagname += title;
    }
    if (tagname != id) {
        if (tagname.length() > 128)
            tagname = tagname.substr(0,128);

        for (auto &c : tagname) {
            if (c < 32 || c > 126 || c == ':' || c == '\\' || c == '/')
                c = ' ';
        }

        tagname += ".txt";

        {
            FILE *fp = fopen(tagname.c_str(),"w");
            if (fp) {
                fclose(fp);
            }
        }
    }

    /* we trust the ID will never need characters that require escaping.
     * they're alphanumeric base64 so far. */
    string invoke_url = string("https://www.youtube.com/watch?v=") + id;

    string creds;

    if (!youtube_user.empty() && !youtube_pass.empty())
        creds += string("--username '") + youtube_user + "' --password '" + youtube_pass + "' ";

    time_t dl_begin = time(NULL);

    string expect_info_json = id + ".info.json";

    /* download the *.info.json first, don't update too often */
    {
        bool doit = true;
        time_t now = time(NULL);
        struct stat st;

        if (stat(expect_info_json.c_str(),&st) == 0 && S_ISREG(st.st_mode)) {
            if ((st.st_mtime + info_json_expire) >= now) {
                doit = false;
            }
        }

        if (doit) {
            string cmd = string("youtube-dl --cookies cookies.txt --skip-download --write-info-json --output '%(id)s' ") + creds + invoke_url;
            int status = system(cmd.c_str());
            if (WIFSIGNALED(status)) should_stop = true;

            if (status != 0) {
                time_t dl_duration = time(NULL) - dl_begin;

                if (dl_duration < 10) {
                    fprintf(stderr,"Failed too quickly, marking\n");
                    mark_failignore_file(id);
                    failignore_mark_counter++;
                }

                return false;
            }
        }
    }

    /* read the info JSON.
     * Do not download live feeds, because they are in progress.
     * youtube-dl is pretty good about not listing live feeds if asked to follow a channel,
     * but for search queries will return results that involve live feeds.
     * Most live feeds finish eventually and turn into a video. */
    bool live_feed = false;
    double duration = -1;
    {
        FILE *fp;

        fp = fopen(expect_info_json.c_str(),"r");
        if (fp != NULL) {
            size_t r = fread(large_tmp,1,sizeof(large_tmp)-1,fp);
            assert(r < sizeof(large_tmp));
            large_tmp[r] = 0;
            fclose(fp);

            string json_err;
            Json info_json = Json::parse(large_tmp,json_err);

            if (info_json == Json()) {
                fprintf(stderr,"INFO JSON parse error: %s\n",json_err.c_str());
            }
            else {
                auto is_live = info_json["is_live"];
                if (is_live.is_bool()) /* else is null? */
                    live_feed = is_live.bool_value();

                auto js_duration = info_json["duration"];
                if (js_duration.is_number())
                    duration = js_duration.number_value();
            }
        }
    }

    if (live_feed) {
        fprintf(stderr,"Item '%s' is a live feed, skipping. It may turn into a downloadable later.\n",id.c_str());
        return false;
    }
    if (duration_limit >= 0 && duration >= 0 && duration > duration_limit) {
//      fprintf(stderr,"Item '%s' duration %.3f exceeds duration limit %.3f\n",id.c_str(),duration,duration_limit);
        return false;
    }

    /* then download the video */
    {
        string cmd = string("youtube-dl --abort-on-unavailable-fragment --cookies cookies.txt --no-mtime --continue --write-all-thumbnails --all-subs --limit-rate=") + to_string(youtube_bitrate) + "K --output '%(id)s' " + youtube_format_spec + creds + invoke_url; /* --write-info-json not needed, first step above */
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) should_stop = true;

        if (status != 0) {
            time_t dl_duration = time(NULL) - dl_begin;

            if (dl_duration < 30) {
                fprintf(stderr,"Failed too quickly, marking\n");
                mark_failignore_file(id);
                failignore_mark_counter++;
            }

            return false;
        }
    }

    /* done! */
    mark_file(id);
    return true;
}

bool download_video_bitchute(const Json &video) {
    string title = video["title"].string_value();
    string url = video["url"].string_value();
    string id = video["id"].string_value();

    if (id.empty() || url.empty()) return false;
    assert(id.find_first_of(' ') == string::npos);
    assert(id.find_first_of('$') == string::npos);
    assert(id.find_first_of('\'') == string::npos);
    assert(id.find_first_of('\"') == string::npos);

    {
        struct stat st;

        string mark_filename = get_mark_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            return false; // already exists
        }
    }

    {
        time_t now = time(NULL);
        struct stat st;

        string mark_filename = get_mark_failignore_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            if ((st.st_mtime + failignore_timeout) >= now) {
                fprintf(stderr,"Ignoring '%s', failignore\n",id.c_str());
                return false; // failignore
            }
        }
    }

    sleep(1);

    string tagname = id;
    if (!title.empty()) {
        tagname += "-";
        tagname += title;
    }
    if (tagname != id) {
        if (tagname.length() > 128)
            tagname = tagname.substr(0,128);

        for (auto &c : tagname) {
            if (c < 32 || c > 126 || c == ':' || c == '\\' || c == '/')
                c = ' ';
        }

        tagname += ".txt";

        {
            FILE *fp = fopen(tagname.c_str(),"w");
            if (fp) {
                fclose(fp);
            }
        }
    }

    time_t dl_begin = time(NULL);

    /* we trust the ID will never need characters that require escaping.
     * they're alphanumeric base64 so far. */
    string invoke_url = string("https://www.bitchute.com/video/") + id;

    string expect_info_json = id + ".info.json";

    /* download the *.info.json first, don't update too often */
    {
        bool doit = true;
        time_t now = time(NULL);
        struct stat st;

        if (stat(expect_info_json.c_str(),&st) == 0 && S_ISREG(st.st_mode)) {
            if ((st.st_mtime + info_json_expire) >= now) {
                doit = false;
            }
        }

        if (doit) {
            string cmd = string("youtube-dl --no-check-certificate --cookies cookies.txt --skip-download --write-info-json --output '%(id)s' ") + invoke_url;
            int status = system(cmd.c_str());
            if (WIFSIGNALED(status)) should_stop = true;

            if (status != 0) {
                time_t dl_duration = time(NULL) - dl_begin;

                if (dl_duration < 10) {
                    fprintf(stderr,"Failed too quickly, marking\n");
                    mark_failignore_file(id);
                    failignore_mark_counter++;
                }

                return false;
            }
        }
    }

    /* All video on BitChute is .mp4, and youtube-dl needs to be given that suffix */
    {
        string cmd = string("youtube-dl --abort-on-unavailable-fragment --no-check-certificate --no-mtime --continue --write-all-thumbnails --write-info-json --all-subs --limit-rate=") + to_string(bitchute_bitrate) + "K --output '%(id)s.mp4' " + invoke_url;
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) should_stop = true;

        if (status != 0) {
            time_t dl_duration = time(NULL) - dl_begin;

            if (dl_duration < 900) { // duration is longer to trigger on youtube-dl's "No Video Formats Found" error when it can't find anything
                fprintf(stderr,"Failed too quickly, marking\n");
                mark_failignore_file(id);
                failignore_mark_counter++;
            }

            return false;
        }
    }

    /* done! */
    mark_file(id);
    return true;
}

bool download_video_soundcloud(const Json &video) {
    string title = video["title"].string_value();
    string url = video["url"].string_value();
    string id = video["id"].string_value();

    if (id.empty() || url.empty()) return false;
    assert(id.find_first_of(' ') == string::npos);
    assert(id.find_first_of('$') == string::npos);
    assert(id.find_first_of('\'') == string::npos);
    assert(id.find_first_of('\"') == string::npos);

    {
        struct stat st;

        string mark_filename = get_mark_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            return false; // already exists
        }
    }

    {
        time_t now = time(NULL);
        struct stat st;

        string mark_filename = get_mark_failignore_filename(id);
        if (stat(mark_filename.c_str(),&st) == 0) {
            if ((st.st_mtime + failignore_timeout) >= now) {
                fprintf(stderr,"Ignoring '%s', failignore\n",id.c_str());
                return false; // failignore
            }
        }
    }

    sleep(1);

    string tagname = id;
    if (!title.empty()) {
        tagname += "-";
        tagname += title;
    }
    if (tagname != id) {
        if (tagname.length() > 128)
            tagname = tagname.substr(0,128);

        for (auto &c : tagname) {
            if (c < 32 || c > 126 || c == ':' || c == '\\' || c == '/')
                c = ' ';
        }

        tagname += ".txt";

        {
            FILE *fp = fopen(tagname.c_str(),"w");
            if (fp) {
                fclose(fp);
            }
        }
    }

    time_t dl_begin = time(NULL);

    /* we trust the ID will never need characters that require escaping.
     * they're alphanumeric base64 so far. */
    string invoke_url = url;

    string expect_info_json = id + ".info.json";

    /* download the *.info.json first, don't update too often */
    {
        bool doit = true;
        time_t now = time(NULL);
        struct stat st;

        if (stat(expect_info_json.c_str(),&st) == 0 && S_ISREG(st.st_mode)) {
            if ((st.st_mtime + info_json_expire) >= now) {
                doit = false;
            }
        }

        if (doit) {
            string cmd = string("youtube-dl --cookies cookies.txt --skip-download --write-info-json --output '%(id)s' ") + invoke_url;
            int status = system(cmd.c_str());
            if (WIFSIGNALED(status)) should_stop = true;

            if (status != 0) {
                time_t dl_duration = time(NULL) - dl_begin;

                if (dl_duration < 10) {
                    fprintf(stderr,"Failed too quickly, marking\n");
                    mark_failignore_file(id);
                    failignore_mark_counter++;
                }

                return false;
            }
        }
    }

    {
        string cmd = string("youtube-dl --no-check-certificate --no-mtime --continue --write-all-thumbnails --write-info-json --all-subs --limit-rate=") + to_string(bitchute_bitrate) + "K --output '%(id)s.mp3' " + invoke_url;
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) should_stop = true;

        if (status != 0) {
            time_t dl_duration = time(NULL) - dl_begin;

            if (dl_duration < 10) {
                fprintf(stderr,"Failed too quickly, marking\n");
                mark_failignore_file(id);
                failignore_mark_counter++;
            }

            return false;
        }
    }

    /* done! */
    mark_file(id);
    return true;
}

void chomp(char *s) {
    char *e = s + strlen(s) - 1;
    while (e >= s && (*e == '\n' || *e == '\r')) *e-- = 0;
}

int parse_argv(int argc,char **argv) {
    int i = 1,nsw = 0;
    char *a;

    while (i < argc) {
        a = argv[i++];

        if (*a == '-') {
            do { a++; } while (*a == '-');

            if (!strcmp(a,"duration-limit")) {
                a = argv[i++];
                if (a == NULL) return 1;
                duration_limit = strtof(a,&a);
                if (*a == 'h')
                    duration_limit *= 60 * 60;
                else if (*a == 'm')
                    duration_limit *= 60;
                else if (*a == 's' || *a == 0)
                    { /*nothing*/ }
                else {
                    fprintf(stderr,"What? %c\n",a);
                    return 1;
                }
            }
            else {
                return 1;
            }
        }
        else {
            switch (nsw) {
                case 0:
                    nsw++;
                    api_url = a;
                    break;
                default:
                    return 1;
            }
        }
    }

    return 0;
}

int main(int argc,char **argv) {
    time_t download_timeout;
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    int download_count = 0;
    int download_limit = 20;
    int failignore_limit = 10;

    if (parse_argv(argc,argv))
        return 1;

    init_marker();

    // youtube creds
    {
        FILE *fp = fopen("youtube-creds.lst","r");
        if (fp == NULL) fp = fopen("../youtube-creds.lst","r");
        if (fp != NULL) {
            char tmp[512];

            fgets(tmp,sizeof(tmp),fp);
            chomp(tmp);
            youtube_user = tmp;

            fgets(tmp,sizeof(tmp),fp);
            chomp(tmp);
            youtube_pass = tmp;

            fclose(fp);
        }

        if (!youtube_user.empty() && !youtube_pass.empty())
            fprintf(stderr,"Using YouTube credentials for '%s'\n",youtube_user.c_str());
    }

    // backwards compat
    if (!api_url.empty()) {
        assert(api_url.find_first_of(' ') == string::npos);
        assert(api_url.find_first_of('$') == string::npos);
        assert(api_url.find_first_of('\'') == string::npos);
        assert(api_url.find_first_of('\"') == string::npos);
        assert(api_url.find_first_of('\\') == string::npos);

        string cmd = string("ytmon-update-pl ") + api_url;
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) return 1;
    }

    // new rule: we no longer download the playlist ourself.
    //           you must run a playlist downloading tool to generate the list this
    //           tool downloads by. The playlist downloading tool can combine the
    //           URLs from the first N videos as well as gradually query the playlist
    //           piecemeal to gather the entire channel list without hitting YouTube's
    //           "Too many requests" condition.
    string js_file = "playlist-combined.json"; // use .json not .js so the archive-off script does not touch it

    /* the JS file is actually MANY JS objects encoded on a line by line basis */

    /* to keep collecting on general going, have a download timeout as well. */
    now = time(NULL);
    download_timeout = now + download_timeout_default;

    {
        char buf[4096]; /* should be large enough for now */
        FILE *fp = fopen(js_file.c_str(),"r");
        if (fp == NULL) {
            fprintf(stderr,"Unable to open %s. Please run playlist downloader tool to generate/update it.\n",js_file.c_str());
            return 1;
        }

        memset(buf,0,sizeof(buf));
        while (!feof(fp) && !ferror(fp)) {
            /* Linux: fgets() will terminate the buffer with \0 (NULL) */
            if (fgets(buf,sizeof(buf),fp) == NULL) break;

            /* eat the newline */
            {
                char *s = buf + strlen(buf) - 1;
                while (s > buf && (*s == '\n' || *s == '\r')) *s-- = 0;
            }

            if (buf[0] == 0) continue;
            if (buf[0] == '#') continue; // download aggregator will use # lines for comments and status

            string json_err;
            Json json = Json::parse(buf,json_err);

            if (json == Json()) {
                fprintf(stderr,"JSON parse error: %s\n",json_err.c_str());
                continue;
            }

            if (should_stop)
                break;

            /* YouTube example:
             *
             * {"url": "AbH3pJnFgY8", "_type": "url", "ie_key": "Youtube", "id": "AbH3pJnFgY8", "title": "No More Twitter? \ud83d\ude02"} */
            if (json["ie_key"].string_value() == "Youtube") { /* FIXME: what if youtube-dl changes that? */
                /* download like a human */
                {
                    time_t now = time(NULL);
                    struct tm tm = *localtime(&now);

                    // look human by stopping downloads between 3AM and 6AM
                    if (tm.tm_hour >= 3 && tm.tm_hour < 6)
                        continue;
                }

                if (download_video_youtube(json)) {
                    if (++download_count >= download_limit)
                        break;

                    sleep(5 + ((unsigned int)rand() % 5u));
                }
            }
            /* Bitchute example:
             *
             * {"url": "https://www.bitchute.com/video/F0yUrwVz9fYV", "_type": "url", "ie_key": "BitChute", "id": "F0yUrwVz9fYV"} */
            else if (json["ie_key"].string_value() == "BitChute") { /* FIXME: what if youtube-dl changes that? */
                if (download_video_bitchute(json)) {
                    if (++download_count >= download_limit)
                        break;

                    sleep(5 + ((unsigned int)rand() % 5u));
                }
            }
            /* Soundcloud example:
             *
             * {"url": "https://soundcloud.com/openai_audio/classic-pop-in-the-style-of-elvis-presley", "_type": "url", "ie_key": "Soundcloud", "id": "814263598", "title": "Classic Pop, in the style of Elvis Presley"} */
            else if (json["ie_key"].string_value() == "Soundcloud") { /* FIXME: what if youtube-dl changes that? */
                if (download_video_soundcloud(json)) {
                    if (++download_count >= download_limit)
                        break;

                    sleep(5 + ((unsigned int)rand() % 5u));
                }
            }

            if (failignore_mark_counter >= failignore_limit)
                break;

            if (should_stop)
                break;

            now = time(NULL);
            if (now >= download_timeout)
                break;
        }

        fclose(fp);
    }

    return 0;
}

