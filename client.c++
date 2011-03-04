#include <unistd.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <strings.h>

#include "local_book_manager.h"

#include <gtk/gtk.h>
static void destroy (GtkWidget*, gpointer);
static gboolean delete_event (GtkWidget*, GdkEvent*, gpointer);

#define single_instance (local_book_manager::instance())

int main(int argc, char **argv){
  local_book_manager::instance();

  int ii;
  char name[64];
  while(single_instance.read_book_catalogue(name, ii)!=-1){
      printf("%s - %d\n", name, ii);
  }
  //int jj, num;
  //char more[64];
  //while(single_instance.read_book_shelf(more, ii, jj, num)!=-1){
  //  printf("%s - %d %d %d\n", more, ii, jj, num);
  //}
  e_book *book= single_instance.get_book(1234,163,100);
  //int ret = book->go_absolute_page(9, name);
  int ret = book->go_absolute_page(0, name);
  g_print("fetch %s\n", name);

  GtkWidget *window, *label;
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
  /* Create a new GtkLabel widget that is selectable. */
  label = gtk_label_new ("Hello World");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  /* Add the label as a child widget of the window. */
  gtk_container_add (GTK_CONTAINER (window), label);
  gtk_widget_show_all (window);

  // main loop
  gtk_main ();
  return 0;
}

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

