
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

    /* we trust the ID will never need characters that require escaping.
     * they're alphanumeric base64 so far. */
    string invoke_url = string("https://www.youtube.com/watch?v=") + id;

    {
        string cmd = string("youtube-dl --continue --all-subs --limit-rate=1000K --output '%(id)s' ") + invoke_url;
        int status = system(cmd.c_str());
        if (status != 0) {
            if (WIFSIGNALED(status)) should_stop = true;
            return false;
        }
    }

    return false;
}

bool download_video(const Json &video) {
    if (!video.is_object()) {
        fprintf(stderr,"WARNING: Videos array element not object\n");
        return false;
    }

    const string title = video["title"].string_value();
    const string summary = video["summary"].string_value();
    const string direct_url = video["directUrl"].string_value();

    if (direct_url.empty()) {
        fprintf(stderr,"WARNING: No direct URL\n");
        return false;
    }

    string filename;
    {
        auto p = direct_url.find_last_of('/');
        if (p == string::npos) {
            fprintf(stderr,"Cannot determine filename\n");
            return false;
        }

        filename = direct_url.substr(p+1);
        if (filename.empty()) {
            fprintf(stderr,"Cannot determine filename\n");
            return false;
        }

        /* it's unusual for the filename to contain anything other than hexadecimal, dashes, and ".mp4" */
        for (const auto &c : filename) {
            if (!(isalpha(c) || isdigit(c) || c == '.' || c == '-')) {
                fprintf(stderr,"Unusual filename '%s'\n",filename.c_str());
                return false;
            }
        }
    }

    assert(filename.find_first_of('/') == string::npos);
    assert(filename.find_first_of('$') == string::npos);
    assert(filename.find_first_of('\\') == string::npos);
    assert(filename.find_first_of('\'') == string::npos);
    assert(filename.find_first_of('\"') == string::npos);

    assert(direct_url.find_first_of('$') == string::npos);
    assert(direct_url.find_first_of('\\') == string::npos);
    assert(direct_url.find_first_of('\'') == string::npos);
    assert(direct_url.find_first_of('\"') == string::npos);

    {
        struct stat st;

        if (stat(filename.c_str(),&st) == 0) {
            mark_file(filename);
            return false; // already exists
        }

        string mark_filename = get_mark_filename(filename);

        if (stat(mark_filename.c_str(),&st) == 0) {
            return false; // already exists
        }
    }

    string tagname = filename;
    if (!title.empty()) {
        tagname += "-";
        tagname += title;
        if (!summary.empty()) {
            tagname += "-";
            tagname += summary;
        }
    }
    if (tagname != filename) {
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

    {
        string cmd = string("wget --timeout=60 --continue --show-progress --limit-rate=250K -O ") + filename + ".part " + direct_url;
        int status = system(cmd.c_str());
        if (status != 0) {
            if (WIFSIGNALED(status)) should_stop = true;
            return false;
        }
    }

    {
        string alt = filename + ".part";
        rename(alt.c_str(),filename.c_str());
    }

    mark_file(filename);
    return true;
}

int main(int argc,char **argv) {
    string api_url;
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    char timestr[128];
    int download_count = 0;
    int download_limit = 1;

    if (argc < 2) {
        fprintf(stderr,"Need channel URL\n");
        return 1;
    }
    api_url = argv[1];

    init_marker();

    // Round to half an hour for JS name to avoid hitting their API too often. Be nice.
    sprintf(timestr,"%04u%02u%02u-%02u%02u%02u",
        tm.tm_year+1900,
        tm.tm_mon+1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min - (tm.tm_min % 30),
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

            /* -j only emits to stdout, sorry.
             * limit the playlist to only monitor RECENT videos.
             * 1-100 ought to keep up fine. */
            string cmd = string("youtube-dl -j --flat-playlist --playlist-items 1-100 ") + " \"" + api_url + "\" >" + js_file;
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

            if (should_stop)
                break;
        }

        fclose(fp);
    }

    return 0;
}

