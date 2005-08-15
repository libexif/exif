#include <config.h>
#include "exif-i18n.h"

#ifdef HAVE_ICONV
#  include <iconv.h>
#  include <langinfo.h>
#endif

#include <string.h>
#include <sys/types.h>

#undef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

const char *
exif_i18n_convert_utf8_to_lat1 (const char *in)
{
#ifdef HAVE_ICONV
	static iconv_t tr = 0;
	size_t t = (in ? strlen (in) : 0);
	static char buf[2048];
	size_t buf_size = sizeof (buf);
	char *out = buf;

	if (!in) return NULL;

	memset (buf, 0, sizeof (buf));
	if (!tr) tr = iconv_open (nl_langinfo (CODESET), "UTF-8");
	iconv (tr, (char **) &in, &t, (char **) &out, &buf_size);
	return buf;
#else
	return in;
#endif
}
