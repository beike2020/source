/******************************************************************************
 * Function: 	GTK use.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall gtk_base.c `pkg-config --cflags --libs gtk+-2.0` -o gtk_base
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <gnome.h>
#include <gtk/gtk.h>

GtkWidget *togglebutton;
GtkWidget *checkbutton;
GtkWidget *radiobutton1;
GtkWidget *radiobutton2;

const char *password = "secret";

enum {
	COLUMN_TITLE,
	COLUMN_ARTIST,
	COLUMN_CATALOGUE,
	N_COLUMNS
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf("1: show the text use gtk\n");
	printf("2: show the spin use gtk\n");
	printf("3: show the button use gtk\n");
	printf("4: show the tree use gtk\n");
	printf("5: show the menu use gtk\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void text_close(GtkWidget * window, gpointer data)
{
	gtk_main_quit();
}

void text_click(GtkWidget * button, gpointer data)
{
	const char *password_text =
	    gtk_entry_get_text(GTK_ENTRY((GtkWidget *) data));

	if (strcmp(password_text, password) == 0)
		printf("Access granted!\n");
	else
		printf("Access denied!\n");
}

int gtk_text(int argc, char *argv[])
{
	GtkWidget *window, *ok_button;
	GtkWidget *hbox1, *hbox2, *vbox;
	GtkWidget *username_label, *password_label;
	GtkWidget *username_entry, *password_entry;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "GtkEntryBox");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

	g_signal_connect(GTK_OBJECT(window), "destroy",
			 GTK_SIGNAL_FUNC(text_close), NULL);

	username_label = gtk_label_new("Login:");
	password_label = gtk_label_new("Password:");
	username_entry = gtk_entry_new();
	password_entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

	ok_button = gtk_button_new_with_label("Ok");
	g_signal_connect(GTK_OBJECT(ok_button), "clicked",
			 GTK_SIGNAL_FUNC(text_click), password_entry);

	hbox1 = gtk_hbox_new(TRUE, 5);
	hbox2 = gtk_hbox_new(TRUE, 5);
	vbox = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(hbox1), username_label, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox1), username_entry, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox2), password_label, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox2), password_entry, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), ok_button, FALSE, FALSE, 5);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

void spin_close(GtkWidget * window, gpointer data)
{
	gtk_main_quit();
}

int gtk_spin(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *spinbutton;
	GtkObject *adjustment;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
	g_signal_connect(GTK_OBJECT(window), "destroy",
			 GTK_SIGNAL_FUNC(spin_close), NULL);

	adjustment = gtk_adjustment_new(100.0, 50.0, 150.0, 0.5, 0.05, 0.05);
	spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.01, 2);

	gtk_container_add(GTK_CONTAINER(window), spinbutton);
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

void button_close(GtkWidget * window, gpointer data)
{
	gtk_main_quit();
}

void button_label(GtkContainer * box, gchar * caption, GtkWidget * widget)
{
	GtkWidget *label = gtk_label_new(caption);
	GtkWidget *hbox = gtk_hbox_new(TRUE, 4);

	gtk_container_add(GTK_CONTAINER(hbox), label);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_container_add(box, hbox);
}

void button_print(char *button_name, GtkToggleButton * button)
{
	gboolean active = gtk_toggle_button_get_active(button);
	printf("%s is %s\n", button_name, active ? "active" : "not active");
}

void button_clicked(GtkWidget * button, gpointer data)
{
	button_print("Checkbutton", GTK_TOGGLE_BUTTON(checkbutton));
	button_print("Togglebutton", GTK_TOGGLE_BUTTON(togglebutton));
	button_print("Radiobutton1", GTK_TOGGLE_BUTTON(radiobutton1));
	button_print("Radiobutton2", GTK_TOGGLE_BUTTON(radiobutton2));
	printf("\n");
}

int gtk_button(gint argc, gchar * argv[])
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *vbox;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
	g_signal_connect(GTK_OBJECT(window), "destroy",
			 GTK_SIGNAL_FUNC(button_close), NULL);

	button = gtk_button_new_with_label("Ok");
	togglebutton = gtk_toggle_button_new_with_label("Toggle");
	checkbutton = gtk_check_button_new();
	radiobutton1 = gtk_radio_button_new(NULL);
	radiobutton2 =
	    gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radiobutton1));

	vbox = gtk_vbox_new(TRUE, 4);
	button_label(GTK_CONTAINER(vbox), "ToggleButton:", togglebutton);
	button_label(GTK_CONTAINER(vbox), "CheckButton:", checkbutton);
	button_label(GTK_CONTAINER(vbox), "Radio 1:", radiobutton1);
	button_label(GTK_CONTAINER(vbox), "Radio 2:", radiobutton2);
	button_label(GTK_CONTAINER(vbox), "Button:", button);

	g_signal_connect(GTK_OBJECT(button), "clicked",
			 GTK_SIGNAL_FUNC(button_clicked), NULL);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

void tree_close(GtkWidget * window, gpointer data)
{
	gtk_main_quit();
}

int gtk_tree(int argc, char *argv[])
{
	GtkWidget *window;
	GtkTreeStore *store;
	GtkWidget *view;
	GtkTreeIter parent_iter, child_iter;
	GtkCellRenderer *renderer;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
	g_signal_connect(GTK_OBJECT(window), "destroy",
			 GTK_SIGNAL_FUNC(tree_close), NULL);

	store =
	    gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
			       G_TYPE_STRING);
	gtk_tree_store_append(store, &parent_iter, NULL);
	gtk_tree_store_set(store, &parent_iter, COLUMN_TITLE,
			   "Dark Side of the Moon", COLUMN_ARTIST, "Pink Floyd",
			   COLUMN_CATALOGUE, "B000024D4P", -1);
	gtk_tree_store_append(store, &child_iter, &parent_iter);
	gtk_tree_store_set(store, &child_iter, COLUMN_TITLE, "Speak to Me", -1);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
						    COLUMN_TITLE, "Title",
						    renderer, "text",
						    COLUMN_TITLE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
						    COLUMN_ARTIST, "Artist",
						    renderer, "text",
						    COLUMN_ARTIST, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
						    COLUMN_CATALOGUE,
						    "Catalogue", renderer,
						    "text", COLUMN_CATALOGUE,
						    NULL);
	gtk_container_add(GTK_CONTAINER(window), view);
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

void menu_close(GtkWidget * window, gpointer data)
{
	gtk_main_quit();
}

void menu_click(GtkWidget * widget, gpointer user_data)
{
	printf("Item Clicked!\n");
}

static GnomeUIInfo filemenu[] = {
	GNOMEUIINFO_MENU_NEW_ITEM("New", "Menu Hint", NULL, NULL),
	GNOMEUIINFO_MENU_OPEN_ITEM(NULL, NULL),
	GNOMEUIINFO_MENU_SAVE_AS_ITEM(NULL, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM(NULL, NULL),
	GNOMEUIINFO_END,
};

static GnomeUIInfo editmenu[] = {
	GNOMEUIINFO_MENU_FIND_ITEM(NULL, NULL),
	GNOMEUIINFO_END,
};

static GnomeUIInfo menubar[] = {
	GNOMEUIINFO_MENU_FILE_TREE(filemenu),
	GNOMEUIINFO_MENU_EDIT_TREE(editmenu),
	GNOMEUIINFO_END,
};

int gtk_menu(int argc, char *argv[])
{
	GtkWidget *app;

	gnome_program_init("gnome1", "0.1", LIBGNOMEUI_MODULE, argc, argv,
			   GNOME_PARAM_NONE);
	app = gnome_app_new("gnome1", "Menus, menus, menus");

	gtk_window_set_default_size(GTK_WINDOW(app), 300, 200);
	g_signal_connect(GTK_OBJECT(app), "destroy",
			 GTK_SIGNAL_FUNC(menu_close), NULL);
	gnome_app_create_menus(GNOME_APP(app), menubar);

	gtk_widget_show(app);
	gtk_main();

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//show the text use gtk
	case 1:
		gtk_text(argc, argv);
		break;

		//show the spin use gtk
	case 2:
		gtk_spin(argc, argv);
		break;

		//show the button use gtk
	case 3:
		gtk_button(argc, argv);
		break;

		//show the tree use gtk
	case 4:
		gtk_tree(argc, argv);
		break;

		//show the menu use gtk
	case 5:
		gtk_menu(argc, argv);
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
