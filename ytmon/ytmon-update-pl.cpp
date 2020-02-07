
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
            tm.tm_sec - (tm.tm_sec % 60)); // TODO: reduce to 15 seconds if confident
    }
    string js_tmp_file = string("playlist-tmp-") + timestr + ".js";

    {
        struct stat st;

        memset(&st,0,sizeof(st));
        if (stat(js_tmp_file.c_str(),&st) != 0 || st.st_size < 32) {
            assert(js_tmp_file.find_first_of(' ') == string::npos);
            assert(js_tmp_file.find_first_of('$') == string::npos);
            assert(js_tmp_file.find_first_of('\'') == string::npos);
            assert(js_tmp_file.find_first_of('\"') == string::npos);

            assert(api_url.find_first_of(' ') == string::npos);
            assert(api_url.find_first_of('$') == string::npos);
            assert(api_url.find_first_of('\'') == string::npos);
            assert(api_url.find_first_of('\"') == string::npos);

            fprintf(stderr,"Downloading playlist...\n");

            /* -j only emits to stdout, sorry. */
            /* FIXME: Limit to the first 99. Some channels have so many entries that merely querying them causes YouTube
             *        to hit back with "Too many requests".
             *
             *        Speaking of gradual playlist building... the same logic should be implemented into the banned.video
             *        downloader as well. */
            string cmd = string("youtube-dl --no-mtime -j --flat-playlist --playlist-end=99 ") + " \"" + api_url + "\" >" + js_tmp_file;
            int status = system(cmd.c_str());
            if (status != 0) return 1;
        }
    }

    /* TODO: Combine tmp with js only if download completed. */

    return 0;
}

