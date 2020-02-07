
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

int main(int argc,char **argv) {
    string api_url;
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    char timestr[128];

    if (argc < 2) {
        fprintf(stderr,"Need channel URL\n");
        return 1;
    }
    api_url = argv[1];

    string js_file = "playlist-combined.json"; // use .json not .js so the archive-off script does not touch it
    {
        sprintf(timestr,"%04u%02u%02u-%02u%02u%02u",
            tm.tm_year+1900,
            tm.tm_mon+1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec - (tm.tm_sec % 15));
    }
    string js_tmp_file = string("playlist-tmp-%s.js") + timestr;

    return 0;
}

