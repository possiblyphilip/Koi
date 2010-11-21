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

#include "gui.h"

static gboolean koi_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *preview;
  GtkWidget *frame;

  GtkWidget *alignment;
  GtkWidget *spinbutton;
  GtkWidget *tab_box;
    GtkWidget *h_box;
  GtkWidget *block_size_spinbutton;

  GtkWidget *texture_hscale;
  GtkWidget *notebook;

  GtkObject *texture_threshold_value;

  GtkObject *block_size_spinbutton_value;
  GtkWidget *jpeg_check_button;
    GtkWidget *texture_check_button;
    GtkWidget *clone_check_button;

     GtkWidget *label;

    int i;
       char bufferf[32];
       char bufferl[32];

  gboolean   run;

  gimp_ui_init ("koi", FALSE);

  dialog = gimp_dialog_new ("Koi", "Koi", NULL, 0, gimp_standard_help_func, "Koi", GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &gui_options.preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(preview), FALSE);

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


  //this is where im going to try and make my notebook tabs

  //first i make the notbook

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  //gtk_table_attach_defaults(GTK_TABLE(table), notebook, 0,6,0,1);
  gtk_widget_show (notebook);


  //and then i make some pages or frames to shove into there i think
  //so this is the page

  label = gtk_label_new ("Texture");

  tab_box = gtk_vbox_new (FALSE, 6);

 gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
 gtk_widget_set_size_request (tab_box, 200, 100);
 gtk_widget_show (tab_box);
 //this is the button i want to add to the page
 texture_check_button = gtk_check_button_new_with_label ( "Find Texture Loss");
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(texture_check_button), FALSE);
   gtk_widget_set_size_request (texture_check_button, 100, 40);
 gtk_widget_show (texture_check_button);
 //i add the button to the page
 gtk_container_add (GTK_CONTAINER (tab_box), texture_check_button);
 //then add the page to the notbook
// gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);

  /* value, lower, upper, step_increment, page_increment, page_size */
     /* Note that the page_size value only makes a difference for
      * scrollbar widgets, and the highest value you'll get is actually
      * (upper - page_size). */
  //    texture_threshold_value = gtk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);


  texture_threshold_value = gtk_adjustment_new (90, 0, 256, 1, 1, 1);
  texture_hscale = gtk_hscale_new (GTK_ADJUSTMENT (texture_threshold_value));
  gtk_scale_set_digits( GTK_SCALE(texture_hscale), 3);
//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
  gtk_widget_set_size_request (texture_hscale, 100, 40);
	 gtk_widget_show (texture_hscale);

gtk_container_add (GTK_CONTAINER (tab_box), texture_hscale);


  //then add the page to the notbook

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);

    label = gtk_label_new ("Clone");

  //so this is the page
//  frame = gtk_frame_new ("Clone tool use");
   tab_box = gtk_vbox_new (FALSE, 6);

  gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
  gtk_widget_set_size_request (tab_box, 200, 75);
  gtk_widget_show (tab_box);
  //this is the button i want to add to the page
  clone_check_button = gtk_check_button_new_with_label ( "Find Cloneing");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clone_check_button), FALSE);
  gtk_widget_show (clone_check_button);
  //i add the button to the page
  gtk_container_add (GTK_CONTAINER (tab_box), clone_check_button);


  block_size_spinbutton = gimp_spin_button_new (&block_size_spinbutton_value, gui_options.clone_block_size, 4, 40, 4, 4, 4, 4, 0);
  gtk_container_add (GTK_CONTAINER (tab_box), block_size_spinbutton);
  gtk_widget_show (block_size_spinbutton);

//  block_size_value = gtk_adjustment_new (90, 0, 256, 1, 1, 1);
//  block_size_hscale = gtk_hscale_new (GTK_ADJUSTMENT (block_size_value));
//  gtk_scale_set_digits( GTK_SCALE(texture_hscale), 3);
////  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
//  gtk_widget_set_size_request (texture_hscale, 100, 40);
//	 gtk_widget_show (texture_hscale);

gtk_container_add (GTK_CONTAINER (tab_box), texture_hscale);

  //then add the page to the notbook
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);


  label = gtk_label_new ("Jpeg");

//so this is the page
//  frame = gtk_frame_new ("Clone tool use");
 tab_box = gtk_vbox_new (FALSE, 6);

gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
gtk_widget_set_size_request (tab_box, 200, 75);
gtk_widget_show (tab_box);
//this is the button i want to add to the page
jpeg_check_button = gtk_check_button_new_with_label ( "Find Jpeg Age");
gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(jpeg_check_button), FALSE);
gtk_widget_show (jpeg_check_button);
//i add the button to the page
gtk_container_add (GTK_CONTAINER (tab_box), jpeg_check_button);
//then add the page to the notbook
gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);




//
//  clone_check_button = gtk_check_button_new_with_label ( "Cloneing");
//  gtk_widget_show (clone_check_button);
//  gtk_box_pack_start (GTK_BOX (main_hbox), clone_check_button, FALSE, FALSE, 6);
//  gtk_label_set_justify (GTK_LABEL (clone_check_button), GTK_JUSTIFY_RIGHT);
//
//  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clone_check_button), FALSE);






  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, FALSE, FALSE, 6);




 // gtk_box_pack_start (GTK_BOX (box2), hscale, TRUE, TRUE, 0);
 //  gtk_box_pack_start (GTK_BOX (main_hbox), hscale, TRUE, TRUE, 6


  //we will see how this runs

  //  radius_label = gtk_label_new_with_mnemonic ("_Radius:");
  //  gtk_widget_show (radius_label);
  //  gtk_box_pack_start (GTK_BOX (main_hbox), radius_label, FALSE, FALSE, 6);
  //  gtk_label_set_justify (GTK_LABEL (radius_label), GTK_JUSTIFY_RIGHT);
  //
  //  spinbutton = gimp_spin_button_new (&spinbutton_adj, gui_options.radius, 1, 32, 1, 1, 1, 5, 0);
  //  gtk_box_pack_start (GTK_BOX (main_hbox), spinbutton, FALSE, FALSE, 0);
  //  gtk_widget_show (spinbutton);

//  frame_label = gtk_label_new ("<b>Modify radius</b>");
//  gtk_widget_show (frame_label);
//  gtk_frame_set_label_widget (GTK_FRAME (frame), frame_label);
//  gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);




  g_signal_connect_swapped (preview, "invalidated",G_CALLBACK (koi), drawable);
  // g_signal_connect_swapped (spinbutton_adj, "value_changed",G_CALLBACK (gimp_preview_invalidate),  preview);

  koi (drawable, GIMP_PREVIEW (preview));

  //  g_signal_connect (spinbutton_adj, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &gui_options.radius);




  g_signal_connect (texture_check_button, "clicked", G_CALLBACK (cb_texture_check_button), &gui_options);
  g_signal_connect (clone_check_button, "clicked", G_CALLBACK (cb_clone_check_button), &gui_options);
    g_signal_connect (jpeg_check_button, "clicked", G_CALLBACK (cb_jpeg_check_button), &gui_options);


  gtk_signal_connect (GTK_OBJECT (texture_threshold_value), "value_changed", GTK_SIGNAL_FUNC (cb_texture_hscale), &gui_options);

  g_signal_connect (block_size_spinbutton_value, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &gui_options.clone_block_size);


  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/* Our usual callback function */
static void cb_texture_hscale( GtkAdjustment *adj,  gpointer   data )
{
    GUI_values *temp_vals;
    temp_vals = (GUI_values *)data;

    temp_vals->texture_threshold = gtk_adjustment_get_value(adj);
//    temp_vals->texture_threshold = gtk_adjustment_set_value(adj, adj->value);

}

/* Our usual callback function */
static void cb_jpeg_check_button( GtkWidget *widget,  gpointer   data )
{
    GUI_values *temp_vals;
    temp_vals = (GUI_values *)data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
    {
	temp_vals->jpeg_checked = TRUE;
    }
    else
    {
	temp_vals->jpeg_checked = FALSE;
    }
}

/* Our usual callback function */
static void cb_texture_check_button( GtkWidget *widget,  gpointer   data )
{
    GUI_values *temp_vals;
    temp_vals = (GUI_values *)data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
    {
	temp_vals->texture_checked = TRUE;
    }
    else
    {
	temp_vals->texture_checked = FALSE;
    }
}

/* Our usual callback function */
static void cb_clone_check_button( GtkWidget *widget,  gpointer   data )
{
    GUI_values *temp_vals;
    temp_vals = (GUI_values *)data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
    {
	temp_vals->clone_checked = TRUE;
    }
    else
    {
	temp_vals->clone_checked = FALSE;
    }
}
