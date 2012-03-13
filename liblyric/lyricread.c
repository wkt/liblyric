//      lyricread.c
//      
//      Copyright 2010 wkt <weikting@gmail.com>
//      

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <langinfo.h>
#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "lyricread.h"


typedef struct _LocalEncode LocalEncode;

struct _LocalEncode
{
    const gchar *lang;
    const gchar **encodev;
};


static const gchar *zh_CN[]={"CP936",NULL};
static LocalEncode localencode[]=
{
    {"zh_CN",zh_CN},
    {NULL}
};

static void
lyric_line_free_list(GList *l);

const gchar*
get_codeset(void)
{
    const char *codeset = NULL;
    gboolean ret;
    ret = g_get_charset(&codeset);
    if(!ret && codeset == NULL){
        codeset = setlocale(LC_CTYPE, NULL);
        if (codeset != NULL) {
            codeset = strchr(codeset, '.');
            if (codeset != NULL) ++codeset;
        }
    }
    return codeset;
}

const gchar **
guess_codeset(void)
{
    char *codeset = NULL;
    const gchar **cov=localencode[0].encodev;
    int i;

    i = 0;
    codeset = setlocale(LC_CTYPE, NULL);
    if(codeset){
        while(localencode[i].lang){
            if(strncmp(codeset,localencode[i].lang,strlen(localencode[i].lang)) == 0){
                cov = localencode[i].encodev;
                break;
            }
            i++;
        }
    }
    return cov;
}

gboolean
is_utf8_native(void)
{
    static gboolean has_run = FALSE;
    static gboolean is_utf8 = TRUE;
    const char *codeset;

    if(has_run)
        return is_utf8;
    has_run = TRUE;

    codeset = get_codeset();
    is_utf8 = codeset ? (strcmp(codeset,"UTF-8") == 0||strcmp(codeset,"utf8") == 0):FALSE;
    return is_utf8;
}

gchar *
encode_to_originally(const gchar *str)
{
    GError* error = NULL;
    if(!str)
            return NULL;
    gchar* pgb = g_convert(str, -1, "iso-8859-1", "utf-8", NULL, NULL, &error);
    if(error)
    {
        fprintf(stderr,"%s",error->message);
        g_error_free(error);
    }
    return pgb;
}

gchar *
guess_string_to_utf8(const gchar *str)
{
    const gchar **codev;
    gchar *ret_str = NULL;
    size_t n = 0;
    gsize nw =0;

    codev = guess_codeset();
    for(n=0;codev && codev[n];n++){
        ret_str = g_convert(str,-1,"utf-8",codev[n],NULL,&nw,NULL);
        if(ret_str){
            break;
        }
        g_free(ret_str);
        ret_str = NULL;
    }
    return ret_str;
}

gchar *
guess_encode_to_utf8(const gchar *str)
{
    gchar *orig;
    gchar *ret_str;

    orig = encode_to_originally(str);
    if(orig){
        ret_str = guess_string_to_utf8(orig);
        g_free(orig);
    }else{
        ret_str = guess_string_to_utf8(str);
    }
    return ret_str;
}

gchar*
encode_to_utf8(const gchar *str,const gchar *charset)
{
    gchar *ret_str;
    ret_str = g_convert(str,-1,"utf-8",charset,NULL,NULL,NULL);
    return ret_str;
}

gchar*
encode_from_utf8(const gchar *str,const gchar *charset)
{
    gchar *ret_str;
    ret_str = g_convert(str,-1,charset,"utf-8",NULL,NULL,NULL);
    return ret_str;
}

void
lyric_line_free(LyricLine *ll)
{
    g_free(ll->line);
    g_free(ll);
}

static gint
cmpfunc(LyricLine *a,LyricLine *b)
{
    if(a == b)
        return 0;
    if(a == NULL)
        return -1;
    if(b == NULL)
        return 1;
    if(a->time == b->time)
        return 0;
    if(a->time > b->time)
        return 1;
    if(a->time < b->time)
        return -1;
    return 0;
}

const gchar*
lyric_line_tag(const gchar *s,gchar **next)
{
    gchar *r = NULL;
    gchar *pt = (gchar*)s;
    if(pt[0] == '[')
    {
        gchar *t = NULL;
        pt++;
        t = pt;
        do{
            if(*pt == ']')
            {
                r = t;
                *pt = 0;
                pt++;
                if(next)*next = pt;
                break;
            }
            pt++;
        }while(*pt);
    }
    return r;
}

guint64
lyric_line_tag_time(const gchar *tag)
{
    gint64 t = 0;
    t = g_ascii_strtoull(tag,NULL,10)*60*1000;
    t = t+g_ascii_strtod(tag+3,NULL)*1000;
    return t;
}

LyricInfo *
lyric_read(const gchar *filename)
{
    GList *_ret = NULL;
    GList *l = NULL;
    GList *times = NULL;
    FILE *fp;
    size_t n;
    char *line = NULL;
    char *pt;
    gchar *tmp_str = NULL;
    gchar *next_pt = NULL;
    gint64 *time_pt = NULL;
    goffset fset = 0;
    LyricInfo *lyricinfo = NULL;
    LyricLine *ll = NULL;

    fp = fopen(filename,"r");
    if(fp == NULL){
        fprintf(stderr,"fopen(%s):%m\n",filename);
        return NULL;
    }

    lyricinfo = g_new0(LyricInfo,1);
    lyricinfo->content_free = lyric_line_free_list;

    while(getline(&line,&n,fp) != -1){
        pt = g_strstrip(line);
        next_pt = pt;
        while(pt[0] == '['){
            tmp_str = lyric_line_tag(pt,&next_pt);
            if(tmp_str == NULL)
                break;
            if(g_ascii_strncasecmp("ti:",tmp_str,3) == 0){
                lyricinfo->title = g_strdup(tmp_str+3);
            }else if(g_ascii_strncasecmp("ar:",tmp_str,3) == 0){
                lyricinfo->artist = g_strdup(tmp_str+3);
            }else if(g_ascii_strncasecmp("al:",tmp_str,3) == 0){
                lyricinfo->album = g_strdup(tmp_str+3);
            }else if(g_ascii_strncasecmp("by:",tmp_str,3) == 0){
                lyricinfo->author = g_strdup(tmp_str+3);
            }else if(isdigit(pt[1])){
                pt = next_pt;
                time_pt = g_new0(gint64,1);
                *time_pt = lyric_line_tag_time(tmp_str);
                times = g_list_append(times,time_pt);
            }else{
                break;
            }
        }

        for(l= times;l;l=l->next){
            time_pt = (gint64*)l->data;
            ll = g_new0(LyricLine,1);
            ll->time = time_pt?*time_pt:0;
            ll->line = g_strdup(next_pt);
            _ret = g_list_append(_ret,ll);
        }
        g_list_foreach(times,(GFunc)g_free,NULL);
        g_list_free(times);
        times = NULL;
    }
    if(line)
        free(line);
    fclose(fp);
    _ret = g_list_sort(_ret,(GCompareFunc)cmpfunc);
    lyricinfo->content = _ret;
    return lyricinfo;
}

const LyricLine*
lyric_info_get_line(LyricInfo *info,gsize n)
{
    const LyricLine *ll = g_list_nth_data(info->content,n);
    return ll;
}

gsize
lyric_info_get_n_lines(LyricInfo *info)
{
    return g_list_length(info->content);
}

static void
lyric_line_free_list(GList *l)
{
    g_list_foreach(l,(GFunc)lyric_line_free,NULL);
    g_list_free(l);
}

void
lyric_info_free(LyricInfo *info)
{
    g_return_if_fail(info != NULL);
    if(info->content_free){
        info->content_free(info->content);
        info->content = NULL;
    }
    g_free(info);
}

void
lyric_line_list(GList *l)
{
    LyricLine *ll;
    for(;l;l=l->next){
        ll=l->data;
        fprintf(stdout,"%-6ld->%s\n",ll->time,ll->line);
    }
}


#ifdef TEST_LYRIC_READ

void
lyric_info_show(LyricInfo *info)
{
    if(info->title)
        fprintf(stdout,"title:%s\n",info->title);
    if(info->artist)
        fprintf(stdout,"artist:%s\n",info->artist);
    if(info->album)
        fprintf(stdout,"album:%s\n",info->album);
    if(info->author)
        fprintf(stdout,"author:%s\n",info->author);
    fprintf(stdout,"offset:%lu\n",info->offset);
    lyric_line_list(info->content);
}

int main(int argc,char **argv)
{
    lyric_info_show(lyric_read(argv[1]));
    return 0;
}
#endif //TEST_LYRIC_READ
