#include <glib.h>
#include "LyricSearchCurl.h"

static size_t
curl_write_function( void *ptr, size_t size, size_t nmemb, GString *str)
{
	g_string_append_printf(str,ptr,size*nmmeb);
	return size;
}

GString*
lyric_search_get_data(const char *uri)
{
	URLcode ucode;
	CURL *curl;
	GString *str;

	ucode = curl_global_init(CURL_GLOBAL_ALL);
	if(ucode != CURLE_OK)
		return NULL;
	str = g_string_new("");
	curl = curl_easy_init( )
	curl_easy_setopt(curl, CURLOPT_URL,uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_write_function);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,str);
	ucode = curl_easy_perform(curl,curl);
	if(ucode != CURLE_OK){
		fprintf(stderr,"curl_easy_perform():%s\n",curl_easy_strerror(ucode));
		g_string_free(str,TRUE);
		str = NULL;
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return str;
}
