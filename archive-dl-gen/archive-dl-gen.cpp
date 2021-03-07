/* run this at the top of the same directory you built catalog.txt */

#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>

#include <map>
#include <list>
#include <vector>
#include <string>

using namespace std;

map<string,bool>    download_list;

void gather(xmlNodePtr n) {
    for (;n!=NULL;n=n->next) {
        if (n->name != NULL) {
            if (!strcasecmp((char*)n->name,"a")) {
                string href;

                {
                    xmlChar *xp;
                    xp = xmlGetNoNsProp(n,(const xmlChar*)"href");
                    if (xp != NULL) {
                        href = (char*)xp;
                        xmlFree(xp);
                    }
                }

                if (href.empty())
                    continue;
                if (href.find_first_of('/') != string::npos)
                    continue;
                if (href.find_first_of('#') != string::npos)
                    continue;

                download_list[href] = true;
                continue;
            }
        }

        gather(n->children);
    }
}

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	xmlDocPtr doc;
	doc = htmlParseFile(argv[1],NULL);
	if (doc == NULL) return 1;

    xmlNodePtr html = xmlDocGetRootElement(doc);
    if (html == NULL) return 1;

    /* look for <html> */
    while (html) {
        if (!strcasecmp((char*)html->name,"html"))
            break;

        html = html->next;
    }
    if (html == NULL) return 1;
    html = html->children;
    if (html == NULL) return 1;

    /* look for <body> */
    while (html) {
        if (!strcasecmp((char*)html->name,"body"))
            break;

        html = html->next;
    }
    if (html == NULL) return 1;
    html = html->children;
    if (html == NULL) return 1;

    /* scan and gather download links */
    gather(html);

    /* show */
    for (map<string,bool>::iterator i=download_list.begin();i!=download_list.end();i++)
        printf("%s\n",i->first.c_str());

	xmlFreeDoc(doc);
	return 0;
}

