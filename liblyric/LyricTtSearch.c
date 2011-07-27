#include <stdio.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "LyricTtSearch.h"

int
tt_CodeFunc(unsigned int Id, char* song)
{
	int tmp1;
	int tmp2=0;
	int tmp3=0;
	int i;
	
	short length = strlen(song);
	//0x00015F18
	tmp1 = (Id & 0x0000FF00) >> 8;		//右移8位后为0x0000015F
										//tmp1 0x0000005F
	if ( (Id & 0x00FF0000) == 0 ) {
		tmp3 = 0x000000FF & ~tmp1;		//CL 0x000000E7
	} else {
		tmp3 = 0x000000FF & ((Id & 0x00FF0000) >> 16);	//右移16位后为0x00000001
	}
	tmp3 = tmp3 | ((0x000000FF & Id) << 8);				//tmp3 0x00001801
	tmp3 = tmp3 << 8;									//tmp3 0x00180100
	tmp3 = tmp3 | (0x000000FF & tmp1);					//tmp3 0x0018015F
	tmp3 = tmp3 << 8;									//tmp3 0x18015F00
	if ( (Id & 0xFF000000) == 0 ) {
		tmp3 = tmp3 | (0x000000FF & (~Id));				//tmp3 0x18015FE7
	} else {
		tmp3 = tmp3 | (0x000000FF & (Id >> 24));		//右移24位后为0x00000000
	}
	
	//tmp3	18015FE7
	
	i=length-1;
	while(i >= 0){
		//printf("%d\n",*(song + i));
		tmp2 = (*(song + i)) + tmp2 + (tmp2 << (i%2 + 4));
		//show_bytes((unsigned char*)&tmp2,4);
		i--;
	}
	//tmp2 88203cc2
	i=0;
	tmp1=0;
	while(i<=length-1){
		tmp1 = (*(song+i)) + tmp1 + (tmp1 << (i%2 + 3));
		i++;
	}
	//EBX 5CC0B3BA
	
	//EDX = EBX | Id;
	//EBX = EBX | tmp3;
	//show_bytes((unsigned char*)&tmp1,4);
	//show_bytes((unsigned char*)&tmp2,4);
	//show_bytes((unsigned char*)&tmp3,4);
	return  ((tmp2 ^ tmp3) + (tmp1 | Id)) * (tmp1 | tmp3) * (tmp2 ^ Id);
}


//g_free when no use again.
gchar*
tt_encode_artist_title(const char* str)
{
	int i, j;
	char temp[64];

	assert(str != NULL);
	int str_len = strlen(str);
	int utf16_len = g_utf8_strlen(str, str_len) * 2;

	char* re = g_new0(char, str_len * 6 + 1); //FIXME:这里应是多大才好呢？

	gunichar2* utf16_str = g_utf8_to_utf16(str, 
			str_len,
			NULL,
			NULL,
			NULL);

	char* p = (char*)utf16_str;

	for(i = 0, j = 0; i < utf16_len; ++i)
	{
		sprintf(temp, "%02x", p[i]);
		int n = strlen(temp);
		re[j] = temp[n - 2];
		++j;
		re[j] = temp[n - 1];
		++j;
	}
	re[j] = '\0';

	g_free((gpointer)utf16_str);
	return re;
}

//g_free when no use again.
static gchar*
tt_remove_blankspace_lower(const gchar* src)
{

	gchar* s = NULL;
	int len = src?strlen(src):0;

	if(len == 0)
	{
		s = g_strdup("");
		return s;
	}

	s = g_new0(gchar, len + 1);

	int i, j;
	for(i = 0, j = 0; i < len; ++i)
	{
		if(src[i] != ' ')
		{
			s[j] = g_ascii_tolower(src[i]);
			++j;
		}
	}
	s[j] = '\0';

	return s;
}

static GSList*
tt_parser_xml(const LyricId *lyricid,const gchar* xml)
{
	xmlDoc* doc;
	xmlNode* root_node;
	xmlNode* cur_node;

	GSList* list = NULL;
	int i = 0;

	if(xml == NULL){
		g_debug("Get no XML .");
		return NULL;
	}

	doc = xmlReadDoc((const xmlChar *)xml, NULL, "UTF-8", 0);
	if(doc == NULL)
	{
		g_debug("can't parse xml string\n");
		return NULL;
	}

	root_node = xmlDocGetRootElement(doc);
	if(root_node == NULL)
	{
		g_debug("parse xml string failed\n");
		return NULL;
	}

	for(cur_node = root_node->children; cur_node; cur_node = cur_node->next)
	{
		LyricId* item = g_new0(LyricId, 1);
		item->no = i;
		gchar *id = (gchar*)xmlGetProp(cur_node,(const xmlChar *) "id");
		if(id == NULL) //FIXME: 为什么中间会有一个NULL元素
			continue;
		item->artist = (gchar*)xmlGetProp(cur_node,(const xmlChar *) "artist");
		item->title = (gchar*)xmlGetProp(cur_node, (const xmlChar *)"title");
		gchar* art_tit = g_strconcat(item->artist, item->title, NULL);
		item->uri = g_strdup_printf(
//		            "http://lrcct2.ttplayer.com/dll/lyricsvr.dll?dl?Id=%d&Code=%d&uid=01&mac=%012x",
                    "http://ttlrcct.qianqian.com/dll/lyricsvr.dll?dl?Id=%d&Code=%d&uid=01&mac=%012x",
		            atoi(id), 
		            tt_CodeFunc(atoi(id), art_tit), 
		            g_random_int_range(0, 0x7FFFFFFF));
		g_free(art_tit);
		g_free(id);
		list = g_slist_append(list, item);
		++i;
	}

	xmlFreeDoc(doc);

	return list;
}


static gchar*
tt_get_uri(const LyricId *lyricid)
{
	gchar *artist;
	gchar *title;
///	GError *error = NULL;

	artist = lyricid->artist;
	title = lyricid->title;

	if(!(artist||title))
		return NULL;

	gchar* art1 = tt_remove_blankspace_lower(artist);
	gchar* art2 = tt_encode_artist_title(art1);

	g_free(art1); //FIXME:

	gchar* tit1 = tt_remove_blankspace_lower(title);
	gchar* tit2 = tt_encode_artist_title(tit1);
	g_free(tit1); //FIXME:

	gchar* url = g_strdup_printf( 
///			"http://lrcct2.ttplayer.com/dll/lyricsvr.dll?sh?Artist=%s&Title=%s&Flags=0",
            "http://ttlrcct.qianqian.com/dll/lyricsvr.dll?sh?Artist=%s&Title=%s&Flags=0",
			art2, tit2);

	g_free(art2); //FIXME:
	g_free(tit2); //FIXME:

	return url;
}

static LyricSearchEngine TtSearchEngine=
{
	N_("TT Search"),
	.get_engine_uri = tt_get_uri,
	.parser = tt_parser_xml
};

LyricSearchEngine*
lyric_search_get_tt_engine(void)
{
	return &TtSearchEngine;
}

#ifdef _test

int main(int argc,char **argv)
{
    LyricId id={"S.H.E",argv[1],NULL};
    gchar *uri;
    GSList *l;
    LyricSearchEngine *engine;
    engine = lyric_search_get_tt_engine();
    uri = engine->get_engine_uri(&id);
    l = engine->parser(&id,lyric_func_get_contents(uri,0,NULL));
    lyric_func_lyricid_list(l);
    return 0;
}
#endif
