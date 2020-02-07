
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include <json11.hpp>

#include <string>
#include <algorithm>

using namespace std;
using namespace json11;

struct js_entry {
    string      json;

    bool operator==(const struct js_entry &e) const {
        return  json == e.json;
    }
};

char line_tmp[4096];

void chomp(char *s) {
	char *e = s + strlen(s) - 1;
	while (e >= s && (*e == '\n' || *e == '\r')) *e-- = 0;
}

int load_js_list(vector<js_entry> &js,const string &jsfile) {
    js.clear();

    FILE *fp = fopen(jsfile.c_str(),"r");
    if (fp == NULL) return -1; // sets errno

    js_entry cur;
    bool cur_set = false;

    while (!feof(fp) && !ferror(fp)) {
        if (fgets(line_tmp,sizeof(line_tmp),fp) == NULL) break;
        if (line_tmp[0] == 0) continue;
        if (line_tmp[0] == '#') continue;
        chomp(line_tmp);

        // JSON
        cur.json = line_tmp;
        cur_set = true;

        // emit
        if (cur_set) {
            js.push_back(cur);
            cur_set = false;
            cur = js_entry();
        }
    }

    // emit
    if (cur_set) {
        js.push_back(cur);
        cur_set = false;
        cur = js_entry();
    }

    fclose(fp);
    return 0;
}

int write_js_list(const string &jsfile,vector<js_entry> &js) {
    FILE *fp = fopen(jsfile.c_str(),"w");
    if (fp == NULL) return -1; // sets errno

    for (auto jsi=js.begin();jsi!=js.end();jsi++) {
        auto &jsent = *jsi;
        if (jsent.json.empty()) continue;
        fprintf(fp,"%s\n",jsent.json.c_str());
    }

    fclose(fp);
    return 0;
}

int combine_js(const string &dst,const string &js1,const string &js2) {
    vector<js_entry> j1,j2;

    if (load_js_list(j1,js1) < 0) {
        if (errno != ENOENT) { // it's OK if it doesn't exist
            fprintf(stderr,"Cannot load %s\n",js1.c_str());
            return -1;
        }
    }

    if (load_js_list(j2,js2) < 0) {
        fprintf(stderr,"Cannot load %s\n",js2.c_str());
        return -1;
    }

    for (auto j2i=j2.begin();j2i!=j2.end();j2i++) {
        if (find(j1.begin(),j1.end(),*j2i) == j1.end())
            j1.push_back(*j2i);
    }

    if (write_js_list(dst,j1) < 0) {
        fprintf(stderr,"Cannot write %s\n",dst.c_str());
        return -1;
    }

    return 0;
}

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
    string js_mix_tmp = "playlist-tmp-combine.js";

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

    if (combine_js(js_mix_tmp,js_file,js_tmp_file)) {
        fprintf(stderr,"Failed to combine JS\n");
        return 1;
    }
    rename(js_mix_tmp.c_str(),js_file.c_str());
    return 0;
}

