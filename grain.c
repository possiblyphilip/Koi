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
#include "grain.h"
#include "laplace.c"

int grain_radius = 16;

pthread_cond_t      grain_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     grain_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int grain_wait_var = 1;


void * grain_highlighter_algorithm(JOB_ARG *job)
{
	GimpRunMode mode = GIMP_RUN_NONINTERACTIVE;
	GimpPixelRgn rgn_in;

    int radius;

    float angle;

    float best_angle;
    int highest;


    int ii;
    int counter = 0;
	double temp;
	int row, col;
    int row_offset;
    int col_offset;

	int max_col, min_col;
	int num_return_vals;

	//PIXEL pixel;
	guchar pixel[4];

	printf("inside %s thread %d\n", grain_plugin.name, job->thread);

	laplace(job);

	//copying the output of the laplace job to my input
			for (row = 0; row < job->height; row++)
			{
				for (col = job->start_colum; col < job->start_colum+job->width; col++)
				{
					job->array_in[col][row].red = job->array_out[col][row].red;
					job->array_in[col][row].green = job->array_out[col][row].green;
					job->array_in[col][row].blue = job->array_out[col][row].blue;
				}
			}


	radius = grain_radius;

			//    //this snipit should let the colums blend in the middle of the image without writing over the edge of the image
				if(job->start_colum+job->width+radius < job->width*NUM_THREADS)
				{
					max_col = job->start_colum+job->width;

				}
				else
				{
					max_col = (job->width*NUM_THREADS) - radius;
				}


	//if im at the left wall i need to start over at least one radius so i dont run off the page
	if(job->start_colum == 0)
	{
		min_col = radius;

	}
	else
	{
		min_col = job->start_colum;
	}

	//set my image to black

	for (row = 0; row < job->height; row++)
	{
		for (col = job->start_colum; col < job->start_colum+job->width; col++)
		{
			job->array_in[col][row].red = job->array_out[col][row].red;
			job->array_out[col][row].red = 0;
	//		job->array_in[col][row].green = job->array_out[col][row].green;
			job->array_out[col][row].green = 0;
	//		job->array_in[col][row].blue = job->array_out[col][row].blue;
			job->array_out[col][row].blue = 0;
		}
	}


	for (row = radius; row < job->height-radius ; row+=radius/4)
	{
		for (col = min_col; col < max_col; col+=radius/4)
		{
			highest = 0;

			for(angle = 0; angle < 3.14*2; angle+= .02)
			{

				temp = 0;
				for(ii = 0; ii < radius; ii++)
				{
					col_offset = (ii*cos(angle));
					row_offset = (ii*sin(angle));
					temp +=  job->array_in[col+col_offset][row+row_offset].red;
				}

				if(temp > highest)
				{
					highest = temp;
					best_angle = angle;
				}
			}
			//write back the best line in only if it is brighter than the other lines it is crossing
			for(ii = 0; ii < radius; ii++)
			{
				col_offset = (ii*cos(best_angle));
				row_offset = (ii*sin(best_angle));
				//		job->array_out[col+col_offset][row+row_offset].red = 230;
				//		job->array_out[col+col_offset][row+row_offset].green = 230;
				//		job->array_out[col+col_offset][row+row_offset].blue = 50;


				if(job->array_out[col+col_offset][row+row_offset].red > highest/radius)
				{
					highest = 0;
				}



			}

			if(best_angle > 3.14)
			{
				best_angle -= 3.14;
			}


			if(highest/radius > 100)
			{
				for(ii = 0; ii < radius; ii++)
				{
					col_offset = (ii*cos(best_angle));
					row_offset = (ii*sin(best_angle));
					//		job->array_out[col+col_offset][row+row_offset].red = 230;
					//		job->array_out[col+col_offset][row+row_offset].green = 230;
					//		job->array_out[col+col_offset][row+row_offset].blue = 50;

					job->array_out[col+col_offset][row+row_offset].red = highest/radius;
					job->array_out[col+col_offset][row+row_offset].green = (best_angle/(3.14))*255;
					job->array_out[col+col_offset][row+row_offset].blue = highest/radius;

				}
			}


		}


		job->progress = (double)row / job->height;

	}

	job->progress = 1;

    return NULL;
}

/* Our usual callback function */
static void cb_grain_check_button( GtkWidget *widget,  gpointer   data )
{

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		grain_plugin.checked = TRUE;
	}
	else
	{
		grain_plugin.checked = FALSE;
	}
}

GtkWidget * create_grain_gui()
{

	printf("creating grain gui\n");

	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *grain_check_button;
	GtkWidget *radius_hscale;
	GtkObject *radius_value;

	label = gtk_label_new ("Grain");

	//so this is the page
	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 75);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	grain_check_button = gtk_check_button_new_with_label ( "Find image grain");
	//this should make sure that it shows the correct status
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grain_check_button), grain_plugin.checked);
	gtk_widget_show (grain_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), grain_check_button);
	//then add the page to the notbook


	radius_value = gtk_adjustment_new (grain_radius, 0, 50, 1, 1, 1);
	radius_hscale = gtk_hscale_new (GTK_ADJUSTMENT (radius_value));
	gtk_scale_set_digits( GTK_SCALE(radius_hscale), 0);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (radius_hscale, 100, 40);
	gtk_widget_show (radius_hscale);

	g_signal_connect (grain_check_button, "clicked", G_CALLBACK (cb_grain_check_button), &grain_plugin.checked);
//	gtk_signal_connect (GTK_OBJECT (radius_value), "value_changed", GTK_SIGNAL_FUNC (cb_radius_hscale), &grain_radius);
	g_signal_connect (GTK_OBJECT (radius_value), "value_changed", G_CALLBACK (gimp_int_adjustment_update), &grain_radius);

	gtk_container_add (GTK_CONTAINER (tab_box), radius_hscale);

	printf("grain gui created\n");

	return tab_box;
}
void * grain_highlighter_analyze(JOB_ARG *job)
{
	int row;
	int col;

	FILE *log_file;

	int temp = 0;

	for (row = 0; row < job->image.height; row++)
	{
		for (col = 0; col < job->image.width; col++)
		{
			temp += job->array_out[col][row].red;
		}
	}

	log_file = fopen("/tmp/koi_log.txt", "a");

	if(log_file == NULL)
	{
		printf("failed to open /tmp/koi_log.txt\n");
	}

	fprintf(log_file, "dont know what to do with the grain just yet\n", temp);

	fclose(log_file);

}

void create_grain_plugin()
{
	printf("creating grain plugin\n");
	grain_plugin.checked = FALSE;
	grain_plugin.name = "Grain Highliter";
	grain_plugin.label = gtk_label_new (grain_plugin.name);
	grain_plugin.algorithm = &grain_highlighter_algorithm;
	grain_plugin.analyze = &grain_highlighter_analyze;
	grain_plugin.create_gui = &create_grain_gui;

	printf("grain plugin created\n");

}
