#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <gtk/gtk.h>
#include "LyricShow.h"

#include "common-glue.h"

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

enum
{
    TIME_REQUEST,
    SEARCH_REQUEST,
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
                 lyric_show_common_marshal_VOID__INT64,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_INT64);

    lyric_show_signals[SEARCH_REQUEST]=
        g_signal_new("search-request",
                 LYRIC_TYPE_SHOW,
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(LyricShowIface,search_request),
                 NULL,NULL,
                 lyric_show_common_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_object_interface_install_property (g_class,
				       g_param_spec_boolean ("time-requestable",
							     N_("time-requestable"),
							     N_("control emit time-request or not"),
							     TRUE,
							     G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

    g_object_interface_install_property(g_class,
                                g_param_spec_int64("time",
                                                    "time",
                                                    "time",
                                                    G_MININT64,
                                                    G_MAXINT64,
                                                    0,
                                                    G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

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

void lyric_show_set_time(LyricShow *lsw,gint64 time)
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
lyric_show_time_request(LyricShow *lsw,gint64 t)
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

void
lyric_show_search_request(LyricShow *lsw)
{
    g_return_if_fail(lsw != NULL && LYRIC_IS_SHOW(lsw));

    g_signal_emit(lsw,lyric_show_signals[SEARCH_REQUEST],0);
}
