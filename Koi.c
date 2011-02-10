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



//    cat /proc/cpuinfo | grep -i "core"
//    cat /proc/cpuinfo | grep -i "cpu cores" | tail -n 1 | awk '{ print $4 }'


#include "Koi.h"

#include "gui.c"

#include "clone.h"
#include "clone.c"

#include "grain.h"
#include "grain.c"


#include "texture.h"
#include "texture.c"

#include "jpeg_compress.h"
#include "jpeg_compress.c"


#include "speckle.h"
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
//will have to do something different here
//		gimp_get_data ("plug-in-Koi", &gui_options);
		break;

	default:
		break;
	}

	koi (drawable, NULL);

	gimp_displays_flush ();
	gimp_drawable_detach (drawable);

//	need to do somthing different here
	/*  Finally, set options in the core  */
//    if (run_mode == GIMP_RUN_INTERACTIVE)
//		gimp_set_data ("plug-in-Koi-rebuild", &gui_options, sizeof (GUI_values));

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
	int ii, jj;
	GimpPixelRgn rgn_in, rgn_out;
	int         width, height;

	gchar *file_name;
	gint *layer_array;
	gint num_layers;

	float temp;

	pthread_t thread_id[NUM_THREADS];
	int thread_return_value[NUM_THREADS];

	JOB_ARG job[NUM_THREADS];

	guchar pixel[4];

	PIXEL **hidden_array;
	PIXEL **in_array;
	PIXEL **out_array;

	printf("############## starting Koi ##############\n");

	if (! preview)
	{
		gimp_progress_init ("Koi...");
	}
	else
	{
		printf("Koi preview mode\n");
	}

	/* Allocate a big enough tile cache */
	gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

	/* Gets upper left and lower right coordinates,
   * and layers number in the image */
	if (preview)
	{
		gimp_preview_get_position (preview, &start_colum, &start_row);
		gimp_preview_get_size (preview, &width, &height);
		x2 = start_colum + width;
		y2 = start_row + height;
//		threads = 1;
	}
	else
	{
		gimp_drawable_mask_bounds (drawable->drawable_id, &start_colum, &start_row, &x2, &y2);

		width = x2 - start_colum;
		height = y2 - start_row;
		//maybe this will add an alpha channel to the original so that the others have one ...


	}

	if(width < 10 || height < 10)
	{

		printf("width or height of image was too small width:%d height:%d\n",width, height);
		//i need to do something smarter here
		//basically im just bailing out because the image data isnt worth looking at
		return;
	}

	printf("building pixel arrays\n");
	//make an array to hold all the pixels

	allocate_pixel_array(&hidden_array,width, height);
	allocate_pixel_array(&in_array,width, height);
	allocate_pixel_array(&out_array,width, height);

	printf("pixel region init\n");
	/* Initialises two PixelRgns, one to read original data,
   * and the other to write output data. That second one will
   * be merged at the end by the call to
   * gimp_drawable_merge_shadow() */
	gimp_pixel_rgn_init (&rgn_in, drawable, start_colum, start_row, width, height, FALSE, FALSE);
//	gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);



	//i really dont use this any more as far as i know
	channels = gimp_drawable_bpp (drawable->drawable_id);

	//dump the gimp image data into my own array for processing
	printf("filling Koi array\n");
	gimp_progress_set_text("filling Koi array\n");

	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			gimp_pixel_rgn_get_pixel (&rgn_in, pixel, start_colum+col,start_row+row);

			hidden_array[col][row].red = pixel[0];
			hidden_array[col][row].green = pixel[1];
			hidden_array[col][row].blue = pixel[2];
		}

		if (row % 50 == 0)
		{
			gimp_progress_update ((gdouble) row / height);
		}
	}





	//cut up and farm out the image job
	//ill only kick off one thred when its the preview for now
	printf("Koi working\n");
	gimp_progress_set_text("Koi working\n");

	//################################################33
	for(jj = 0; jj < NUM_PLUGINS; jj++)
	{
		if(plugin[jj]->checked)
		{

			printf("copying Koi array\n");
			gimp_progress_set_text("copying Koi array\n");

			//copying the original to my output so i can have the original image as my output image with the blocks written over it
				for (row = 0; row < height; row++)
				{
					for (col = 0; col < width; col++)
					{
						in_array[col][row].red = hidden_array[col][row].red;
						in_array[col][row].green = hidden_array[col][row].green;
						in_array[col][row].blue = hidden_array[col][row].blue;
					}
				}

				for (row = 0; row < height; row++)
				{
					for (col = 0; col < width; col++)
					{
						out_array[col][row].red = 0;
						out_array[col][row].green = 0;
						out_array[col][row].blue = 0;
					}
				}


			gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);

			image_id = gimp_drawable_get_image(drawable->drawable_id);

			//all this is to make a copy of the bottom layer and stuffs it into another layer
			//for processing so i dont mess up the original image
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

//			new_layer = gimp_layer_new (image_id, plugin[jj]->name,width, height,GIMP_RGB_IMAGE,100.0,GIMP_NORMAL_MODE);

//			new_layer = gimp_layer_new_from_visible(image_id,image_id,plugin[jj]->name);

			//Add the new layer to this image as the top layer
			if (gimp_image_add_layer(image_id, new_layer, 0) != TRUE)
			{
				printf("failed to create layer\n");
				return;
			}
			//set the drawable to the top layer that we created for processing in
			drawable->drawable_id = gimp_image_get_active_drawable(image_id);

			gimp_drawable_set_name (drawable->drawable_id, plugin[jj]->name);

			printf("running %s\n",plugin[jj]->name);
			gimp_progress_set_text("running\n");

			//so i kick off a few threads for each algorithm
			for(ii = 0; ii < NUM_THREADS; ii++)
			{
				job[ii].array_in = in_array;
				job[ii].array_out = out_array;
				job[ii].start_colum = (width*ii) / NUM_THREADS;
				job[ii].start_row = 0;
				job[ii].width = (width / NUM_THREADS);
				job[ii].height = height;
				job[ii].image.height = height;
				job[ii].image.width = width;
				job[ii].drawable = drawable;
				job[ii].image_id = image_id;

				job[ii].thread = ii;
				job[ii].progress = 0;

				thread_return_value[ii] = pthread_create((pthread_t*) &thread_id[ii], NULL, (void *(*)(void *))plugin[jj]->algorithm, (void*)&job[ii]);
				if (thread_return_value[ii] != 0)
				{
					printf("thread %s failed to start\n",plugin[jj]->name);
					//something bad happened
				}
			}

			temp = 0;
			//this is a really dumb way of making each thread set its progress to 1 i.e. done
			while(temp != 1)
			{
				temp = 0;
				//sum up the progress for each thread and send that out to the gimp progress bar
				for(ii = 0; ii < NUM_THREADS; ii++)
				{
					temp += job[ii].progress;
				}
				temp /= NUM_THREADS;

				gimp_progress_update (temp);
//I need to look into a shorter sleep timer
				sleep(1);
			}
			//all the threads are allready done at this point but i wrap them up here
			for(ii = 0; ii < NUM_THREADS; ii++)
			{
				thread_return_value[ii] = pthread_join(thread_id[ii], NULL);
				if (thread_return_value[ii] != 0)
				{
					printf("thread %s returned %d badness\n",plugin[jj]->name, thread_return_value[ii]);
					//something bad happened
				}
			}

//			drawable->drawable_id = gimp_image_get_active_drawable(image_id);
//			gimp_pixel_rgn_init (&rgn_out, drawable,  start_colum, start_row, width, height, preview == NULL, TRUE);

			// write the array back to the out image here
			printf("exporting Koi array\n");
			gimp_progress_set_text ("exporting Koi array\n");
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
			else
			{
				for (row = 0; row < height; row++)
				{
					for (col = 0; col < width; col++)
					{
						pixel[0] = out_array[col][row].red;
						pixel[1] = out_array[col][row].green;
						pixel[2] = out_array[col][row].blue;
						gimp_pixel_rgn_set_pixel (&rgn_out, pixel,  col, row);
					}

					if (row % 50 == 0)
					{
					gimp_progress_update ((gdouble) (row / height));
					}
				}
printf("flushing drawable\n");
				gimp_drawable_flush (drawable);
				printf("merge shadow\n");
				gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
				printf("drawable update\n");
				gimp_drawable_update (drawable->drawable_id, start_colum, start_row, width, height);

			}
		}
	}

	printf("freeing pixel arrays\n");
	free_pixel_array(hidden_array,width);
	free_pixel_array(in_array,width);
	free_pixel_array(out_array,width);

	/*  Update the modified region */
	if (preview)
	{
		gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),  &rgn_out);
	}

	printf("Koi done\n");

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






