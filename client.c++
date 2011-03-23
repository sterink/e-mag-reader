#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <strings.h>

#include "local_book_manager.h"

#include <gtk/gtk.h>

/* Stop the GTK+ main loop function when the window is destroyed. */
static void destroy (GtkWidget *window, gpointer data)
{
  gtk_main_quit ();
}
/* Return FALSE to destroy the widget. By returning TRUE, you can cancel
 * a delete-event. This can be used to confirm quitting the application. */
static gboolean delete_event (GtkWidget *window,
    GdkEvent *event, gpointer data)
{
  return FALSE;
}

static gboolean iochannel_read (GIOChannel *channel,
    GIOCondition condition,
    GtkEntry *entry)
{
  GIOStatus ret_value;
  gchar *message;
  gsize length;
  /* The pipe has died unexpectedly, so exit the application. */
  if (condition & G_IO_HUP)
    g_error ("Error: The pipe has died!\n");
  /* Read the data that has been sent through the pipe. */
  ret_value = g_io_channel_read_line (channel, &message, &length, NULL, NULL);
  if (ret_value == G_IO_STATUS_ERROR)
    g_error ("Error: The line could not be read!\n");

  message[length-1] = 0;
  gtk_entry_set_text (entry, message);
  return TRUE;
}

#define single_instance (local_book_manager::instance())
int notify_fd = -1;
int main(int argc, char **argv){
  int fd[2];
  int r = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
  notify_fd = fd[0];

  int index = 0;
  int total = 12;
  int bb[total];
  int ii;
  char name[64];
  while(single_instance->read_book_catalogue(name, ii)!=-1){
      printf("%s - %d~~\n", name, ii);
      bb[index++] = ii;
  }
  int jj, num;
  char more[64];
  for(int i=0;i<index;i++){
    ii = bb[i];
    while(single_instance->read_book_shelf(more, ii, jj, num)!=-1){
      printf("^^^%s - %d %d %d\n", more, ii, jj, num);
    }
  }
  e_book *book= single_instance->get_book(1234,163,100);
  //int ret = book->go_absolute_page(9, name);
  int ret = book->go_absolute_page(0, name);
  g_print("fetch %s\n", name);

  GtkWidget *window, *entry;
  // init gui
  gtk_init (&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (window), "Hello World!");
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_widget_set_size_request (window, 200, 100);
  /* Connect the main window to the destroy and delete-event signals. */
  g_signal_connect (G_OBJECT (window), "destroy",
      G_CALLBACK (destroy), NULL);
  g_signal_connect (G_OBJECT (window), "delete_event",
      G_CALLBACK (delete_event), NULL);

  entry = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (window), entry);

  gtk_widget_show_all (window);

  GIOChannel* channel = g_io_channel_unix_new(fd[1]);
  guint source = g_io_add_watch(channel, G_IO_IN, (GIOFunc) iochannel_read, (gpointer) entry);
  // main loop
  gtk_main ();
  return 0;
}

