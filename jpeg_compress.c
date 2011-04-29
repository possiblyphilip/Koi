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
#include"jpeg_compress.h"

float jpeg_compress = .96;
int jpeg_threshold = 60;

#define SLEEP_TIME 2

pthread_cond_t      jpeg_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     jpeg_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int jpeg_wait = 1;


void * jpeg_highlighter_algorithm(JOB_ARG *job)
{
	GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
	int num_return_vals;
	gint32 layer, temp_layer;
	char temp_file_name[256];
	int ii;

	printf("inside %s thread %d\n", jpeg_plugin.name, job->thread);

// i really only want to run this once and the plugin doesnt know how many threads will get kicked off because its dynamic
	if(job->thread == 0)
	{

		sleep(1);


		sprintf(temp_file_name,"%stemp.jpg",job->file_name);


//		mkstemp(file_name);
		printf("using filename %s\n", temp_file_name);


		printf("saving jpeg at %f compression\n", jpeg_compress);
		gimp_progress_set_text("waiting for jpeg save\n");

		gimp_run_procedure("file-jpeg-save",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, job->image_id , GIMP_PDB_DRAWABLE, job->drawable->drawable_id, GIMP_PDB_STRING, temp_file_name, GIMP_PDB_STRING, "temp", GIMP_PDB_FLOAT, jpeg_compress, GIMP_PDB_FLOAT, 0.0, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 0, GIMP_PDB_STRING,"created with Koi", GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_END);
//		for(ii = 0; ii < SLEEP_TIME; ii++)
//		{
//			job->progress = ((float)ii/SLEEP_TIME) * 4;
			sleep(1);
//		}



		printf("saved jpeg\n");
//		sleep(1);
		// reload our saved image and suck a layer off of it to subtract against or original image
		temp_layer = gimp_file_load_layer(mode, job->image_id, temp_file_name);

		printf("loaded new layer %d in image %d\n", temp_layer, job->image_id);

		//gimp_layer_add_alpha(temp_layer);

		gimp_layer_set_mode(temp_layer, 8);

		printf("set layer mode %d\n", temp_layer);

		/* Add the new layer to this image as the top layer */
		if (gimp_image_add_layer(job->image_id, temp_layer, -1) != TRUE)
		{
			printf("failed to create layer\n");
			return;
		}


		printf("set layer as top\n");

		layer = gimp_image_get_active_layer(job->image_id);
		if (layer == -1)
		{
			printf("failed to get active layer\n");
			return;
		}

		gimp_image_merge_down(job->image_id, layer, 2);

		printf("merged layers\n");
		job->drawable->drawable_id = gimp_image_get_active_drawable(job->image_id);
//		printf("get active drawable\n");
//		gimp_brightness_contrast(job->drawable->drawable_id, 126, 125);
//		printf("adjust contrast\n");
//
//		printf("Jpeg threshold: %d\n",jpeg_threshold);
//
//		//I should have this subtract against an edge detection layer and then threshold it
//
//		gimp_threshold(job->drawable->drawable_id, jpeg_threshold,255 );
//		printf("threshold\n");

//			if(! gimp_drawable_has_alpha (job->drawable->drawable_id))
//			{
//				 /* some filtermacros do not work with layer that do not have an alpha channel
//				 * and cause gimp to fail on attempt to call gimp_pixel_rgn_init
//				  * with both dirty and shadow flag set to TRUE
//				  * in this situation GIMP displays the error message
//				  *    "expected tile ack and received: 5"
//				  *    and causes the called plug-in to exit immediate without success
//				  * Therfore always add an alpha channel before calling a filtermacro.
//				  */
//				  gimp_layer_add_alpha(layer);
//				  printf("adding alpha channel\n");
//		   }

		remove(temp_file_name);

		sleep(1);

//		job->progress = 4;

		pthread_cond_broadcast(&jpeg_cond);
		pthread_mutex_lock(&jpeg_mutex);
		printf("got lock\n");
		jpeg_wait = 0;
		pthread_mutex_unlock(&jpeg_mutex);

		printf("drawable ID after jpeg %d\n",gimp_image_get_active_drawable(job->image_id));

	}
	else
	{
		printf("thread %d waiting\n", job->thread);
		pthread_mutex_lock(&jpeg_mutex);
		while (jpeg_wait)
		{
			pthread_cond_wait(&jpeg_cond, &jpeg_mutex);
		}
		pthread_mutex_unlock(&jpeg_mutex);
	}


	job->progress = 1;

	return NULL;
}

/* Our usual callback function */
static void cb_jpeg_compress_check_button( GtkWidget *widget,  gpointer   data )
{

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		jpeg_plugin.checked = TRUE;
	}
	else
	{
		jpeg_plugin.checked = FALSE;
	}
}

GtkWidget * create_jpeg_gui()
{

	printf("creating jpeg gui\n");

	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *jpeg_compress_check_button;
	GtkWidget *jpeg_compress_hscale;
	GtkObject *jpeg_compress_value;
	GtkWidget *jpeg_threshold_hscale;
	GtkObject *jpeg_threshold_value;

	label = gtk_label_new ("Jpeg");

	//so this is the page
	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 150);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	jpeg_compress_check_button = gtk_check_button_new_with_label ( "Find Jpeg Age");
	gtk_widget_set_size_request (jpeg_compress_check_button, 200, 50);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(jpeg_compress_check_button), FALSE);
	gtk_widget_show (jpeg_compress_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), jpeg_compress_check_button);
	//then add the page to the notbook


	jpeg_compress_value = gtk_adjustment_new (jpeg_compress, 0, 1.0, .01, .01, .01);
	jpeg_compress_hscale = gtk_hscale_new (GTK_ADJUSTMENT (jpeg_compress_value));
	gtk_scale_set_digits( GTK_SCALE(jpeg_compress_hscale), 3);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (jpeg_compress_hscale, 100, 50);
	gtk_widget_show (jpeg_compress_hscale);

	gtk_container_add (GTK_CONTAINER (tab_box), jpeg_compress_hscale);


	jpeg_threshold_value = gtk_adjustment_new (jpeg_threshold, 0, 255, 1, 1, 1);
	jpeg_threshold_hscale = gtk_hscale_new (GTK_ADJUSTMENT (jpeg_threshold_value));
	gtk_scale_set_digits( GTK_SCALE(jpeg_threshold_hscale), 0);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (jpeg_threshold_hscale, 100, 50);
	gtk_widget_show (jpeg_threshold_hscale);

	gtk_container_add (GTK_CONTAINER (tab_box), jpeg_threshold_hscale);

	g_signal_connect (jpeg_compress_check_button, "clicked", G_CALLBACK (cb_jpeg_compress_check_button), &jpeg_plugin.checked);
	g_signal_connect (GTK_OBJECT (jpeg_compress_value), "value_changed", G_CALLBACK (gimp_float_adjustment_update), &jpeg_compress);
	g_signal_connect (GTK_OBJECT (jpeg_threshold_value), "value_changed", G_CALLBACK (gimp_int_adjustment_update), &jpeg_threshold);

	printf("jpeg compress gui created\n");

	return tab_box;
}

void * jpeg_highlighter_analyze(JOB_ARG *job)
{
	int ii, jj, kk;
	int row;
	int col;

	long long red_sum = 0;
	long long green_sum = 0;
	long long blue_sum = 0;

	guchar max_pixel_color[4];

	union
	{
		guchar pixel[4];
		unsigned int value;
	} stuff;

	gint32 layer_id;
	int temp = 0;
	int unique_compressed_colors = 0;
	int unique_original_colors = 0;

	int *histogram;

	max_pixel_color[0] = 0;
	max_pixel_color[1] = 0;
	max_pixel_color[2] = 0;
	max_pixel_color[3] = 0;

	histogram = (int*)malloc(sizeof(int)*255*255*255);
//
//	GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
//	PIXEL **temp_array;
GimpPixelRgn rgn_in;

	printf("in analizer\n");

	job->drawable->drawable_id = gimp_image_get_active_drawable(job->image_id);

	layer_id = gimp_image_get_active_layer(job->image_id);

//	printf("adding alpha channel\n");
//	gimp_layer_add_alpha(layer_id);
//
//
////	if(! gimp_drawable_has_alpha (layer_id))
////	{
////		 /* some filtermacros do not work with layer that do not have an alpha channel
////		 * and cause gimp to fail on attempt to call gimp_pixel_rgn_init
////		  * with both dirty and shadow flag set to TRUE
////		  * in this situation GIMP displays the error message
////		  *    "expected tile ack and received: 5"
////		  *    and causes the called plug-in to exit immediate without success
////		  * Therfore always add an alpha channel before calling a filtermacro.
////		  */
////		  gimp_layer_add_alpha(layer_id);
////		  printf("adding alpha channel\n");
////   }


	printf("zeroed array\n");

	for(ii = 0; ii < 255*255*255; ii++)
	{
		histogram[ii] = 0;
	}

	gimp_pixel_rgn_init(&rgn_in, job->drawable, job->start_colum, job->start_row, job->image.width, job->image.height, FALSE, FALSE);

	for (row = 0; row < job->image.height; row++)
	{
		for (col = 0; col < job->image.width; col++)
		{
			gimp_pixel_rgn_get_pixel (&rgn_in, stuff.pixel, col,row);

			job->array_out[col][row].red = stuff.pixel[0];
			job->array_out[col][row].green = stuff.pixel[1];
			job->array_out[col][row].blue = stuff.pixel[2];

			red_sum += stuff.pixel[0];
			green_sum += stuff.pixel[1];
			blue_sum += stuff.pixel[2];

			stuff.pixel[3] = 0;

			if(stuff.pixel[0] + stuff.pixel[1]+ stuff.pixel[2] > max_pixel_color[0] +  max_pixel_color[1] +  max_pixel_color[2] )
			{
				max_pixel_color[0] = stuff.pixel[0];
				max_pixel_color[1] = stuff.pixel[1];
				max_pixel_color[2] = stuff.pixel[2];
			}

			histogram[stuff.value]++;

		}

		if (row % 50 == 0)
		{
			gimp_progress_update ((gdouble) row / job->image.height);

		}
	}

	for(ii = 0; ii < 255*255*255; ii++)
	{	
		if(histogram[ii] != 0)
		{
			unique_compressed_colors++;
		}
	}

	//doing the original

	printf("zeroed array\n");

	for(ii = 0; ii < 255*255*255; ii++)
	{
		histogram[ii] = 0;
	}


	for (row = 0; row < job->image.height; row++)
	{
		for (col = 0; col < job->image.width; col++)
		{
			stuff.pixel[0] = job->array_in[col][row].red;
			stuff.pixel[1] = job->array_in[col][row].green;
			stuff.pixel[2] = job->array_in[col][row].blue;

			stuff.pixel[3] = 0;

			histogram[stuff.value]++;

		}
	}

	for(ii = 0; ii < 255*255*255; ii++)
	{
		if(histogram[ii] != 0)
		{
			unique_original_colors++;
		}
	}

	free(histogram);

	print_log("\nJpeg compression difference\n",temp);
	print_log("unique colors in original image:%d\n",unique_original_colors);
	print_log("unique colors in compressed image:%d\n",unique_compressed_colors);
	print_log("ratio: %f\n",(double)unique_original_colors/unique_compressed_colors);
	print_log("brightest compressed color %d %d %d\n",max_pixel_color[0], max_pixel_color[1], max_pixel_color[2]);
	print_log("avg compressed color %f %f %f\n",(float)red_sum/(job->image.height*job->image.width), (float)green_sum/(job->image.height*job->image.width),(float)blue_sum/(job->image.height*job->image.width));
}

void create_jpeg_plugin()
{
	printf("creating jpeg plugin\n");
	jpeg_plugin.checked = FALSE;
	jpeg_plugin.name = "Jpeg compression age";
	jpeg_plugin.label = gtk_label_new (jpeg_plugin.name);
	jpeg_plugin.algorithm = &jpeg_highlighter_algorithm;
	jpeg_plugin.analyze = &jpeg_highlighter_analyze;
	jpeg_plugin.create_gui = &create_jpeg_gui;

	printf("jpeg plugin created\n");

}
