

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "LyricDownloader.h"


struct _LyricDownloaderPriv
{
    struct
    {
        GPid   pid;
        gint   in;
        gint   out;
        gint   err;
        gchar  *out_data;
        gchar  *err_data;
    }loaddata;
    gint  status;
    gchar *uri;
    gchar *filename;
};

enum{
    SIGNAL_DONE,
    SIGNAL_ERROR,
    SIGNAL_LAST
};

guint lyric_down_loader_signals[SIGNAL_LAST] = {0};


#define LYRIC_DOWNLOADER_GET_PRIVATE(o)   (G_TYPE_INSTANCE_GET_PRIVATE ((o), LYRIC_DOWNLOADER_TYPE, LyricDownloaderPriv))

G_DEFINE_TYPE(LyricDownloader,lyric_down_loader,G_TYPE_OBJECT)


static void
lyric_down_loader_init(LyricDownloader *ldl)
{
    ldl->priv = LYRIC_DOWNLOADER_GET_PRIVATE(ldl);
    ldl->priv->uri = NULL;
    ldl->priv->filename = NULL;
}


static void
lyric_down_loader_class_init(LyricDownloaderClass *klass)
{

    lyric_down_loader_signals[SIGNAL_DONE] = 
        g_signal_new ("done",
            G_TYPE_FROM_CLASS(klass),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(LyricDownloaderClass,done),
            NULL,NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE,1,
            G_TYPE_STRING
            );

    lyric_down_loader_signals[SIGNAL_ERROR] = 
        g_signal_new ("error",
            G_TYPE_FROM_CLASS(klass),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(LyricDownloaderClass,error),
            NULL,NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE,1,
            G_TYPE_STRING
            );

    g_type_class_add_private (klass, sizeof (LyricDownloaderPriv));
}

static void
lyric_down_loader_gpid_clear(LyricDownloader *ldl)
{

    if(ldl->priv->loaddata.pid > 0){
        kill(ldl->priv->loaddata.pid,SIGTERM);
    }

    if(ldl->priv->loaddata.err_data){
        g_free(ldl->priv->loaddata.err_data);
        ldl->priv->loaddata.err_data = NULL;
    }
    if(ldl->priv->loaddata.out_data){
        g_free(ldl->priv->loaddata.out_data);
        ldl->priv->loaddata.out_data = NULL;
    }
    
}

static void
lyric_down_loader_clear(LyricDownloader *ldl)
{
    if(ldl->priv->uri){
        g_free(ldl->priv->uri);
        ldl->priv->uri = NULL;
    }
    if(ldl->priv->filename){
        g_free(ldl->priv->filename);
        ldl->priv->filename = NULL;
    }
    lyric_down_loader_gpid_clear(ldl);
}

static void
cmd_child_watch_callback(GPid pid,gint status,LyricDownloader *ldl)
{
    gchar *done_data = NULL;
    gchar *err_data = NULL;
    if(WIFEXITED(status)){
        if(WEXITSTATUS(status) == 0){
            ldl->priv->status = DONE_OK;
            done_data = ldl->priv->loaddata.out_data;
        }else{
            ldl->priv->status = DOWNLODER_FAILED;
            err_data = ldl->priv->loaddata.err_data;
            if(err_data == NULL){
                err_data = ldl->priv->loaddata.out_data;
            }
        }
    }else{
        ldl->priv->status = DOWNLODER_FAILED;
        err_data = _("command terminated unormally");
    }

    g_spawn_close_pid(ldl->priv->loaddata.pid);

    if(err_data){
        g_signal_emit(ldl,lyric_down_loader_signals[SIGNAL_ERROR],0,err_data);
    }

    g_signal_emit(ldl,lyric_down_loader_signals[SIGNAL_DONE],0,done_data);

    ldl->priv->loaddata.pid = 0;

    close(ldl->priv->loaddata.in);
    ldl->priv->loaddata.in = -1;

    close(ldl->priv->loaddata.out);
    ldl->priv->loaddata.out = -1;

    close(ldl->priv->loaddata.err);
    ldl->priv->loaddata.err = -1;

    lyric_down_loader_clear(ldl);
}

typedef struct
{
    LyricDownloader *ldl;
    gint fd;
}CmdThread;

static gpointer
cmd_get_data_thread(CmdThread *ctd)
{
    gchar *data = NULL;
    gchar buf[1026] = {0};
    ssize_t n = 0;
    GString *str = NULL;

    str = g_string_new("");
    while(1){
        n = read(ctd->fd,buf,sizeof(buf)-2);
        if(n > 0){
            buf[n]=0;
            buf[n+1]=0;
            str = g_string_append(str,buf);
        }else if (n == 0){
            if(str->len >1){
                data = g_strdup(str->str);
            }
            break;
        }else if (n < 0){
            break;
        }
    }

    g_string_free(str,TRUE);
    str = NULL;

    if (ctd->fd ==  ctd->ldl->priv->loaddata.out){
        ctd->ldl->priv->loaddata.out_data = data;
    }else if(ctd->fd == ctd->ldl->priv->loaddata.err){
        ctd->ldl->priv->loaddata.err_data = data;
    }
    g_free(ctd);
    return NULL;
}

static gboolean
cmd_create_data_thread(LyricDownloader *ldl,gint fd)
{
    GError *error = NULL;
    CmdThread *ctd;

    ctd = g_new0(CmdThread,1);
    ctd->ldl = ldl;
    ctd->fd = fd;
    g_thread_create((GThreadFunc)cmd_get_data_thread,ctd,FALSE,&error);
    if(error){
        ldl->priv->status = CREATE_THREAD_FAILED;

        g_signal_emit(ldl,lyric_down_loader_signals[SIGNAL_ERROR],0,error->message);
        g_error_free(error);
        error = NULL;

        g_free(ctd);
        ctd = NULL;
        return FALSE;
    }
    return TRUE;
}

void
lyric_down_loader_get_data(LyricDownloader *ldl,const gchar *uri)
{
    lyric_down_loader_clear(ldl);
    ldl->priv->uri = g_strdup(uri);
    GError *error = NULL;
    gchar *cmd[]={"/usr/bin/wget","-t1","-O","-",ldl->priv->uri,NULL};
    if(g_spawn_async_with_pipes(NULL,
                            cmd,NULL,
                            G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD,
                            NULL,NULL,
                            &ldl->priv->loaddata.pid,
                            NULL,
                            &ldl->priv->loaddata.out,
                            &ldl->priv->loaddata.err,
                            &error)){

        if(cmd_create_data_thread(ldl,ldl->priv->loaddata.out));
            cmd_create_data_thread(ldl,ldl->priv->loaddata.err);

        g_child_watch_add(ldl->priv->loaddata.pid,
                        (GChildWatchFunc)cmd_child_watch_callback,
                        ldl);
        
    }else{
        ldl->priv->status = RUN_CMDMAND_FAILD;
        g_signal_emit(ldl,lyric_down_loader_signals[SIGNAL_ERROR],0,error->message);
        g_error_free(error);
        error = NULL;
        g_signal_emit(ldl,lyric_down_loader_signals[SIGNAL_DONE],0,NULL);
    }
}

LyricDownloader*
lyric_down_loader_new(void)
{
    return LYRIC_DOWNLOADER(g_object_new(LYRIC_DOWNLOADER_TYPE,NULL));
}

#if 1

static void
done_callback(LyricDownloader*ldl,const gchar *data)
{
    g_printerr("%s\n",data);
}

static void
error_callback(LyricDownloader*ldl,const gchar *message)
{
    g_printerr("55555555 .....\nSomething is wrong:\n%s\n",message);
}

int main(int argc,char **argv)
{
    GMainLoop *loop = NULL;
    LyricDownloader* ldl = NULL;

    g_type_init();
    loop = g_main_loop_new(NULL,FALSE);
    ldl = lyric_down_loader_new();
    g_signal_connect(ldl,"done",G_CALLBACK(done_callback),NULL);
    g_signal_connect(ldl,"error",G_CALLBACK(error_callback),NULL);
    lyric_down_loader_get_data(ldl,argv[1]);
    g_main_loop_run(loop);
    return 0;
}

#endif
