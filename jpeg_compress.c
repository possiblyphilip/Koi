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

gfloat jpeg_compress = .99;
int jpeg_threshold = 60;

#define SLEEP_TIME 5

pthread_cond_t      jpeg_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     jpeg_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int jpeg_wait = 1;


void * jpeg_highlighter_algorithm(JOB_ARG *job)
{
	GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
	int num_return_vals;
	gchar file_name[] = "/tmp/koi_temp.jpg";
	gint32 layer, temp_layer;

	int ii;

	printf("inside %s thread %d\n", jpeg_plugin.name, job->thread);

// i really only want to run this once and the plugin doesnt know how many threads will get kicked off because its dynamic
	if(job->thread == 0)
	{


//		file_name = gimp_image_get_filename (job->image_id);


//		mkstemp(file_name);
//		printf("using filename %s\n", file_name);


		printf("saving jpeg at %f compression\n", jpeg_compress);
		gimp_run_procedure("file-jpeg-save",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, job->image_id , GIMP_PDB_DRAWABLE, job->drawable->drawable_id, GIMP_PDB_STRING, "/tmp/koi_temp.jpg", GIMP_PDB_STRING, "temp", GIMP_PDB_FLOAT, jpeg_compress, GIMP_PDB_FLOAT, 0.0, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 0, GIMP_PDB_STRING,"created with Koi", GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_INT32, 0, GIMP_PDB_INT32, 1, GIMP_PDB_END);
		printf("waiting for jpeg save\n");
		sleep(1);
		gimp_progress_set_text("waiting for jpeg save\n");
		for(ii = 0; ii < SLEEP_TIME; ii++)
		{
			job->progress = ((float)ii/SLEEP_TIME) * 4;
			sleep(1);
		}

		printf("saved jpeg\n");
//		sleep(1);
		// reload our saved image and suck a layer off of it to subtract against or original image
		temp_layer = gimp_file_load_layer(mode, job->image_id, "/tmp/koi_temp.jpg");

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
		printf("get active drawable\n");
		gimp_brightness_contrast(job->drawable->drawable_id, 126, 125);
		printf("adjust contrast\n");
//		gimp_run_procedure("plug-in-gauss",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, job->drawable->drawable_id, GIMP_PDB_FLOAT, 20.0, GIMP_PDB_FLOAT, 20.0, GIMP_PDB_INT32, 1, GIMP_PDB_END);
//		printf("blur\n");
		printf("Jpeg threshold: %d\n",jpeg_threshold);

		//I should have this subtract against an edge detection layer and then threshold it

		gimp_threshold(job->drawable->drawable_id, jpeg_threshold,255 );
		printf("threshold\n");

		sleep(1);

//		job->progress = 4;

		pthread_cond_broadcast(&jpeg_cond);
		pthread_mutex_lock(&jpeg_mutex);
		printf("got lock\n");
		jpeg_wait = 0;
		pthread_mutex_unlock(&jpeg_mutex);

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

void create_jpeg_plugin()
{
	printf("creating jpeg plugin\n");
	jpeg_plugin.checked = FALSE;
	jpeg_plugin.name = "Jpeg compression age";
	jpeg_plugin.label = gtk_label_new (jpeg_plugin.name);
	jpeg_plugin.algorithm = &jpeg_highlighter_algorithm;
	jpeg_plugin.create_gui = &create_jpeg_gui;

	printf("jpeg plugin created\n");

}
