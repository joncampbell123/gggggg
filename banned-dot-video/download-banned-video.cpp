
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

const string default_channel = "5b885d33e6646a0015a6fa2d"; /* The Alex Jones Show */

string chosen_channel = default_channel;

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
    struct stat st;

    if (parse_argv(argc,argv))
        return 1;

    /* construct the query JSON for the POST request */
    string channel_query_string;
    {
        Json::object channel_query;
        channel_query["operationName"] = "GetChannelVideos";
        {
            Json::object obj;
            obj["id"] = chosen_channel;
            obj["limit"] = 99;
            obj["offset"] = 0;
            channel_query["variables"] = obj;
        }
        // FIXME: What is this? Node JS?
        channel_query["query"] = "query GetChannelVideos($id: String!, $limit: Float, $offset: Float) {\n  getChannel(id: $id) {\n    _id\n    videos(limit: $limit, offset: $offset) {\n      ...DisplayVideoFields\n      __typename\n    }\n    __typename\n  }\n}\n\nfragment DisplayVideoFields on Video {\n  _id\n  title\n  summary\n  playCount\n  largeImage\n  embedUrl\n  published\n  videoDuration\n  channel {\n    _id\n    title\n    avatar\n    __typename\n  }\n  createdAt\n  __typename\n}\n";
        // GENERATE
        Json j = channel_query;
        j.dump(channel_query_string);
    }

    // Round to half an hour for JS name to avoid hitting their API too often. Be nice.
    sprintf(timestr,"%04u%02u%02u-%02u%02u%02u",
        tm.tm_year+1900,
        tm.tm_mon+1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min - (tm.tm_min % 30),
        0);

    string js_file = string("iw-api-") + timestr + ".js";//WARNING: No spaces allowed!

    if (stat(js_file.c_str(),&st) || !S_ISREG(st.st_mode) || st.st_size == 0) {
        /* NTS: We trust the JSON will not have '@' or shell escapable chars */
        string cmd = "curl -X POST --data '" + channel_query_string + "' --header 'Content-Type:application/json' -o '" + js_file + "' 'https://vod-api.infowars.com/graphql'";
        int x = system(cmd.c_str());
        if (x != 0) return 1;
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

    return 0;
}

