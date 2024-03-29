
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>

#include <json11.hpp>

#include <string>

using namespace std;
using namespace json11;

//const string api_url = "https://api.infowarsmedia.com/api/channel/5b885d33e6646a0015a6fa2d/videos?limit=99&offset=0";
const string api_url_pre = "https://api.infowarsmedia.com/api/channel/";
const string api_url_suf = "/videos?limit=99&offset=0";

const string default_channel = "5b885d33e6646a0015a6fa2d"; /* The Alex Jones Show */

string api_url;

string chosen_channel = default_channel;

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
        string cmd = string("wget --timeout=60 --continue --no-use-server-timestamps --show-progress --limit-rate=500K -O ") + filename + ".part " + direct_url;
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

int parse_argv(int argc,char **argv) {
    int i = 1;
    char *a;

    while (i < argc) {
        a = argv[i++];

        if (*a == '-') {
            do { a++; } while (*a == '-');

            if (!strcmp(a,"ch")) {
                a = argv[i++];
                if (a == NULL) return 1;
                chosen_channel = a;
            }
            else {
                return 1;
            }
        }
        else {
            return 1;
        }
    }

    return 0;
}

int main(int argc,char **argv) {
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    char timestr[128];
    int download_count = 0;
    int download_limit = 1;

    if (parse_argv(argc,argv))
        return 1;

    api_url = api_url_pre + chosen_channel + api_url_suf;
    fprintf(stderr,"Using API url: %s\n",api_url.c_str());

    init_marker();

    // Round to half an hour for JS name to avoid hitting their API too often. Be nice.
    sprintf(timestr,"%04u%02u%02u-%02u%02u%02u",
        tm.tm_year+1900,
        tm.tm_mon+1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min - (tm.tm_min % 30),
        0);

    string js_file = string("iw-api-") + timestr + ".js";//WARNING: No spaces allowed!

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

            string cmd = string("wget --timeout=60 --show-progress --no-use-server-timestamps --limit-rate=750K -O ") + js_file + " \"" + api_url + "\"";
            int status = system(cmd.c_str());
            if (status != 0) return 1;
        }
    }

    Json json;
    {
        int fd = open(js_file.c_str(),O_RDONLY);
        if (fd < 0) return 1;

        off_t len = lseek(fd,0,SEEK_END);
        if (len > (16*1024*1024)) return 1;
        if (lseek(fd,0,SEEK_SET) != 0) return 1;

        char *buf = new char[len+1]; // or throw exception
        if (read(fd,buf,len) != len) return 1;
        buf[len] = 0;

        string json_err;

        json = Json::parse(buf,json_err);

        if (json == Json()) {
            fprintf(stderr,"JSON parse error: %s\n",json_err.c_str());
            return 1;
        }

        delete[] buf;
        close(fd);
    }

    if (json["id"].string_value() != chosen_channel)
        fprintf(stderr,"WARNING: JSON is listing the wrong channel id\n");
    if (json["title"].string_value().empty())
        fprintf(stderr,"WARNING: JSON is missing some fields\n");

    /* look at the videos array */
    {
        auto videos = json["videos"];
        if (videos.is_array()) {
            for (auto &video : videos.array_items()) {
                if (download_video(video)) {
                    if (++download_count >= download_limit)
                        break;
                }

                if (should_stop)
                    break;
            }
        }
        else {
            fprintf(stderr,"No videos array\n");
        }
    }

    return 0;
}

