#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "util.h"
#include "xml.h"

static XMLParser parser; /* XML parser state */
static char url[2048], text[256], title[256];

static void
printsafe(const char *s)
{
	for (; *s; s++) {
		if (iscntrl((unsigned char)*s))
			continue;
		else if (*s == '\\')
			fputs("\\\\", stdout);
		else if (*s == '\'')
			fputs("'\\''", stdout);
		else
			putchar(*s);
	}
}

static void
xmltagend(XMLParser *p, const char *t, size_t tl, int isshort)
{
	if (strcasecmp(t, "outline"))
		return;

	if (url[0]) {
		fputs("\tfeed '", stdout);
		if (title[0])
			printsafe(title);
		else if (text[0])
			printsafe(text);
		else
			fputs("unnamed", stdout);
		fputs("' '", stdout);
		printsafe(url);
		fputs("'\n", stdout);
	}
	url[0] = text[0] = title[0] = '\0';
}

static void
xmlattr(XMLParser *p, const char *t, size_t tl, const char *n, size_t nl,
	const char *v, size_t vl)
{
	if (strcasecmp(t, "outline"))
		return;

	if (!strcasecmp(n, "title"))
		strlcat(title, v, sizeof(title));
	else if (!strcasecmp(n, "text"))
		strlcat(text, v, sizeof(text));
	else if (!strcasecmp(n, "xmlurl"))
		strlcat(url, v, sizeof(url));
}

static void
xmlattrentity(XMLParser *p, const char *t, size_t tl, const char *n, size_t nl,
	const char *v, size_t vl)
{
	char buf[16];
	ssize_t len;

	if ((len = xml_entitytostr(v, buf, sizeof(buf))) < 0)
		return;
	if (len > 0)
		xmlattr(p, t, tl, n, nl, buf, len);
	else
		xmlattr(p, t, tl, n, nl, v, vl);
}

int
main(void)
{
	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	parser.xmlattr = xmlattr;
	parser.xmlattrentity = xmlattrentity;
	parser.xmltagend = xmltagend;

	fputs(
	    "#sfeedpath=\"$HOME/.sfeed/feeds\"\n"
	    "\n"
	    "# list of feeds to fetch:\n"
	    "feeds() {\n"
	    "	# feed <name> <feedurl> [basesiteurl] [encoding]\n", stdout);
	/* NOTE: getnext is defined in xml.h for inline optimization */
	xml_parse(&parser);
	fputs("}\n", stdout);

	return 0;
}
