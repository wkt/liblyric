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


static const gchar *zh_CN[]={"CP936","UTF-16",NULL};
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
    
    if(a->time == b->time)
        return 0;
    if(a->time > b->time)
        return 1;
    if(a->time < b->time)
        return -1;
    return 0;
}

LyricInfo *
lyric_read(const gchar *filename)
{
    GList *_ret = NULL;
    GList *l = NULL;
    GList *times = NULL;
    LyricLine *ll = NULL;
    FILE *fp;
    size_t n;
    char *line = NULL;
    char *pt;
    gchar *li = NULL;
    goffset fset = 0;
    gint lcn = 0;
    LyricInfo *lyricinfo = NULL;

    fp = fopen(filename,"r");
    if(fp == NULL){
        fprintf(stderr,"fopen(%s):%m\n",filename);
        return NULL;
    }

    lyricinfo = g_new0(LyricInfo,1);
    lyricinfo->content_free = lyric_line_free_list;

    while(getline(&line,&n,fp) != -1){
        pt = g_strstrip(line);
        li = NULL;
        if(pt[0] == '['){
            if(g_ascii_strncasecmp(pt+1,"ti:",3) == 0){
                lyricinfo->title = g_strdup(pt+4);
                li = lyricinfo->title;
            }else if(g_ascii_strncasecmp(pt+1,"ar:",3) == 0){
                lyricinfo->artist = g_strdup(pt+4);
                li = lyricinfo->artist;
            }else if(g_ascii_strncasecmp(pt+1,"al:",3) == 0){
                lyricinfo->album = g_strdup(pt+4);
                li = lyricinfo->album;
            }else if(g_ascii_strncasecmp(pt+1,"by:",3) == 0){
                lyricinfo->author = g_strdup(pt+4);
                li = lyricinfo->author;
            }else if(isdigit(pt[1])){
                pt++;
                lcn = 1;
                while(*pt && lcn == 1){
                    gint64 *t;
                    lcn = 0;
                    t = g_new0(gint64,1);
                    *t = atoll(pt)*60*1000;
                    while(*pt){
                        pt++;
                        if(*pt == ':'){
                            pt ++;
                            *t = *t + atof(pt)*1000;
                            lcn = 0;
                        }else if(*pt == ']'){
                            fset = pt-line+1;
                            pt++;
                            if(*pt =='[' && isdigit(*(pt+1))){
                                lcn = 1;
                                pt++;
                            }
                            times = g_list_append(times,t);
                            break;
                        }
                    }
                }
            }
        }
        if(li){
            size_t len = strlen(li)-1;
            if(len >0 && li[len] == ']')
                li[len]='\0';
        }
        for(l= times;l;l=l->next){
            ll = g_new0(LyricLine,1);
            ll->time = l->data?*((gint64*)l->data):0;
            ll->line = g_strdup(line+fset);
            _ret = g_list_append(_ret,ll);
        }
        li = NULL;
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

const gchar *
lyric_info_get_line(LyricInfo *info,gsize n)
{
    LyricLine *ll = g_list_nth_data(info->content,n);
    return (const gchar*)ll->line;
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

#ifdef TEST_LYRIC_READ

static void
lyric_line_list(GList *l)
{
    LyricLine *ll;
    for(;l;l=l->next){
        ll=l->data;
        fprintf(stderr,"%-6lld->%s\n",ll->time,ll->line);
    }
}

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
    fprintf(stdout,"offset:%llu\n",info->offset);
    lyric_line_list(info->content);
}

int main(int argc,char **argv)
{
    lyric_info_show(lyric_read(argv[1]));
    return 0;
}
#endif //TEST_LYRIC_READ
