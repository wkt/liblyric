#include <gtk/gtk.h>
#include "LyricShow.h"

static void
lyric_show_base_init (gpointer g_class)
{
    
}

GType                   
lyric_show_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (LyricShowIface),
			lyric_show_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "LyricShow",
					       &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	
	return type;
}

const gchar *lyric_show_get_name(LyricShow *lsw)
{
    const gchar* dest = NULL;
    LyricShowIface *iface;

    iface = LYRIC_SHOW_GET_IFACE(lsw);

    g_return_val_if_fail (LYRIC_IS_SHOW(lsw),NULL);

    if(iface->get_name){
        dest = (*iface->get_name)(lsw);
    }else{
        g_critical ("LyricShow->label() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (lsw)));
    }
    return dest;
}

void lyric_show_set_time(LyricShow *lsw,guint64 time)
{
    LyricShowIface *iface;

    iface = LYRIC_SHOW_GET_IFACE(lsw);
    g_return_if_fail(LYRIC_IS_SHOW(lsw));
    if(iface->set_time){
        (*iface->set_time)(lsw,time);
    }else{
        g_critical ("LyricShow->set_time() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (lsw)));
    }
}

void lyric_show_set_lyric(LyricShow *lsw,const gchar *lyric_file)
{
    LyricShowIface *iface;
    iface = LYRIC_SHOW_GET_IFACE(lsw);
    g_return_if_fail(LYRIC_IS_SHOW(lsw));
    if(iface->set_lyric){
        (*iface->set_lyric)(lsw,lyric_file);
    }else{
        g_critical ("LyricShow->set_lyric() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (lsw)));
    }
}

void
lyric_show_set_text(LyricShow *lsw,const gchar *text)
{
    LyricShowIface *iface;
    iface = LYRIC_SHOW_GET_IFACE(lsw);
    g_return_if_fail(LYRIC_IS_SHOW(lsw));
    if(iface->set_text){
        (*iface->set_text)(lsw,text);
    }else{
        g_critical ("LyricShow->set_text() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (lsw)));
    }
}
