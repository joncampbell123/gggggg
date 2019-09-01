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

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	xmlDocPtr doc;
	doc = htmlParseFile(argv[1],NULL);
	if (doc == NULL) return 1;

	xmlFreeDoc(doc);
	return 0;
}

