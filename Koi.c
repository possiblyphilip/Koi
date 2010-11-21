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

#include "Koi.h"
#include "gui.c"
#include "clone.c"
#include "texture.c"

MAIN()

static void query (void)
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

static void run (const char *name, int nparams, const GimpParam *param,  int *nreturn_vals, GimpParam **return_vals)
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
//	gimp_get_data ("plug-in-Koi", &gui_options);

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
//	    gui_options.radius = param[3].data.d_int32;
	break;

    case GIMP_RUN_WITH_LAST_VALS:
	/*  Get options last values if needed  */
	gimp_get_data ("plug-in-Koi", &gui_options);
	break;

    default:
	break;
    }

    koi (drawable, NULL);

    gimp_displays_flush ();
    gimp_drawable_detach (drawable);

    /*  Finally, set options in the core  */
    if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data ("plug-in-Koi", &gui_options, sizeof (GUI_values));

    return;
}

static void koi (GimpDrawable *drawable, GimpPreview  *preview)
{
    GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
    gint32 image_id, layer, new_layer, temp_image_id, temp_layer;
    int          row, col, channel, channels;
    int         start_colum, start_row, x2, y2;
    int num_return_vals;
    int ii;
    GimpPixelRgn rgn_in, rgn_out;
    int         width, height;

   // gchar file_name[250];
	gchar *file_name;
    
    gint *layer_array;
    gint num_layers;

    pthread_t thread_id[4];
    int thread_return_value[4];

    int threads = 4;

    guchar pixel[4];

    guchar ***in_array;
    guchar ***out_array;

    JOB_ARG job_args[4];

    if (! preview)
    {
	gimp_progress_init ("Koi...");
    }

 //   g_message("block_size %d\n",gui_options.clone_block_size);

    /* Allocate a big enough tile cache */
    gimp_tile_cache_ntiles (8 * (drawable->width / gimp_tile_width () + 1));

//    	image_id = gimp_drawable_get_image(drawable->drawable_id);
//    layer = gimp_image_get_active_layer(image_id);
//
//    g_message("%d layer id s", layer);

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


	layer_array = gimp_image_get_layers(image_id, &num_layers);

//		g_message("%d layer id", layer_array[num_layers]);
	
	layer = gimp_image_get_active_layer(image_id);
	if (layer == -1)
	{
	    return;
	}


	layer = layer_array[num_layers-1];

	/* Make a copy of the layer.  There's no error indicator? */
	new_layer = gimp_layer_copy(layer);
	/* Add the new layer to this image as the top layer */
	if (gimp_image_add_layer(image_id, new_layer, 0) != TRUE)
	{
	    g_message("failed to create layer");
	    return;
	}

	drawable->drawable_id = gimp_image_get_active_drawable(image_id);


    }

    if(width < 10 || height < 10)
    {
//i need to do something smarter here
//basically im just bailing out because the image data isnt worth looking at
	return;
    }

//make an array to hold all the pixels
    allocate_pixel_array(&in_array,width, height, 4);
    allocate_pixel_array(&out_array,width, height, 4);

    /* Initialises two PixelRgns, one to read original data,
   * and the other to write output data. That second one will
   * be merged at the end by the call to
   * gimp_drawable_merge_shadow() */
    gimp_pixel_rgn_init (&rgn_in, drawable, start_colum, start_row, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);

    if(gui_options.texture_checked == TRUE)
    {

	gimp_progress_set_text("filling secondary Koi array");

	for (row = 0; row < height; row++)
	{
	    for (col = 0; col < width; col++)
	    {
		gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

		for(channel = 0; channel < 4; channel++)
		{
		    out_array[col][row][channel] = pixel[channel];
		}
	    }

	    if (row % 10 == 0)
	    {
		gimp_progress_update ((gdouble) (row - start_row) / (gdouble) (x2 - start_colum));
	    }
	}

	gimp_run_procedure("gimp-desaturate",&num_return_vals, GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_END);
	gimp_run_procedure("plug-in-edge",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 9.99, GIMP_PDB_INT32, 2, GIMP_PDB_INT32, 5, GIMP_PDB_END);
    }

    channels = gimp_drawable_bpp (drawable->drawable_id);







//dump the gimp image data into my own array for processing
    gimp_progress_set_text("filling Koi array");
    if(gui_options.clone_checked || gui_options.texture_checked)
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

//	g_message("filled primary array");
//    }
//    else
//    {
//	for (row = 0; row < height; row++)
//	{
//	    for (col = 0; col < width; col++)
//	    {
//		gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);
//
//		for(channel = 0; channel < 4; channel++)
//		{
//		    in_array[col][row][channel] = pixel[channel];
//		}
//	    }
//	    if (row % 10 == 0)
//	    {
//		gimp_progress_update ((gdouble) (row - start_row) / (gdouble) (x2 - start_colum));
//	    }
//	}
//    }

//making sure i have the pointer hooked up to each copy of my  job arguments
    for(ii = 0; ii < threads; ii++)
    {
	job_args[ii].array_in = in_array;
	job_args[ii].array_out = out_array;
	job_args[ii].gui_options.texture_threshold = gui_options.texture_threshold;
	job_args[ii].gui_options.clone_block_size = gui_options.clone_block_size;

    }


//cut up and farm out the image job
//ill only kick off one thred when its the preview for now
    gimp_progress_set_text("Koi working");

//    if(preview)
//    {
//	job_args[0].start_colum = 0;
//	job_args[0].start_row = 0;
//	job_args[0].width = width;
//	job_args[0].height = height;
//
//	thread_return_value[0] = pthread_create((pthread_t*) &thread_id[0], NULL, find_blur_job, (void*)&job_args[0]);
//	if (thread_return_value[0] != 0)
//	{
////something bad happened
//	}
//    }
//    else
    if(gui_options.texture_checked == TRUE)
    {
	for(ii = 0; ii < threads; ii++)
	{
	    job_args[ii].start_colum = (width*ii) / threads;
	    job_args[ii].start_row = 0;
	    job_args[ii].width = (width / threads);
	    job_args[ii].height = height;

	    thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, find_blur_job, (void*)&job_args[ii]);
	    if (thread_return_value[ii] != 0)
	    {
		//something bad happened
	    }
	}
	//hang out and wait till all the threads are done
	for(ii = 0; ii < threads; ii++)
	{
	    thread_return_value[ii] = pthread_join(thread_id[ii], NULL);
	    if (thread_return_value[ii] != 0)
	    {
		//something bad happened
	    }
	}
    }



    if(gui_options.clone_checked == TRUE)
    {
	for(ii = 0; ii < threads; ii++)
	{
	    job_args[ii].start_colum = (width*ii) / threads;
	    job_args[ii].start_row = 0;
	    job_args[ii].width = (width / threads);
	    job_args[ii].height = height;

	    thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, find_clone_job, (void*)&job_args[ii]);
	    if (thread_return_value[ii] != 0)
	    {
		//something bad happened
	    }
	}
	//hang out and wait till all the threads are done
	for(ii = 0; ii < threads; ii++)
	{
	    thread_return_value[ii] = pthread_join(thread_id[ii], NULL);
	    if (thread_return_value[ii] != 0)
	    {
		//something bad happened
	    }
	}
    }

    if(gui_options.jpeg_checked == TRUE)
    {

	file_name = gimp_image_get_filename (image_id);

//	g_message("file name: %s\n", file_name);

//	printf("file name %s\n", file_name);

	gimp_run_procedure("file-jpeg-save",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, image_id , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_STRING, "/home/ben/programming/Koi/test_images/test.jpg", GIMP_PDB_STRING, "test", GIMP_PDB_FLOAT, .85, GIMP_PDB_FLOAT, 0.0, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 0, GIMP_PDB_STRING,"created with Koi", GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_END);

	// reload our saved image and suck a layer off of it to subtract against or original image


	temp_layer = gimp_file_load_layer(mode, image_id, "/home/ben/programming/Koi/test_images/test.jpg");

	gimp_layer_set_mode(temp_layer, 8);

	/* Add the new layer to this image as the top layer */
	if (gimp_image_add_layer(image_id, temp_layer, -1) != TRUE)
	{
	    g_message("failed to create layer");
	    return;
	}

	layer = gimp_image_get_active_layer(image_id);
	if (layer == -1)
	{
	    return;
	}

	gimp_image_merge_down(image_id, layer, 2);

	drawable->drawable_id = gimp_image_get_active_drawable(image_id);

	gimp_brightness_contrast(drawable->drawable_id, 127, 127);

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
    else if(gui_options.clone_checked || gui_options.texture_checked)
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

    free_pixel_array(in_array,width, height, 4);
    free_pixel_array(out_array,width, height, 4);

    /*  Update the modified region */
    if (preview)
    {
	gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),  &rgn_out);
    }
    else if(gui_options.clone_checked || gui_options.texture_checked)
    {
	gimp_drawable_flush (drawable);
	gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
	gimp_drawable_update (drawable->drawable_id, start_colum, start_row, width, height);

//	pixel[0] = 80;
//	pixel[0] = 190;
//	pixel[0] = 70;
//	pixel[0] = 0;


//	gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_COLOR, pixel[0], GIMP_PDB_END);
	if(gui_options.texture_checked == TRUE)
	{
	    gimp_run_procedure("plug-in-despeckle",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_INT32, 20, GIMP_PDB_INT32, 3, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 158,  GIMP_PDB_END);
	}
    }

//    gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_COLOR, (80, 190, 70), GIMP_PDB_END);

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
		arr[ii][jj][kk] = 255;
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





HSL RGBtoHSL( guchar r, guchar g, guchar b)
{

    HSL temp;

    double L = 0;
    double S = 0;
    double H = 0;

    double max_color = 0;

    double min_color = 0;

    double r_percent = ((double)r)/255;
    double g_percent = ((double)g)/255;
    double b_percent = ((double)b)/255;


    if((r_percent >= g_percent) && (r_percent >= b_percent))
    {
	max_color = r_percent;
    }

    if((g_percent >= r_percent) && (g_percent >= b_percent))

    {
	max_color = g_percent;
    }

    if((b_percent >= r_percent) && (b_percent >= g_percent))
    {

	max_color = b_percent;
    }

    if((r_percent <= g_percent) && (r_percent <= b_percent))
    {
	min_color = r_percent;
    }

    if((g_percent <= r_percent) && (g_percent <= b_percent))
    {
	min_color = g_percent;
    }

    if((b_percent <= r_percent) && (b_percent <= g_percent))
    {
	min_color = b_percent;
    }

    L = (max_color + min_color)/2;

    if(max_color == min_color)
    {
	S = 0;
	H = 0;
    }
    else
    {
	if(L < .50)
	{
	    S = (max_color - min_color)/(max_color + min_color);
	}
	else
	{
	    S = (max_color - min_color)/(2 - max_color - min_color);
	}

	if(max_color == r_percent)
	{
	    H = (g_percent - b_percent)/(max_color - min_color);
	}

	if(max_color == g_percent)
	{
	    H = 2 + (b_percent - r_percent)/(max_color - min_color);
	}

	if(max_color == b_percent)
	{
	    H = 4 + (r_percent - g_percent)/(max_color - min_color);
	}
    }

    temp.s = (int)(S*100);
    temp.l = (int)(L*100);

    H = H*60;
    if(H < 0)
    {
	H += 360;
    }

    temp.h  = (int)H;

    return temp;
}
