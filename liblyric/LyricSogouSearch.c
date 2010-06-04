#include <stdio.h>
#include "LyricSogouSearch.h"
#include "lyricread.h"

#define LRC_URI_FLAG "downlrc"

GSList* 
sogou_get_lyrics_list(const LyricId *lyricid)
{
    gchar *en_ti;
    gchar *en_ar;
    gchar *es_ti;
    gchar *es_ar;
    GString *uri;
    GSList *l= NULL;
    gchar *html;
    gchar *pt;
    gsize i=0;
    LyricId *id;

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

    fprintf(stdout,"%s\n",uri->str);
    g_free(en_ti);
    g_free(en_ar);
    g_free(es_ti);
    g_free(es_ar);
    html = lyric_func_get_contents(uri->str,NULL,NULL);
    pt=html;
    while(pt && *pt &&(pt=strstr(pt,LRC_URI_FLAG))){
        gchar *ti,*ar;
        gchar *end;
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
    g_free(html);
    g_string_free(uri,TRUE);
    ///lyric_func_lyricid_list(l);
    return l;
}

static LyricSearchEngine SogouSearchEngine=
{
	N_("Sogou"),
	sogou_get_lyrics_list
};

LyricSearchEngine*
lyric_search_get_sogou_engine(void)
{
	return &SogouSearchEngine;
}

#if 0

int main(int argc,char **argv)
{
    
    LyricId id={"S.H.E",argv[1],NULL};
    sogou_get_lyrics_list(&id);
    return 0;
}

#endif
