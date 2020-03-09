
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>

#include <json11.hpp>

#include <libxml/parser.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>

#include <string>

using namespace std;
using namespace json11;

int                         download_bitrate = 2000;

const string default_channel = "5b885d33e6646a0015a6fa2d"; /* The Alex Jones Show */

string chosen_channel = default_channel;

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
    int download_limit = 2;
    struct stat st;

    if (parse_argv(argc,argv))
        return 1;

    init_marker();

    // per-machine adjustment
    {
        char hostname[256] = {0};
        gethostname(hostname,sizeof(hostname)-1);

        if (!strcmp(hostname,"something")) {
            download_bitrate = 500;

            /* go faster on BitChute on Friday, Saturday, Sunday */
            if (tm.tm_wday == 6/*sat*/ || tm.tm_wday == 0/*sun*/ || tm.tm_wday == 1/*mon*/) {
                download_bitrate = 2000;
            }
        }
    }

    /* construct the query JSON for the POST request */
    string channel_query_string;
    {
        Json::object channel_query;
        channel_query["operationName"] = "GetChannelVideos";
        {
            Json::object obj;
            obj["id"] = chosen_channel;
            obj["limit"] = 999;
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
        tm.tm_hour - (tm.tm_hour % 6), /* every six hours */
        0,
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
            /* move it aside, don't try to re-use it */
            string newname = js_file + ".reject.js";

            rename(js_file.c_str(), newname.c_str());

            fprintf(stderr,"JSON parse error: %s\n",json_err.c_str());
            return 1;
        }

        delete[] buf;
        close(fd);
    }

    auto &json_data = json["data"];
    if (!json_data.is_object()) return 1;
    auto &json_getChannel = json_data["getChannel"];
    if (!json_getChannel.is_object()) return 1;
    auto &json_videos = json_getChannel["videos"];
    if (!json_videos.is_array()) return 1;

    for (auto &json_vobj : json_videos.array_items()) {
        if (!json_vobj.is_object()) continue;

        string _id;
        {
            auto &id = json_vobj["_id"];
            if (id.is_string()) _id = id.string_value();
        }
        if (_id.empty()) continue;

        if (_id.find_first_of('/') != string::npos ||
            _id.find_first_of('$') != string::npos ||
            _id.find_first_of('\\') != string::npos ||
            _id.find_first_of('\'') != string::npos ||
            _id.find_first_of('\"') != string::npos)
            continue;

        /* do not download if downloaded already */
        string marker_file = get_mark_filename(_id);
        if (stat(marker_file.c_str(),&st) == 0 && S_ISREG(st.st_mode))
            continue;

        /* NTS: embedUrl contains the value _id at the end */
        string embedUrl;
        {
            auto &id = json_vobj["embedUrl"];
            if (id.is_string()) embedUrl = id.string_value();
        }
        if (embedUrl.empty()) continue;
        {
            const char *s = embedUrl.c_str();
            const char *r = strrchr(s,'/');
            if (r == NULL) continue;
            assert(*r == '/');
            r++;
            if (_id != r) continue;
        }

        if (embedUrl.find_first_of('$') != string::npos ||
            embedUrl.find_first_of('\\') != string::npos ||
            embedUrl.find_first_of('\'') != string::npos ||
            embedUrl.find_first_of('\"') != string::npos)
            continue;

        string title;
        {
            auto &id = json_vobj["title"];
            if (id.is_string()) title = id.string_value();
        }

        string summary;
        {
            auto &id = json_vobj["summary"];
            if (id.is_string()) summary = id.string_value();
        }

        /* write title marker */
        string tagname = _id;
        if (!title.empty()) {
            tagname += "-";
            tagname += title;
            if (!summary.empty()) {
                tagname += "-";
                tagname += summary;
            }
        }
        if (tagname != _id) {
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

        /* the downloadable MP4 isn't in the JSON, we have to go fetch the embedUrl to get it */
        string html = _id + ".html";
        if (stat(html.c_str(),&st)) {
            string htmlpart = html + ".part";
            string cmd = "wget -O '" + htmlpart + "' '" + embedUrl + "'";
            int x = system(cmd.c_str());
            if (x != 0) return 1;
            if (rename(htmlpart.c_str(),html.c_str())) continue;
        }

        /* look for the download url in the HTML */
        string mp4_file_ext = ".mp4";
        string downloadurl;
        {
            xmlDocPtr docp;
            xmlNodePtr htmlp;
            xmlNodePtr htmln = NULL;
            xmlNodePtr bodyn = NULL;

            docp = htmlParseFile(html.c_str(),NULL);
            if (docp == NULL) continue;

            htmlp = xmlDocGetRootElement(docp);
            if (htmlp != NULL) {
                for (xmlNodePtr n=htmlp;n;n=n->next) {
                    if (!strcasecmp((char*)n->name,"html")) {
                        if (n->children != NULL) {
                            htmln = n->children;
                            break;
                        }
                    }
                }
            }

            if (htmln != NULL) {
                for (xmlNodePtr n=htmln;n;n=n->next) {
                    if (!strcasecmp((char*)n->name,"body")) {
                        if (n->children != NULL) {
                            bodyn = n->children;
                            break;
                        }
                    }
                }
            }

            if (bodyn != NULL) {
                for (xmlNodePtr n=bodyn;n;n=n->next) {
                    if (!strcasecmp((char*)n->name,"div")) {
                        {
                            xmlChar *xp;
                            xp = xmlGetNoNsProp(n,(const xmlChar*)"downloadurl");
                            if (xp != NULL) {
                                if (strstr((char*)xp,".mp4") != NULL) {
                                    downloadurl = (char*)xp;
                                }
                                else if (strstr((char*)xp,".mov") != NULL || strstr((char*)xp,".MOV") != NULL) {
                                    /* some Harrison Smith videos provide a MOV download link? */
                                    downloadurl = (char*)xp;
                                    mp4_file_ext = ".mov";
                                }

                                xmlFree(xp);
                            }
                        }
                    }
                }
            }

            xmlFreeDoc(docp);
        }

        if (downloadurl.empty())
            continue;

        if (downloadurl.find_first_of('$') != string::npos ||
            downloadurl.find_first_of('\\') != string::npos ||
            downloadurl.find_first_of('\'') != string::npos ||
            downloadurl.find_first_of('\"') != string::npos)
            continue;

        string mp4_file = _id + mp4_file_ext;

        if (stat(mp4_file.c_str(),&st)) {
            string mp4_file_part = mp4_file + ".part";
            string cmd = string("wget --continue --no-use-server-timestamps --limit-rate=") + to_string(download_bitrate) + "K -O '" + mp4_file_part + "' '" + downloadurl + "'";
            int x = system(cmd.c_str());
            if (x != 0) return 1;
            if (rename(mp4_file_part.c_str(),mp4_file.c_str())) continue;
        }

        /* done */
        mark_file(_id);

        /* count */
        if (++download_limit >= download_count)
            break;
    }

    return 0;
}

