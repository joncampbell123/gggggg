
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

bool should_stop = false;

void init_marker(void) {
    mkdir("marker",0755);
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

    {
        string cmd = string("youtube-dl --no-mtime --continue --all-subs --limit-rate=3000K --output '%(id)s' ") + invoke_url;
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) should_stop = true;
        if (status != 0) return false;
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
    string invoke_url = string("https://www.bitchute.com/video/") + id;

    /* All video on BitChute is .mp4, and youtube-dl needs to be given that suffix */
    {
        string cmd = string("youtube-dl --no-mtime --continue --all-subs --limit-rate=3000K --output '%(id)s.mp4' ") + invoke_url;
        int status = system(cmd.c_str());
        if (WIFSIGNALED(status)) should_stop = true;
        if (status != 0) return false;
    }

    /* done! */
    mark_file(id);
    return true;
}

int main(int argc,char **argv) {
    string api_url;
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    char timestr[128];
    int download_count = 0;
    int download_limit = 4;

    if (argc < 2) {
        fprintf(stderr,"Need channel URL\n");
        return 1;
    }
    api_url = argv[1];

    init_marker();

    // look human by stopping downloads between 11AM and 3PM
    if (tm.tm_hour >= 11 && tm.tm_hour < 15) {
        if (tm.tm_wday >= 1/*monday*/ && tm.tm_wday <= 5/*friday*/) {
            fprintf(stderr,"Time for work.\n");
            return 1;
        }
    }
    // look human by stopping downloads between 1AM and 5AM
    if (tm.tm_hour >= 1 && tm.tm_hour < 5) {
        fprintf(stderr,"Time for bed.\n");
        return 1;
    }

    // once every 24 hours
    sprintf(timestr,"%04u%02u%02u-%02u%02u%02u",
        tm.tm_year+1900,
        tm.tm_mon+1,
        tm.tm_mday,
        0,
        0,
        0);

    string js_file = string("playlist-") + timestr + ".js";//WARNING: No spaces allowed!

    {
        struct stat st;

        if (stat(js_file.c_str(),&st) != 0) {
            assert(js_file.find_first_of(' ') == string::npos);
            assert(js_file.find_first_of('$') == string::npos);
            assert(js_file.find_first_of('\'') == string::npos);
            assert(js_file.find_first_of('\"') == string::npos);

            assert(api_url.find_first_of(' ') == string::npos);
            assert(api_url.find_first_of('$') == string::npos);
            assert(api_url.find_first_of('\'') == string::npos);
            assert(api_url.find_first_of('\"') == string::npos);

            fprintf(stderr,"Downloading playlist...\n");

            /* -j only emits to stdout, sorry. */
            string cmd = string("youtube-dl --no-mtime -j --flat-playlist ") + " \"" + api_url + "\" >" + js_file;
            int status = system(cmd.c_str());
            if (status != 0) return 1;
        }
    }

    /* the JS file is actually MANY JS objects encoded on a line by line basis */

    {
        char buf[4096]; /* should be large enough for now */
        FILE *fp = fopen(js_file.c_str(),"r");
        if (fp == NULL) return 1;

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
                if (download_video_youtube(json)) {
                    if (++download_count >= download_limit)
                        break;
                }
            }
            /* Bitchute example:
             *
             * {"url": "https://www.bitchute.com/video/F0yUrwVz9fYV", "_type": "url", "ie_key": "BitChute", "id": "F0yUrwVz9fYV"} */
            else if (json["ie_key"].string_value() == "BitChute") { /* FIXME: what if youtube-dl changes that? */
                if (download_video_bitchute(json)) {
                    if (++download_count >= download_limit)
                        break;
                }
            }

            if (should_stop)
                break;
        }

        fclose(fp);
    }

    return 0;
}

