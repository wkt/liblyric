#include <gtk/gtk.h>
#include "LyricShow.h"

#include "common-glue.h"

enum
{
    TIME_REQUEST,
    SIGNAL_LAST
};

static guint lyric_show_signals[SIGNAL_LAST] = {0};

static void
lyric_show_base_init (gpointer g_class)
{
    lyric_show_signals[TIME_REQUEST]=
        g_signal_new("time-request",
                 LYRIC_TYPE_SHOW,
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(LyricShowIface,time_request),
                 NULL,NULL,
                 lyric_show_common_marshal_VOID__UINT64,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_UINT64);
}

GType                   
lyric_show_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (LyricShowIface),
			NULL,
			NULL,
			lyric_show_base_init,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "LyricShowIface",
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

void
lyric_show_time_request(LyricShow *lsw,guint64 t)
{
    g_return_if_fail(lsw != NULL && LYRIC_IS_SHOW(lsw));

    g_signal_emit(lsw,lyric_show_signals[TIME_REQUEST],0,t);
/*
    LyricShowIface *iface;
    iface = LYRIC_SHOW_GET_IFACE(lsw);

    if(iface->time_request){
        (*iface->time_request)(lsw,t);
    }else{
        g_critical ("LyricShow->time_request() unimplemented for type %s", 
                    g_type_name (G_OBJECT_TYPE (lsw)));
    }
*/
}
