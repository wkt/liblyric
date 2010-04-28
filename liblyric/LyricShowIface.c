#include <gtk/gtk.h>

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
					       "NautilusView",
					       &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	
	return type;
}

const gchar *lyric_show_get_description(LyricShow *lsw)
{
    const gchar* dest = NULL;
    LyricShowIface *iface;

    iface = LYRIC_SHOW_GET_IFACE(lsw);

    g_return_val_if_fail (LYRIC_IS_SHOW(lsw),NULL);

    if(iface->description){
        dest = (*iface->description)(lsw);
    }else{
        g_critical ("LyricShow->description() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (activatable)));
    }
    return dest;
}

void lyric_show_set_time(LyricShow *lsw,guint64 time)
{
    LyricShowIface *iface;

    iface = LYRIC_SHOW_GET_IFACE(lsw);
    g_return_if_fail(LYRIC_IS_SHOW(lsw));
    if(iface->set_time){
        dest = (*iface->set_time)(lsw,time);
    }else{
        g_critical ("LyricShow->set_time() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (activatable)));
    }
}

void lyric_show_set_lyric(LyricShow *lsw,const gchar *lyric_file)
{
    iface = LYRIC_SHOW_GET_IFACE(lsw);
    g_return_if_fail(LYRIC_IS_SHOW(lsw));
    if(iface->set_lyric){
        dest = (*iface->set_lyric)(lsw,lyric_file);
    }else{
        g_critical ("LyricShow->set_lyric() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (activatable)));
    }
}
