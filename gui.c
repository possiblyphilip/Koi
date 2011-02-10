/*
  Koi - a GIMP image authentication plugin
	Copyright (C) 2010  ben howard

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "clone.h"
#include "grain.h"
#include "texture.h"
#include "jpeg_compress.h"
#include "speckle.h"
#include "gui.h"

	#define NUM_PLUGINS 5
	KOI_PLUGIN* plugin[NUM_PLUGINS];

static gboolean koi_dialog (GimpDrawable *drawable)
{


	GtkWidget *dialog;
	GtkWidget *main_vbox;
	GtkWidget *main_hbox;

	GtkWidget *preview;

	GtkWidget *frame;
	GtkWidget *alignment;
	GtkWidget *vbox;
	GtkWidget *label;

	GtkWidget *thread_count_spinbutton;
	GtkWidget *notebook;

	int ii;



//	gui_options.threads = 4;
//	gui_options.texture_threshold = 140;
//	gui_options.clone_block_size = 16;
//	gui_options.histogram_block_size = 8;
//	gui_options.radius = 20;
//	gui_options.jpeg_threshold = 64;
//	gui_options.compress = .85;

	gboolean run;

	printf("######################################\n");
	printf("############## GUI init ##############\n");


	gimp_ui_init ("koi rebuild", FALSE);

	dialog = gimp_dialog_new ("Koi rebuld", "Koi rebuild", NULL, 0, gimp_standard_help_func, "Koi rebuild", GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	//create plugins and link them into the list
	create_clone_plugin();
	plugin[0] = &clone_plugin;

	create_grain_plugin();
	plugin[1] = &grain_plugin;

	create_texture_plugin();
	plugin[2] = &texture_plugin;

	create_jpeg_plugin();
	plugin[3] = &jpeg_plugin;

	create_speckle_plugin();
	plugin[4] = &speckle_plugin;

	main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
	gtk_widget_show (main_vbox);

	preview = gimp_drawable_preview_new (drawable, (gboolean *)TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
	gtk_widget_show (preview);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 6);

	alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 6, 6);

	main_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (main_hbox);
	gtk_container_add (GTK_CONTAINER (alignment), main_hbox);

//######################################3
//this is where im make my notebook tabs
	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
	gtk_widget_show (notebook);

	printf("creating tabs\n");

	for(ii = 0; ii< NUM_PLUGINS; ii++)
	{
		// should make a checkbox with its name will do that later
		//add the page to the notbook
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), plugin[ii]->create_gui(), plugin[ii]->label);
	}

	printf("all tabs created\n");

	gtk_box_pack_start (GTK_BOX (main_hbox), notebook, FALSE, FALSE, 6);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

	label = gtk_label_new ("Thread count");
	gtk_widget_show (label);

	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  //  thread_count_spinbutton = gimp_spin_button_new (&thread_count_spinbutton_value, gui_options.threads, 1, 8, 1, 1, 1, 1, 0);
 //   gtk_box_pack_start (GTK_BOX (vbox), thread_count_spinbutton, FALSE, FALSE, 0);
//    gtk_widget_show (thread_count_spinbutton);

	gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);

	g_signal_connect_swapped (preview, "invalidated",G_CALLBACK (koi), drawable);

	koi (drawable, GIMP_PREVIEW (preview));

//	g_signal_connect (clone_check_button, "clicked", G_CALLBACK (cb_clone_check_button), &gui_options);

//	g_signal_connect (block_size_spinbutton_value, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &gui_options.threads);

	gtk_widget_show (dialog);

	run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

	gtk_widget_destroy (dialog);

	return run;
}
