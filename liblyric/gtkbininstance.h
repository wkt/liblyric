/**
* *gtkbininstance.h
* *
* *Copyright (c) 2012 Wei Keting
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifndef __GTK_BIN_INSTANCE_H_
#define __GTK_BIN_INSTANCE_H_

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS

#define GTK_BIN_INSTANCE_TYPE                  (gtk_bin_instance_get_type())
#define GTK_BIN_INSTANCE(o)                    (G_TYPE_CHECK_INSTANCE_CAST((o),GTK_BIN_INSTANCE_TYPE,GtkBinInstance))
#define GTK_BIN_INSTANCE_CLASS(o)              (G_TYPE_CHECK_CLASS_CAST((o),GTK_BIN_INSTANCE_TYPE,GtkBinInstanceClass))
#define GTK_BIN_INSTANCE_GET_CLASS(o)          (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_BIN_INSTANCE_TYPE, GtkBinInstanceClass))
#define IS_GTK_BIN_INSTANCE(o)                 (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_BIN_INSTANCE_TYPE))
#define IS_GTK_BIN_INSTANCE_CLASS(o)           (G_TYPE_CHECK_CLASS_TYPE ((o), GTK_BIN_INSTANCE_TYPE))


typedef struct _GtkBinInstance GtkBinInstance;
typedef struct _GtkBinInstanceClass GtkBinInstanceClass;
typedef struct _GtkBinInstancePrivate GtkBinInstancePrivate;

struct _GtkBinInstance
{
    GtkBin parent;

    GtkBinInstancePrivate *priv;
};

struct _GtkBinInstanceClass
{
    GtkBinClass parent_class;

};


GType
gtk_bin_instance_get_type(void);

void
gtk_bin_instance_set_increment_width(GtkBinInstance *bin,gint increment_width);

void
gtk_bin_instance_set_increment_height(GtkBinInstance *bin,gint increment_height);

GtkWidget*
gtk_bin_instance_new(void);

G_END_DECLS


#endif //__GTK_BIN_INSTANCE_H_
