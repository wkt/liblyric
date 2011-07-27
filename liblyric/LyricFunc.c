#include "LyricFunc.h"
#include <stdio.h>

#include "lyricread.h"

#include <glib.h>

#if 0
#include <curl/curl.h>

static size_t
curl_write_function(void *ptr, size_t size, size_t nmemb, GString **str)
{
	size_t n = size*nmemb;
	gchar *pt = ptr;
	if(*str == NULL){
		*str = g_string_new("");
	}
	*str = g_string_append_len(*str,pt,n);
	return n;
}

GString*
lyric_search_get_data(const char *uri)
{
	CURLcode ucode;
	CURL *curl;
	GString *str = NULL;

	ucode = curl_global_init(CURL_GLOBAL_ALL);
	if(ucode != CURLE_OK)
		return NULL;
	curl = curl_easy_init( );
	curl_easy_setopt(curl, CURLOPT_URL,uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_write_function);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,&str);
	ucode = curl_easy_perform(curl);
	if(ucode != CURLE_OK){
		fprintf(stderr,"curl_easy_perform():%s\n",curl_easy_strerror(ucode));
		g_string_free(str,TRUE);
		str = NULL;
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return str;
}

char*
lyric_func_get_contents(const char *uri,gsize *length, GError **error)
{
	GString *str;
	gchar *text = NULL;
	str = lyric_search_get_data(uri);
	if(str){
		text = str->str;
		if(length)
			*length = str->len;
		g_string_free(str,0);
	}
	return text;
}

#else
char*
lyric_func_get_contents(const char *uri,gsize *length, GError **error)
{
	GFile *gf;
	char *text = NULL;
	gf = g_file_new_for_uri(uri);
	g_file_load_contents(gf,NULL,&text,length,NULL,error);
	g_object_unref(gf);
	return text;
}
#endif

gboolean
lyric_func_save_data(const gchar *filename,const gchar *data,gsize length,GError **error)
{
	const gchar *text = data;
	gboolean ret_bl = FALSE;
	
	if(length < 0){
		length = strlen(data);
	}
	if(text){
		if(!g_utf8_validate(text,-1,NULL)){
			gchar *utf8;
			utf8 = guess_string_to_utf8(text);
			if(utf8){
				g_free(text);
				text = utf8;
				length = strlen(utf8);
			}
		}
		fprintf(stderr,"fetch:%s\n",filename);
		ret_bl = g_file_set_contents(filename,text,length,error);
		fprintf(stderr,"save lyric to %s : %s\n",filename,ret_bl?"OK":"NO");
	}
	return ret_bl;
}

gboolean
lyric_func_save_lyric(const char *uri,const gchar *filename,GError **error)
{
	char *text;
	gsize length;
	gboolean ret_bl;

	fprintf(stderr,"fetch:%s\n",uri);
	text = lyric_func_get_contents(uri,&length,NULL);
	text[length]=0;
	ret_bl = lyric_func_save_data(filename,text,length,error);
	g_free(text);
	return ret_bl;
}

static void
lyricid_free(LyricId *item)
{
	if(!item){
		g_debug("come to where nerver should come .");
		return ;
	}
	g_free(item->uri);
	g_free(item->artist);
	g_free(item->title);
	g_free(item->album);
	g_free(item);
}

void
lyric_func_free_lyricid_list(GSList *list)
{
	if(list == NULL)
		return ;
	g_slist_foreach(list,(GFunc)lyricid_free,NULL);
	g_slist_free(list);
}

void
lyric_func_lyricid_list(GSList *l)
{
	LyricId *id;
	for(;l;l=l->next){
		id = l->data;
		fprintf(stdout,"------------------------\n");
		fprintf(stdout,"artist:%s\n",id->artist);
		fprintf(stdout,"title :%s\n",id->title);
		fprintf(stdout,"album :%s\n",id->album);
		fprintf(stdout,"uri   :%s\n",id->uri);
		fprintf(stdout,"no    :%d\n",id->no);
		fprintf(stdout,"------------------------\n");
	}
}
