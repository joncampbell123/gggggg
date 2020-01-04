
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
        channel_query["operationName"] = "GetChannelPage";
        {
            Json::object obj;
            obj["id"] = chosen_channel;
            channel_query["variables"] = obj;
        }
        // FIXME: What is this? Node JS?
        channel_query["query"] = "query GetChannelPage($id: String!) {\n  getChannel(id: $id) {\n    _id\n    title\n    summary\n    coverImage\n    liveStreamVideo {\n      streamUrl\n      __typename\n    }\n    __typename\n  }\n}\n";
        // GENERATE
        Json j = channel_query;
        j.dump(channel_query_string);
    }

    string js_file = "iw.js";
    if (stat(js_file.c_str(),&st)) {
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

    string rurl;
    auto &json_data = json["data"];
    if (json_data.is_object()) {
        auto &json_gc = json_data["getChannel"];
        if (json_gc.is_object()) {
            auto &json_lsv = json_gc["liveStreamVideo"];
            if (json_lsv.is_object()) {
                auto &url = json_lsv["streamUrl"];
                if (url.is_string()) {
                    rurl = url.string_value();
                }
            }
        }
    }

    if (rurl.empty()) {
        fprintf(stderr,"No live stream URL found\n");
        return 1;
    }

    printf("%s\n",rurl.c_str());
    return 0;
}

