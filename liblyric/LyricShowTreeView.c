
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "LyricShowTreeView.h"

#include "LyricShowIface.h"

struct _LSTreeViewPrivate
{
    const gchar   *label;
    guint64 time;
    gchar   *lyric;
};

#define LYIRC_SHOW_TREE_VIEW_GET_PRIVATE(o)       (G_TYPE_INSTANCE_GET_PRIVATE ((o),LYRIC_SHOW_TREE_VIEW_TYPE,LSTreeViewPrivate))

static void
lyric_show_iface_interface_init(LyricShowIface *iface);


G_DEFINE_TYPE_WITH_CODE (LyricShowTreeView, lyric_show_tree_view, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (LYRIC_TYPE_SHOW,
						lyric_show_iface_interface_init))

static void
lyric_show_tree_view_init(LyricShowTreeView *lstv)
{
    LSTreeViewPrivate *priv = LYIRC_SHOW_TREE_VIEW_GET_PRIVATE(lstv);
    priv->label = _("SeeSee");
}

static void
lyric_show_tree_view_class_init(LyricShowTreeViewClass *class)
{
    g_type_add_class_private(G_OBJECT_CLASS(class),sizeof(LSTreeViewPrivate));
}

static const gchar*
lyric_show_tree_view_label(LyricShow *iface)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);
    return lstv->priv->label;
}

static void
lyric_show_tree_view_set_time(LyricShow *iface,guint64 time)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);
    lstv->priv->time = time;
    g_printerr("set time : %llu\n",time);
}

static void
lyric_show_tree_view_set_lyric(LyricShow *iface,const gchar *lyric_file)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);

    g_free(lstv->priv->lyric);
    lstv->priv->lyric = g_strdup(lyric_file);
}

static void
lyric_show_iface_interface_init(LyricShowIface *iface)
{
    
    iface->label = lyric_show_tree_view_label;
    iface->set_time = lyric_show_tree_view_set_time;
    iface->set_lyric = lyric_show_tree_view_set_lyric;
}
