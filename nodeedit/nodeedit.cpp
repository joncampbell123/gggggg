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

FILE *ls_fp = NULL;

void removeJS(xmlDocPtr doc,xmlNodePtr node,const char *rpath) {
#if 0
	while (node != NULL) {
		if (node->name == NULL) {
		}
		else if (!strcmp((char*)node->name,"a")) {
			string href;

			{
				xmlChar *xp;
				xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
				if (xp != NULL) {
					href = (char*)xp;
					xmlFree(xp);
				}
			}

			{
				const char *s = href.c_str();

				if (!strncasecmp(s,"javascript:",11)) {
					xmlUnsetProp(node,(const xmlChar*)"href");
				}
			}

			/* on click */
			xmlUnsetProp(node,(const xmlChar*)"onload");
			xmlUnsetProp(node,(const xmlChar*)"onclick");
			xmlUnsetProp(node,(const xmlChar*)"onunload");
			xmlUnsetProp(node,(const xmlChar*)"onmouseup");
			xmlUnsetProp(node,(const xmlChar*)"onmouseout");
			xmlUnsetProp(node,(const xmlChar*)"onmousedown");
			xmlUnsetProp(node,(const xmlChar*)"onmouseover");
		}
		else if (!strcmp((char*)node->name,"body")) {
			xmlUnsetProp(node,(const xmlChar*)"onload");
			xmlUnsetProp(node,(const xmlChar*)"onclick");
			xmlUnsetProp(node,(const xmlChar*)"onunload");
			xmlUnsetProp(node,(const xmlChar*)"onmouseup");
			xmlUnsetProp(node,(const xmlChar*)"onmouseout");
			xmlUnsetProp(node,(const xmlChar*)"onmousedown");
			xmlUnsetProp(node,(const xmlChar*)"onmouseover");
		}
		else if (!strcmp((char*)node->name,"script")) {
			xmlNodePtr n = node->next;
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			node = n;
			continue;
		}

		if (node->children)
			removeJS(doc,node->children,rpath);

		node = node->next;
	}
#endif
}

void writeFile(const char *src,const std::string &tmp,xmlDocPtr doc) {
	FILE *fp = fopen(tmp.c_str(),"w");
	if (fp == NULL) abort();
	htmlDocDump(fp,doc);
	fclose(fp);
	unlink(src);
	rename(tmp.c_str(),src);
}

char read_char(void) {
	char c = 0;

	read(0/*STDIN*/,&c,1);

	return c;
}

std::string read_in(void) {
	std::string ret;
	char c;

	c = read_char();
	ret += c;

	if (c == 27) {
		c = read_char();
		ret += c;

		if (c == '[') {
			do {
				c = read_char();
				ret += c;
			} while(isdigit(c));
		}
	}

	return ret;
}

struct undoEntry {
	xmlDocPtr		doc;
	xmlNodePtr		nav;
};

size_t max_undo = 15;
std::list<undoEntry> undo_queue;

void discard_oldest_undo(void) {
	if (!undo_queue.empty()) {
		undoEntry &x = undo_queue.front();
		assert(x.doc != NULL);
		assert(x.nav != NULL);
		xmlFreeDoc(x.doc);
		x.doc = NULL;
		x.nav = NULL;
		undo_queue.pop_front();
	}
}

void clear_undo(void) {
	if (!undo_queue.empty())
		discard_oldest_undo();
}

void pop_undo(xmlDocPtr &doc,xmlNodePtr &nav) {
	if (!undo_queue.empty()) {
		assert(doc != NULL);
		xmlFreeDoc(doc);

		undoEntry &x = undo_queue.back();
		assert(x.doc != NULL);
		assert(x.nav != NULL);
		doc = x.doc;
		nav = x.nav;

		undo_queue.pop_back();
	}
}

void xmlDocMatchNav(xmlNodePtr &d_nav,xmlNodePtr s_nav) {
	while (1) {
		if (d_nav->prev)
			d_nav = d_nav->prev;
		else if (d_nav->parent && d_nav->parent->parent != NULL/*not root*/)
			d_nav = d_nav->parent;
		else
			break;
	}

	std::vector<char> navc;

	while (1) {
		if (s_nav->prev) {
			s_nav = s_nav->prev;
			navc.push_back('<');
		}
		else if (s_nav->parent && s_nav->parent->parent != NULL/*not root*/) {
			assert(s_nav->prev == NULL);
			s_nav = s_nav->parent;
			navc.push_back('^');
		}
		else
			break;
	}

	if (navc.empty()) return;
	assert(navc.size() != 0);

	for (size_t i=navc.size()-1u;(ssize_t)i >= 0;i--) {
		char how = navc[i];

		if (how == '^') {
			assert(d_nav->children != NULL);
			d_nav = d_nav->children;
		}
		else if (how == '<') {
			assert(d_nav->next != NULL);
			d_nav = d_nav->next;
		}
		else {
			abort();
		}
	}
}

void push_undo(xmlDocPtr doc,xmlNodePtr nav) {
	if (undo_queue.size() >= max_undo)
		discard_oldest_undo();

	undoEntry ent;

	assert(doc != NULL);
	xmlDocPtr copy = xmlCopyDoc(doc,1);
	assert(copy != NULL);

	ent.doc = copy;
	ent.nav = xmlDocGetRootElement(ent.doc);

	/* need the copy nav to match nav */
	xmlDocMatchNav(/*&*/ent.nav, nav);

	sleep(1);

	undo_queue.push_back(ent);
}

void printf_tag(xmlNodePtr trc) {
	printf("%p <%s",(void*)trc,(const char*)trc->name != NULL ? (const char*)trc->name : "DOCROOT");

	int acount = 0;
	for (xmlAttrPtr p=trc->properties;p!=NULL;p=p->next) {
		if (p->children && p->name) {
			xmlChar *xp;
			xp = xmlNodeListGetString(trc->doc,p->children,1);
			if (xp != NULL) {
				printf(" \"%s\"=\"%s\"",p->name,(char*)xp);
				xmlFree(xp);
				acount++;
			}
		}

	}

	printf(">");

	if (trc->name == NULL)
		return;

	if (xmlNodeIsText(trc) ||
		!(strcasecmp((char*)trc->name,"body") == 0 ||
		  strcasecmp((char*)trc->name,"html") == 0 ||
		  strcasecmp((char*)trc->name,"meta") == 0)) {
		xmlChar *xp;
		xp = xmlNodeGetContent(trc);
		if (xp != NULL) {
			std::string str;

			{
				char *s = (char*)xp;
				char pc,c;

				while ((c=(*s++)) != 0) {
					if (str.length() >= 300) {
						str += "...";
						break;
					}

					if (c == '\t') c = ' ';
					if (c > 32 || c < 0) str += c;
					else if (c == 32 && pc != 32) str += c;

					pc = c;
				}
			}

			printf("%s",str.c_str());
			xmlFree(xp);
		}
	}
}

xmlNodePtr nextElementById(xmlNodePtr nav,const std::string &what) {
	xmlNodePtr init_nav = nav;
	xmlNodePtr ret = nav;

	do {
		if (nav->children)
			nav = nav->children;
		else if (nav->next)
			nav = nav->next;
		else {
			do {
				if (nav->parent) {
					nav = nav->parent;
					if (nav->next) {
						nav = nav->next;
						break;
					}
				}
				else {
					break;
				}
			} while(1);
		}

		if (nav->parent == NULL && nav->next == NULL)
			break;

		{
			xmlChar *xp;
			xp = xmlGetNoNsProp(nav,(const xmlChar*)"id");
			if (xp != NULL) {
				if (!strncasecmp((char*)xp,what.c_str(),what.length())) {
					ret = nav;
					xmlFree(xp);
					break;
				}
				else {
					xmlFree(xp);
				}
			}
		}
	} while(1);

	return ret;
}

void editorLoop(const char *src,const std::string &tmp,xmlDocPtr &doc) {
	unsigned char run = 1;
	unsigned char redraw = 1;
	struct termios omode,mode;
	std::string key;
	xmlNodePtr html;
	xmlNodePtr nav;

	html = xmlDocGetRootElement(doc);
	if (html == NULL) return;

	tcgetattr(0/*STDIN*/,&omode);
	mode = omode;
	mode.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL);
	tcsetattr(0/*STDIN*/,TCSANOW,&mode);

	nav = html;

	push_undo(doc,nav);

	while (run) {
		if (redraw) {
			std::vector<xmlNodePtr> showList;
			xmlNodePtr trc;
			int count;

			/* erase screen */
			printf("\x1B[2J" "\x1B[H"); fflush(stdout);

			/* trace up to 5 parent nodes */
			trc = nav;
			count = 5;
			showList.push_back(trc);
			while (count-- > 0 && trc->parent != NULL) {
				trc = trc->parent;
				showList.push_back(trc);
			}

			/* show it */
			assert(showList.size() != 1u);
			for (int i=showList.size()-1u;i >= 0;i--) {
				trc = showList[i];
				assert(trc != NULL);

				count = (i == 0) ? 3 : 0;
				while (count-- > 0 && trc->prev != NULL)
					trc = trc->prev;

				count = (i == 0) ? (3+3+1) : 1;
				while (count-- > 0 && trc != NULL) {
					if (trc == nav || trc == showList[i])
						printf("\x1B[1;33;44m");
					else
						printf("\x1B[0m");

					if (i != (showList.size()-1))
						printf("\x1B[%dC",(showList.size() - (i + 1)) * 4);

					printf_tag(trc);
					printf("\n");
					printf("\x1B[0m");

					if (trc->children && trc == nav) {
						xmlNodePtr cc = trc->children;
						int subcount = 3;

						while (subcount-- > 0 && cc != NULL) {
							printf("\x1B[%dC",((showList.size() - (i + 1)) + 1) * 4);
							printf_tag(cc);
							printf("\n");

							cc = cc->next;
						}
					}

					trc = trc->next;
				}
			}

			/* done */
			redraw = 0;
		}

		key = read_in();
		if (key.empty()) break;
		if (key == "\x1B" || key == "\x1B\x1B") break;

		if (key == "\x1B[A") { /* up arrow */
			if (nav && nav->prev) {
				nav = nav->prev;
				redraw = 1;
			}
		}
		else if (key == "\x1B[B") { /* down arrow */
			if (nav && nav->next) {
				nav = nav->next;
				redraw = 1;
			}
		}
		else if (key == "\x1B[D") { /* left arrow */
			if (nav && nav->parent) {
				if (nav->parent->parent) {
					nav = nav->parent;
					redraw = 1;
				}
			}
		}
		else if (key == "\x1B[C") { /* right arrow */
			if (nav && nav->children) {
				nav = nav->children;
				redraw = 1;
			}
		}
		else if (key == "U") {
			if (nav && nav->parent) {
				xmlNodePtr n,old;

				n = nav->parent;

				xmlUnlinkNode(nav);

				old = nav;
				nav = n;
				redraw = 1;

				xmlAddNextSibling(nav,old);

				nav = old;
			}
		}
		else if (key == "D") {
			if (nav) {
				xmlNodePtr n;

					       n = nav->next;
				if (n == NULL) n = nav->prev;
				if (n == NULL) n = nav->parent;
				assert(n != NULL);

				xmlUnlinkNode(nav);
				xmlFreeNode(nav);

				nav = n;
				redraw = 1;
			}
		}
		else if (key == "P") {
			/* erase screen */
			printf("\x1B[2J" "\x1B[H" "SAVE FOR UNDO..."); fflush(stdout);

			push_undo(doc,nav);

			sleep(1);

			printf("\x1B[2J" "\x1B[H"); fflush(stdout);
			redraw = 1;
		}
		else if (key == "B") {
			if (!undo_queue.empty()) {
				/* erase screen */
				printf("\x1B[2J" "\x1B[H" "UNDO..."); fflush(stdout);

				pop_undo(doc,nav);

				if (undo_queue.empty())
					push_undo(doc,nav);

				usleep(100);

				redraw = 1;
			}
		}
		else if (key == "W") {
			/* erase screen */
			printf("\x1B[2J" "\x1B[H" "WRITING..."); fflush(stdout);

			push_undo(doc,nav);

			writeFile(src,tmp,doc);

			sleep(1);

			printf("\x1B[2J" "\x1B[H"); fflush(stdout);
			redraw = 1;
		}
		else if (key == "/") {
			static std::string what;

			printf("\x1B[2J" "\x1B[H" "LOOK FOR:\n"); fflush(stdout);

			while (1) {
				printf("\x0D" "%s" "\x1B[K",what.c_str()); fflush(stdout);

				key = read_in();
				if (key.empty()) break;

				if (key == "\x1B" || key == "\x1B\x1B") break;

				if (key[0] == '\x1B') continue;

				if (key == "\x08" || key == "\x7F") {
					if (what.length() > 0)
						what = what.substr(0,what.length()-1);
				}
				else if (key == "\x0D" || key == "\x0A") {
					printf("\nSEARCHING..."); fflush(stdout);
					if (!what.empty())
						nav = nextElementById(nav,what);
					break;
				}
				else if (key[0] >= 32) {
					what += key;
				}
			}

			redraw = 1;
		}
	}

	tcsetattr(0/*STDIN*/,TCSANOW,&omode);

	/* erase screen, home cursor */
	printf("\x1B[2J" "\x1B[H"); fflush(stdout);
}

void runEditor(const char *src) {
	xmlDocPtr doc;
	string tmp;

	tmp = src;
	tmp += ".tmp.xyz.tmp";

	doc = htmlParseFile(src,NULL);
	if (doc == NULL) return;

	editorLoop(src, tmp, doc);

	xmlFreeDoc(doc);

	clear_undo();
}

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	runEditor(argv[1]);
	return 0;
}

