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
exif_i18n_convert_utf8_to_locale (const char *in)
{
#if defined(ENABLE_GETTEXT_ICONV) && defined(HAVE_ICONV)
	/* If gettext() doesn't convert the message texts into the proper
	 * encoding for the current locale, then it's broken (because there's
	 * no way for the app to know the encoding of the translated text).
	 * In this case, assume the translated text is in UTF-8 (which could
	 * be wrong) and use iconv to convert to the proper encoding.
	 * This is only an issue with really old gettext versions (< 0.10.36) 
	 */
	static iconv_t tr = 0;
	size_t t = (in ? strlen (in) : 0);
	static char buf[2048];
	size_t buf_size = sizeof (buf);
	char *out = buf;

	if (!in) return "";

	memset (buf, 0, sizeof (buf));
	if (!tr) tr = iconv_open (nl_langinfo (CODESET), "UTF-8");
	iconv (tr, (char **) &in, &t, (char **) &out, &buf_size);
	return buf;
#else
	if (!in) return "";
	return in;
#endif
}
