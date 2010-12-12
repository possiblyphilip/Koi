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
#include "dy_con.c"
#include "grain.c"
#include "histogram.c"
#include "speckle.c"

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
		// this doesnt do anything any more because i hard code the starting values
//			gimp_get_data ("plug-in-Koi", &gui_options);


//it still displays this if i hit the cancel button
//thats because it either runs or fails there is no "canceled" state
		/* Display the dialog */
		if (! koi_dialog (drawable))
		{
//			g_message("could not open dialog\n");
			return;
		}
		break;
//i need to work on this area for non interactive running
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
	gint32 image_id, layer, new_layer, temp_layer;
	int row, col;
	int channel, channels;
	int start_colum, start_row, x2, y2;
    int num_return_vals;
    int ii;
    GimpPixelRgn rgn_in, rgn_out;
    int         width, height;

	gchar *file_name;
    
    gint *layer_array;
    gint num_layers;

	float temp;

    int threads = gui_options.threads;

    pthread_t thread_id[threads];
    int thread_return_value[threads];

    guchar pixel[4];

	PIXEL **in_array;
	PIXEL **out_array;

    JOB_ARG job_args[4];

    if (! preview)
    {
		gimp_progress_init ("Koi...");
    }

    /* Allocate a big enough tile cache */
    gimp_tile_cache_ntiles (8 * (drawable->width / gimp_tile_width () + 1));

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


//all this is to make a copy of the bottom layer and stuffs it into another layer for processing so i dont mess up the original image
//get all the layers
		layer_array = gimp_image_get_layers(image_id, &num_layers);
		layer = gimp_image_get_active_layer(image_id);
		if (layer == -1)
		{
			printf("failed to get active layer\n");
			return;
		}

//set the layer i want to the bottom most layer
		layer = layer_array[num_layers-1];

//make a copy of the bottom most layer
		new_layer = gimp_layer_copy(layer);
//Add the new layer to this image as the top layer
		if (gimp_image_add_layer(image_id, new_layer, 0) != TRUE)
		{
			printf("failed to create layer\n");
			return;
		}
//set the drawable to the top layer that we created for processing in
		drawable->drawable_id = gimp_image_get_active_drawable(image_id);


    }

    if(width < 10 || height < 10)
    {

		printf("width or height of image was too small width:%d height:%d\n",width, height);
		//i need to do something smarter here
		//basically im just bailing out because the image data isnt worth looking at
		return;
    }

	//make an array to hold all the pixels
	allocate_pixel_array(&in_array,width, height);
	allocate_pixel_array(&out_array,width, height);

    /* Initialises two PixelRgns, one to read original data,
   * and the other to write output data. That second one will
   * be merged at the end by the call to
   * gimp_drawable_merge_shadow() */
    gimp_pixel_rgn_init (&rgn_in, drawable, start_colum, start_row, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);

//I fill this array
    if(gui_options.texture_checked == TRUE)
    {		
		gimp_progress_set_text("filling secondary Koi array\n");

		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{
				gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

				out_array[col][row].red = pixel[0];
				out_array[col][row].green = pixel[1];
				out_array[col][row].blue = pixel[2];
			}

			if (row % 50 == 0)
			{
				gimp_progress_update ((gdouble) row / height);
			}
		}

    }


//doing some pre processing of the image before i hand it off to my code
//will rewrite all the plugins my self for speed when i get time
    if(gui_options.texture_checked == TRUE )
    {
		gimp_run_procedure("gimp-desaturate",&num_return_vals, GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_END);
		gimp_run_procedure("plug-in-edge",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 9.99, GIMP_PDB_INT32, 2, GIMP_PDB_INT32, 5, GIMP_PDB_END);
		gimp_run_procedure("plug-in-gauss",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 1.0, GIMP_PDB_FLOAT, 1.0, GIMP_PDB_INT32, 1, GIMP_PDB_END);

    }

    if( gui_options.grain_checked == TRUE)
    {
		gimp_run_procedure("gimp-desaturate",&num_return_vals, GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_END);
		gimp_run_procedure("plug-in-edge",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 9.99, GIMP_PDB_INT32, 2, GIMP_PDB_INT32, 5, GIMP_PDB_END);


    }

//i really dont use this any more and i aught to re write my array with a type def instead of
//a dynamic array
    channels = gimp_drawable_bpp (drawable->drawable_id);

//dump the gimp image data into my own array for processing
	gimp_progress_set_text("filling Koi array\n");
    if(!gui_options.jpeg_checked)
    {
		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{
				gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

				in_array[col][row].red = pixel[0];
				in_array[col][row].green = pixel[1];
				in_array[col][row].blue = pixel[2];
			}

			if (row % 50 == 0)
			{
				gimp_progress_update ((gdouble) row / height);
			}
		}
    }

	//making sure i have the pointer hooked up to each copy of my  job arguments
    for(ii = 0; ii < threads; ii++)
    {
		job_args[ii].array_in = in_array;
		job_args[ii].array_out = out_array;

		job_args[ii].gui_options = &gui_options;		
    }



	//cut up and farm out the image job
	//ill only kick off one thred when its the preview for now
	gimp_progress_set_text("Koi working\n");

    //################################################33

    if(gui_options.speckle_checked == TRUE)
    {
		for(ii = 0; ii < threads; ii++)
		{
			job_args[ii].start_colum = (width*ii) / threads;
			job_args[ii].start_row = 0;
			job_args[ii].width = (width / threads);
			job_args[ii].height = height;

			thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, speckle, (void*)&job_args[ii]);
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

    //################################################33

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


	//################################################33
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
//this is a really dumb way of making each thread set its progress to 2 i.e. done
		while(temp != 1)
		{
			temp = 0;
//sum up the progress for each thread
			for(ii = 0; ii < threads; ii++)
			{
				temp += job_args[ii].progress;
			}
			temp /= threads;

			gimp_progress_update (temp);

			sleep(1);
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
	//################################################33

    if(gui_options.grain_checked == TRUE)
    {
		for(ii = 0; ii < threads; ii++)
		{
			job_args[ii].start_colum = (width*ii) / threads;
			job_args[ii].start_row = 0;
			job_args[ii].width = (width / threads);
			job_args[ii].height = height;

			thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, grain, (void*)&job_args[ii]);
			if (thread_return_value[ii] != 0)
			{
				//something bad happened
			}
		}

		//this is a really dumb way of making each thread set its progress to 2 i.e. done
		while(temp != 1)
		{
			temp = 0;
			//sum up the progress for each thread
			for(ii = 0; ii < threads; ii++)
			{
				temp += job_args[ii].progress;
			}
			temp /= threads;

			gimp_progress_update (temp);

			sleep(1);
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

	//################################################33

	if(gui_options.histogram_checked == TRUE)
	{

		for(ii = 0; ii < threads; ii++)
		{
			job_args[ii].start_colum = (width*ii) / threads;
			job_args[ii].start_row = 0;
			job_args[ii].width = (width / threads);
			job_args[ii].height = height;

			thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, histogram, (void*)&job_args[ii]);
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

	//################################################33
	if(gui_options.jpeg_checked == TRUE)
	{

		file_name = gimp_image_get_filename (image_id);

		gimp_run_procedure("file-jpeg-save",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, image_id , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_STRING, "/home/ben/programming/Koi/test_images/test.jpg", GIMP_PDB_STRING, "test", GIMP_PDB_FLOAT, gui_options.compress, GIMP_PDB_FLOAT, 0.0, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 0, GIMP_PDB_STRING,"created with Koi", GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_END);

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
		gimp_brightness_contrast(drawable->drawable_id, 126, 125);

		gimp_run_procedure("plug-in-gauss",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_FLOAT, 20.0, GIMP_PDB_FLOAT, 20.0, GIMP_PDB_INT32, 1, GIMP_PDB_END);

		printf("Jpeg threshold: %d\n",gui_options.jpeg_threshold);

		gimp_threshold(drawable->drawable_id, gui_options.jpeg_threshold,255 );


	}


	// write the array back to the out image here
	gimp_progress_set_text ("dumping Koi array\n");


    if(preview)
    {
		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{				
				pixel[0] = out_array[col][row].red;
				pixel[1] = out_array[col][row].green;
				pixel[2] = out_array[col][row].blue;

				gimp_pixel_rgn_set_pixel (&rgn_out, pixel,  start_colum+col, start_row+row);
			}
		}
    }
    else if(!gui_options.jpeg_checked)
    {
		//this way the green should be clear ...

		//I need to make this output and input multi threaded so it can run faster


		//	top_layer = gimp_run_procedure("gimp-image-get-active-layer",&num_return_vals, GIMP_PDB_IMAGE, image_id ,  GIMP_PDB_END);
		//	top_layer = gimp_image_get_active_layer( image_id);
		//	gimp_run_procedure("gimp-layer-add-alpha",&num_return_vals, GIMP_PDB_LAYER, top_layer ,  GIMP_PDB_END);

		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{
				pixel[0] = out_array[col][row].red;
				pixel[1] = out_array[col][row].green;
				pixel[2] = out_array[col][row].blue;

				gimp_pixel_rgn_set_pixel (&rgn_out, pixel,  start_colum+col, start_row+row);
			}

			if (row % 50 == 0)
			{
				gimp_progress_update ((gdouble) row / height);
			}
		}
    }

	free_pixel_array(in_array,width);
	free_pixel_array(out_array,width);

    /*  Update the modified region */
    if (preview)
    {
		gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),  &rgn_out);
    }
    else if(!gui_options.jpeg_checked)
    {
		gimp_drawable_flush (drawable);
		gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
		gimp_drawable_update (drawable->drawable_id, start_colum, start_row, width, height);
		//	gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_COLOR, pixel[0], GIMP_PDB_END);
		if(gui_options.texture_checked == TRUE)
		{
			gimp_run_procedure("plug-in-despeckle",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_PDB_INT32, 20, GIMP_PDB_INT32, 3, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 158,  GIMP_PDB_END);
		}
    }

	//    gimp_run_procedure("plug-in-colortoalpha",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, drawable->drawable_id, GIMP_COLOR, (80, 190, 70), GIMP_PDB_END);
	gimp_progress_end ();
}


//################################### make array #######################
void allocate_pixel_array(PIXEL ***array, int width, int height)
{
	PIXEL **arr;
    int ii;

	arr = (PIXEL **)malloc (width * sizeof(PIXEL *));
    for (ii = 0; ii < width; ++ii)
    {
		arr[ii] =  (PIXEL*)malloc (height * sizeof(PIXEL));
    }

	*array = arr;

}

void free_pixel_array(PIXEL **array, int width)
{

    int ii;

    for (ii = 0; ii < width; ii++)
    {
			free(array[ii]);
    }
    free(array);
}






