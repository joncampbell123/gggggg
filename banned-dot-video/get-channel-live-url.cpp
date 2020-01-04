
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

    return 0;
}

