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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <stdlib.h>

typedef struct
{
    int     radius;
    gboolean preview;
    gboolean texture_checked;
    gboolean clone_checked;
} GUI_values;

typedef struct
{
    guchar ***array_in;
    guchar ***array_out;
    int start_row;
    int start_colum;
    int height;
    int width;
}JOB_ARG;

static void query (void);
static void run (const guchar *name,  int nparams, const GimpParam  *param,  int *nreturn_vals, GimpParam **return_vals);

static void koi (GimpDrawable  *drawable, GimpPreview  *preview);


static gboolean koi_dialog (GimpDrawable *drawable);
static void texture_check_button_callback( GtkWidget *widget,  gpointer   data );
static void clone_check_button_callback( GtkWidget *widget,  gpointer   data );

void * find_blur_job(void *pArg);
void free_pixel_array(guchar ***array, int width, int height, int depth);
void allocate_pixel_array(guchar ****array, int width, int height, int depth);

/* Set up default values for options */
static GUI_values gvalues =
{
  3,  /* radius */
  1,   /* preview */
    0   /*check box? */
};

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    {
      GIMP_PDB_INT32,
      "run-mode",
      "Run mode"
    },
    {
      GIMP_PDB_IMAGE,
      "image",
      "Input image"
    },
    {
      GIMP_PDB_DRAWABLE,
      "drawable",
      "Input drawable"
    }
  };

  gimp_install_procedure (
    "plug-in-Koi",
    "Koi",
    "Highlights image forgery",
    "ben howard",
    "Copyright ben howard",
    "2010",
    "_Koi",
    "RGB*, GRAY*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);

  gimp_plugin_menu_register ("plug-in-Koi", "<Image>/Filters/Misc");
}

static void run (const guchar *name, int nparams, const GimpParam *param,  int *nreturn_vals, GimpParam **return_vals)
{
    static GimpParam  values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode       run_mode;
    GimpDrawable     *drawable;

    /* Setting mandatory output values */
    *nreturn_vals = 1;
    *return_vals  = values;

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;

    /* Getting run_mode - we won't display a dialog if
   * we are in NONINTERACTIVE mode */
    run_mode = param[0].data.d_int32;

    /*  Get the specified drawable  */
    drawable = gimp_drawable_get (param[2].data.d_drawable);

    switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
	/* Get options last values if needed */
	gimp_get_data ("plug-in-Koi", &gvalues);

	/* Display the dialog */
	if (! koi_dialog (drawable))
	{
	    g_message("could not open dialog\n");
	    return;
	}
	break;

    case GIMP_RUN_NONINTERACTIVE:
	if (nparams != 4)
	    status = GIMP_PDB_CALLING_ERROR;
	if (status == GIMP_PDB_SUCCESS)
	    gvalues.radius = param[3].data.d_int32;
	break;

    case GIMP_RUN_WITH_LAST_VALS:
	/*  Get options last values if needed  */
	gimp_get_data ("plug-in-Koi", &gvalues);
	break;

    default:
	break;
    }

    koi (drawable, NULL);

    gimp_displays_flush ();
    gimp_drawable_detach (drawable);

    /*  Finally, set options in the core  */
    if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data ("plug-in-Koi", &gvalues, sizeof (GUI_values));

    return;
}

static void koi (GimpDrawable *drawable, GimpPreview  *preview)
{
    GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
    gint32 image_id, layer, new_layer, top_layer;
    int          row, col, channel, channels;
    int         start_colum, start_row, x2, y2;
    int num_return_vals;
//    int         start_colum, start_row;
    int ii;
    GimpPixelRgn rgn_in, rgn_out;
    int         width, height;
    pthread_t thread_id[4];
    int rc[4];
    int threads = 4;

    guchar pixel[4];

    guchar ***in_array;
    guchar ***out_array;

    JOB_ARG job_args[4];

    if (! preview)
    {
	gimp_progress_init ("Koi...");
    }

    /* Gets upper left and lower right coordinates,
   * and layers number in the image */
    if (preview)
    {
	gimp_preview_get_position (preview, &start_colum, &start_row);
	gimp_preview_get_size (preview, &width, &height);
	x2 = start_colum + width;
	y2 = start_row + height;
	threads = 1;
    }
    else
    {
	gimp_drawable_mask_bounds (drawable->drawable_id, &start_colum, &start_row, &x2, &y2);

	width = x2 - start_colum;
	height = y2 - start_row;
//maybe this will add an alpha channel to the original so that the others have one ...

	image_id = gimp_drawable_get_image(drawable->drawable_id);

//	top_layer = gimp_image_get_active_layer( image_id);
//	gimp_layer_add_alpha( top_layer);


//all this is to make a copy of the image and stuff it into a layer for processing so i dont mess up the original image

	/* Get the active layer */
	layer = gimp_image_get_active_layer(image_id);
	if (layer == -1)
	{
	    return;
	}
	/* Make a copy of the layer.  There's no error indicator? */
	new_layer = gimp_layer_copy(layer);
	/* Add the new layer to this image as the top layer */
	if (gimp_image_add_layer(image_id, new_layer, 0) != TRUE)
	{

	    return;
	}

	drawable->drawable_id = gimp_image_get_active_drawable(image_id);

	gimp_run_procedure("gimp-desaturate",&num_return_vals, GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_END);
	gimp_run_procedure("plug-in-edge",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 9.99, GIMP_PDB_INT32, 2, GIMP_PDB_INT32, 5, GIMP_PDB_END);

    }

    channels = gimp_drawable_bpp (drawable->drawable_id);

    /* Allocate a big enough tile cache */
    gimp_tile_cache_ntiles (8 * (drawable->width / gimp_tile_width () + 1));

    /* Initialises two PixelRgns, one to read original data,
   * and the other to write output data. That second one will
   * be merged at the end by the call to
   * gimp_drawable_merge_shadow() */
    gimp_pixel_rgn_init (&rgn_in, drawable, start_colum, start_row, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);

    if(width < 10 || height < 10)
    {
//i need to do something smarter here
//basically im just bailing out because the image data isnt worth looking at
	return;
    }

//make an array to hold all the pixels
    allocate_pixel_array(&in_array,width, height, 4);
    allocate_pixel_array(&out_array,width, height, 4);

//dump the gimp image data into my own array for processing
    gimp_progress_set_text("filling Koi array");
    if (preview)
    {
	for (row = 0; row < height; row++)
	{
	    for (col = 0; col < width; col++)
	    {
		gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

		for(channel = 0; channel < 4; channel++)
		{
		    in_array[col][row][channel] = pixel[channel];
		}
	    }

	    if (row % 10 == 0)
	    {
		gimp_progress_update ((gdouble) (row - start_row) / (gdouble) (x2 - start_colum));
	    }
	}
    }
    else
    {
	for (row = 0; row < height; row++)
	{
	    for (col = 0; col < width; col++)
	    {
		gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

		for(channel = 0; channel < 4; channel++)
		{
		    in_array[col][row][channel] = pixel[channel];
		}
	    }
	    if (row % 10 == 0)
	    {
		gimp_progress_update ((gdouble) (row - start_row) / (gdouble) (x2 - start_colum));
	    }
	}
    }

//making sure i have the pointer hooked up to each copy of my  job arguments
    for(ii = 0; ii < threads; ii++)
    {
	job_args[ii].array_in = in_array;
	job_args[ii].array_out = out_array;
    }


//cut up and farm out the image job
//ill only kick off one thred when its the preview for now
    gimp_progress_set_text("Koi working");

    if(preview)
    {
	job_args[0].start_colum = 0;
	job_args[0].start_row = 0;
	job_args[0].width = width;
	job_args[0].height = height;

	rc[0] = pthread_create((pthread_t*) &thread_id[0], NULL, find_blur_job, (void*)&job_args[0]);
	if (rc[0] != 0)
	{
//something bad happened
	}
    }
    else
    {
	for(ii = 0; ii < threads; ii++)
	{
	    job_args[ii].start_colum = (width*ii) / threads;
	    job_args[ii].start_row = 0;
	    job_args[ii].width = (width / threads);
	    job_args[ii].height = height;

	    rc[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, find_blur_job, (void*)&job_args[ii]);
	    if (rc[ii] != 0)
	    {
		//something bad happened
	    }
	}
    }


//hang out and wait till all the threads are done
    for(ii = 0; ii < threads; ii++)
    {
	rc[ii] = pthread_join(thread_id[ii], NULL);
	if (rc[ii] != 0)
	{
//	     g_message("thread join failed\n");
	  //something bad happened
	}
    }

// write the array back to the out image here
    gimp_progress_set_text ("dumping Koi array");


    if(preview)
    {
	for (row = 0; row < height; row++)
	{
	    for (col = 0; col < width; col++)
	    {
		gimp_pixel_rgn_set_pixel (&rgn_out, out_array[col][row],  start_colum+col, start_row+row);
	    }
	}
    }
    else
    {
//this way the green should be clear ...
//	top_layer = gimp_run_procedure("gimp-image-get-active-layer",&num_return_vals, GIMP_PDB_IMAGE, image_id ,  GIMP_PDB_END);
//	top_layer = gimp_image_get_active_layer( image_id);
//	gimp_run_procedure("gimp-layer-add-alpha",&num_return_vals, GIMP_PDB_LAYER, top_layer ,  GIMP_PDB_END);

	for (row = 0; row < height; row++)
	{
	    for (col = 0; col < width; col++)
	    {
		gimp_pixel_rgn_set_pixel (&rgn_out, out_array[col][row],  col, row);
	    }

	    if (row % 10 == 0)
	    {
		gimp_progress_update ((gdouble) (row - start_row) / (gdouble) (x2 - start_colum));
	    }
	}
    }



//#################################3 free the array memory
    free_pixel_array(in_array,width, height, 4);
    free_pixel_array(out_array,width, height, 4);

    /*  Update the modified region */
    if (preview)
    {
	gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),  &rgn_out);
    }
    else
    {


	gimp_drawable_flush (drawable);
	gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
	gimp_drawable_update (drawable->drawable_id, start_colum, start_row, width, height);

	pixel[0] = 80;
	pixel[0] = 190;
	pixel[0] = 70;
	pixel[0] = 0;


//	gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_COLOR, pixel[0], GIMP_PDB_END);
	gimp_run_procedure("plug-in-despeckle",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_INT32, 5, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 255,  GIMP_PDB_END);

    }

//    gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_COLOR, (80, 190, 70), GIMP_PDB_END);

}

  //################################### blur job #######################3
void * find_blur_job(void *pArg)
{

    int size = 20;

    guchar slider[20];
    int ii;
    int counter = 0;
    guchar temp;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;

//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
	slider[ii] = 0;
    }

    int row, col, channel;

    for (row = 0; row < job_args->height ; row++)
    {
	for (col = job_args->start_colum; col < job_args->start_colum+job_args->width; col++)
	{
//set the current element in the slider to our newest pixel value
	    slider[counter] = job_args->array_in[col][row][0];
	    temp = 0;
//look through the slider to see if we have any bright spots
	    for(ii = 0; ii < size; ii++)
	    {
		if(slider[ii] > temp)
		{
		    temp = slider[ii];
		}
	    }
//set the color to red because its been messed with
	    if(temp < 110)
	    {
		job_args->array_out[col][row][0] = 255;
		job_args->array_out[col][row][1] = 0;
		job_args->array_out[col][row][2] = 0;
		job_args->array_out[col][row][3] = 255;
	    }
	    else
	    {
		job_args->array_out[col][row][0] = 80;
		job_args->array_out[col][row][1] = 190;
		job_args->array_out[col][row][2] = 70;
		job_args->array_out[col][row][3] = 0;
	    }

	    //this will reset my slider counter so i dont have to make a queue or anything slow like that
	    counter++;
	    counter%=size;

	}
    }
    //###################
// now do the same image block virtically
//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
	slider[ii] = 0;
    }


    for (col = job_args->start_colum; col < job_args->start_colum+job_args->width; col++)
    {
	for (row = 0; row < job_args->height ; row++)
	{

	    //set the current element in the slider to our newest pixel value
	    slider[counter] = job_args->array_in[col][row][0];
	    temp = 0;
	    //look through the slider to see if we have any bright spots
	    for(ii = 0; ii < size; ii++)
	    {
		if(slider[ii] > temp)
		{
		    temp = slider[ii];
		}
	    }
	    //set the color to red because its been messed with
	    if(temp < 110)
	    {
//if the pixel is already green we dont want to set it to red, we will set it to grey instead
		if(job_args->array_out[col][row][0] != 80)
		{
		    job_args->array_out[col][row][0] = 255;
		    job_args->array_out[col][row][1] = 0;
		    job_args->array_out[col][row][2] = 0;
		    job_args->array_out[col][row][3] = 255;
		}
		else
		{
		    job_args->array_out[col][row][0] = 30;
		    job_args->array_out[col][row][1] = 30;
		    job_args->array_out[col][row][2] = 30;
		    job_args->array_out[col][row][3] = 255;
		}
	    }
	    else
	    {
		job_args->array_out[col][row][0] = 80;
		job_args->array_out[col][row][1] = 190;
		job_args->array_out[col][row][2] = 70;
		job_args->array_out[col][row][3] = 0;
	    }

	    //this will reset my slider counter so i dont have to make a queue or anything slow like that
	    counter++;
	    counter%=size;

	}
    }

    return NULL;
}

  //################################### make array #######################
void allocate_pixel_array(guchar ****array, int width, int height, int depth)
{

    guchar ***arr;
    int ii;
    int jj;
    int kk;

    arr = (guchar ***)malloc (width * sizeof(guchar **));
    for (ii = 0; ii < width; ++ii)
    {
	arr[ii] =  (guchar **)malloc (height * sizeof(guchar *));
	for (jj = 0; jj < height; ++jj)
	{
	    arr[ii][jj]= (guchar *)malloc (depth * sizeof(guchar));
	    for (kk = 0; kk < depth; ++kk)
	    {
		arr[ii][jj][kk] = 0;
	    }
	}
    }

    *array = arr;

}

void free_pixel_array(guchar ***array, int width, int height, int depth)
{

    int ii;
    int jj;
    int kk;
//g_message("free array %p\n", array);
//free the pixel arrays
    for (ii = 0; ii < width; ii++)
    {
	for (jj = 0; jj < height; jj++)
	{
	    free(array[ii][jj]);
	}
	free(array[ii]);
    }
    free(array);
}

static gboolean
koi_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *radius_label;
  GtkWidget *alignment;
  GtkWidget *spinbutton;
  GtkObject *spinbutton_adj;
  GtkWidget *frame_label;

    GtkWidget *texture_check_button;
    GtkWidget *clone_check_button;

  gboolean   run;

  gimp_ui_init ("koi", FALSE);

  dialog = gimp_dialog_new ("Koi", "Koi",
			    NULL, 0,
			    gimp_standard_help_func, "Koi",

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OK,     GTK_RESPONSE_OK,

			    NULL);

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &gvalues.preview);
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

  //im going to try and put in my own boxes

  texture_check_button = gtk_check_button_new_with_label ( "Texture loss");
  gtk_widget_show (texture_check_button);
  gtk_box_pack_start (GTK_BOX (main_hbox), texture_check_button, FALSE, FALSE, 6);
  gtk_label_set_justify (GTK_LABEL (texture_check_button), GTK_JUSTIFY_RIGHT);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(texture_check_button), FALSE);


    clone_check_button = gtk_check_button_new_with_label ( "Cloneing");
    gtk_widget_show (clone_check_button);
    gtk_box_pack_start (GTK_BOX (main_hbox), clone_check_button, FALSE, FALSE, 6);
    gtk_label_set_justify (GTK_LABEL (clone_check_button), GTK_JUSTIFY_RIGHT);

      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clone_check_button), FALSE);

  //we will see how this runs

//  radius_label = gtk_label_new_with_mnemonic ("_Radius:");
//  gtk_widget_show (radius_label);
//  gtk_box_pack_start (GTK_BOX (main_hbox), radius_label, FALSE, FALSE, 6);
//  gtk_label_set_justify (GTK_LABEL (radius_label), GTK_JUSTIFY_RIGHT);
//
//  spinbutton = gimp_spin_button_new (&spinbutton_adj, gvalues.radius, 1, 32, 1, 1, 1, 5, 0);
//  gtk_box_pack_start (GTK_BOX (main_hbox), spinbutton, FALSE, FALSE, 0);
//  gtk_widget_show (spinbutton);

  frame_label = gtk_label_new ("<b>Modify radius</b>");
  gtk_widget_show (frame_label);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame_label);
  gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);




  g_signal_connect_swapped (preview, "invalidated",G_CALLBACK (koi), drawable);
 // g_signal_connect_swapped (spinbutton_adj, "value_changed",G_CALLBACK (gimp_preview_invalidate),  preview);

  koi (drawable, GIMP_PREVIEW (preview));

//  g_signal_connect (spinbutton_adj, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &gvalues.radius);




  g_signal_connect (texture_check_button, "clicked", G_CALLBACK (texture_check_button_callback), &gvalues);
   g_signal_connect (clone_check_button, "clicked", G_CALLBACK (clone_check_button_callback), &gvalues);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/* Our usual callback function */
static void texture_check_button_callback( GtkWidget *widget,  gpointer   data )
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
static void clone_check_button_callback( GtkWidget *widget,  gpointer   data )
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
