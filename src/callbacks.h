#include <gtk/gtk.h>

void log_txt(gchar *);
gboolean add_node(gchar *);
gboolean nodes_rm(gchar *);
gboolean exist_node(gchar *);

gboolean
on_w_main_destroy_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_w_main_destroy                      (GtkObject       *object,
                                        gpointer         user_data);

void
on_exit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_node_list_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_node_add_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_node_rm_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_hb_list_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_hb_add_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_hb_rm_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_serv_list_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_serv_add_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_serv_rm_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_help_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_nodes_list_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button4_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button5_clicked                     (GtkButton       *button,
                                        gpointer         user_data);




void
on_button6_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_hb_add_net_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_hb_add_disk_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button9_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button10_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button11_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button12_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button13_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button14_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button16_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button17_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_srv_add_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_srv_list_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_srv_rm_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button18_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button19_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button20_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button21_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_clist4_select_row                   (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);
