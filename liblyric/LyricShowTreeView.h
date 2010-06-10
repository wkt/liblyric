#ifndef LYRIC_SHOW_TREE_VIEW_H

#define LYRIC_SHOW_TREE_VIEW_H

#define LYRIC_SHOW_TREE_VIEW_TYPE                 (lyric_show_tree_view_get_type ())
#define LYRIC_SHOW_TREE_VIEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), LYRIC_SHOW_TREE_VIEW_TYPE, LyricShowTreeView))
#define LYRIC_SHOW_TREE_VIEW_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), LYRIC_SHOW_TREE_VIEW_TYPE, LyricShowTreeViewClass))
#define IS_LYRIC_SHOW_TREE_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LYRIC_SHOW_TREE_VIEW_TYPE))
#define IS_LYRIC_SHOW_TREE_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), LYRIC_SHOW_TREE_VIEW_TYPE))
#define LYRIC_SHOW_TREE_VIEW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), LYRIC_SHOW_TREE_VIEW_TYPE, LyricShowTreeViewClass))

typedef struct _LyricShowTreeView LyricShowTreeView;
typedef struct _LyricShowTreeViewClass LyricShowTreeViewClass;
typedef struct _LSTreeViewPrivate LSTreeViewPrivate;

struct _LyricShowTreeView
{
    GtkTreeView      parent;
    GtkListStore     *store;
    GtkCellRenderer  *cell;

    LSTreeViewPrivate *priv;
};

struct _LyricShowTreeViewClass
{
    GtkTreeViewClass parent_class;
};

GType
lyric_show_tree_view_get_type(void);


#endif ///LYRIC_SHOW_TREE_VIEW_H
