#include <stdio.h>
#include "LyricSogouSearch.h"
#include "lyricread.h"

#define LRC_URI_FLAG "downlrc"

static gchar* 
sogou_get_uri(const LyricId *lyricid)
{
    gchar *en_ti;
    gchar *en_ar;
    gchar *es_ti;
    gchar *es_ar;
    GString *uri;
    gchar *ret_uri = NULL;

    const gchar *charset="cp936";

    uri = g_string_new("");
    gchar *base_uri="http://mp3.sogou.com/gecisearch.so?query=";

    en_ti = encode_from_utf8(lyricid->title,charset);
    if(!en_ti)
        en_ti = g_strdup(lyricid->title);
    es_ti = g_uri_escape_string(en_ti,NULL,0);

    en_ar = encode_from_utf8(lyricid->artist,charset);
    if(!en_ar)
        en_ar = g_strdup(lyricid->artist);
    es_ar = g_uri_escape_string(en_ar,NULL,0);

    g_string_append_printf(uri,"%s%s",base_uri,es_ti);
    g_string_append_printf(uri,"-%s",es_ar);

    g_free(en_ti);
    g_free(en_ar);
    g_free(es_ti);
    g_free(es_ar);
    ret_uri = uri->str;
    g_string_free(uri,FALSE);
    return ret_uri;
}

static GSList *
sogou_parser_html(const LyricId *lyricid,const gchar *html)
{

    GSList *l= NULL;
    gchar *pt;
    gsize i=0;
    const gchar *charset="cp936";

    pt = (gchar*)html;
    while(pt && *pt &&(pt=strstr(pt,LRC_URI_FLAG))){
        gchar *ti,*ar;
        gchar *es_ar,*es_ti;
        gchar *end;
        LyricId *id;
        end = strchr(pt,'\"');
        if(!end){
            pt++;
            continue;
        }
        end[0]=0;
        id = g_new0(LyricId,1);
        id->uri = g_strdup_printf("http://mp3.sogou.com/%s",pt);
        ti = strrchr(pt,'=');
        ti ++;
        ar = strchr(ti,'-');
        *ar = 0;
        ar ++;
        es_ti = g_uri_unescape_string(ti,NULL);
        for(ti=es_ti;*ti;ti++)
            if(*ti == '+')*ti =' ';
        es_ar = g_uri_unescape_string(ar,NULL);
        for(ti=es_ar;*ti;ti++)
            if(*ti == '+')*ti =' ';
        i++;
        id->no = i;
        id->artist = encode_to_utf8(g_strstrip(es_ar),charset);
        id->title = encode_to_utf8(g_strstrip(es_ti),charset);
        l = g_slist_append(l,id);
        g_free(es_ti);
        g_free(es_ar);
        pt=end+1;
    }
    return l;
}

static LyricSearchEngine SogouSearchEngine=
{
    N_("Sogou"),
    .get_engine_uri = sogou_get_uri,
    .parser = sogou_parser_html,
};

LyricSearchEngine*
lyric_search_get_sogou_engine(void)
{
	return &SogouSearchEngine;
}

#ifdef _test

int main(int argc,char **argv)
{
    LyricId id={"S.H.E",argv[1],NULL};
    gchar *uri;
    GSList *l;
    LyricSearchEngine *engine;
    engine = lyric_search_get_sogou_engine();
    uri = engine->get_engine_uri(&id);
    l = engine->parser(&id,lyric_func_get_contents(uri,0,NULL));
    lyric_func_lyricid_list(l);
    return 0;
}
#endif
