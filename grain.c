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
#include"grain.h"

int grain_radius = 16;

pthread_cond_t      grain_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     grain_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int grain_wait_var = 1;


void laplace(JOB_ARG *job)
{

	int offset_col, offset_row;
	int SUM;
	int	MASK[5][5];
	int row, col;
	int temp;
	int max_col;

	/* 5x5 Laplace mask.  Ref: Myler Handbook p. 135 */
	MASK[0][0] = -1; MASK[0][1] = -1; MASK[0][2] = -1; MASK[0][3] = -1; MASK[0][4] = -1;
	MASK[1][0] = -1; MASK[1][1] = -1; MASK[1][2] = -1; MASK[1][3] = -1; MASK[1][4] = -1;
	MASK[2][0] = -1; MASK[2][1] = -1; MASK[2][2] = 24; MASK[2][3] = -1; MASK[2][4] = -1;
	MASK[3][0] = -1; MASK[3][1] = -1; MASK[3][2] = -1; MASK[3][3] = -1; MASK[3][4] = -1;
	MASK[4][0] = -1; MASK[4][1] = -1; MASK[4][2] = -1; MASK[4][3] = -1; MASK[4][4] = -1;

	if(job->start_colum+job->width+5 < job->width*NUM_THREADS)
	{
		max_col = job->start_colum+job->width+5;

	}
	else
	{
		max_col = (job->width*NUM_THREADS)-5;
	}

	for(row = 0; row < job->height-5; row++)
	{
		for(col = job->start_colum; col < max_col; col++)
		{
			SUM = 0;

			for(offset_row=0; offset_row < 5; offset_row++)
			{
				for(offset_col=0; offset_col < 5; offset_col++)
				{
					temp = 0;

					temp += job->array_in[col+offset_col][row+offset_row].red;
					temp += job->array_in[col+offset_col][row+offset_row].green;
					temp += job->array_in[col+offset_col][row+offset_row].blue;

					temp /= 3;

					SUM += temp * MASK[offset_col][offset_row];

				}
			}

			SUM = abs(SUM);

			if(SUM>255)
			{
				SUM=255;
			}


			job->array_out[col][row].red = SUM;
			job->array_out[col][row].green = SUM;
			job->array_out[col][row].blue = SUM;
		}
	}
}

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

void create_grain_plugin()
{
	printf("creating grain plugin\n");
	grain_plugin.checked = FALSE;
	grain_plugin.name = "Grain Highliter";
	grain_plugin.label = gtk_label_new (grain_plugin.name);
	grain_plugin.algorithm = &grain_highlighter_algorithm;
	grain_plugin.create_gui = &create_grain_gui;

	printf("grain plugin created\n");

}


//	if(job->thread == 0)
//	{
//
//				printf("making new pixel region\n");
//		gimp_pixel_rgn_init (&rgn_in, job->drawable, job->start_colum, 0,job->width, job->height, FALSE, FALSE);
//		sleep(3);
//		printf("making image gray scale\n");
//		gimp_run_procedure("gimp-desaturate",&num_return_vals, GIMP_PDB_DRAWABLE, job->drawable->drawable_id, GIMP_PDB_END);
//		sleep(3);
//		printf("edge finding\n");
//		gimp_run_procedure("plug-in-edge",&num_return_vals, GIMP_PDB_INT32, mode, GIMP_PDB_IMAGE, 0 , GIMP_PDB_DRAWABLE, job->drawable->drawable_id, GIMP_PDB_FLOAT, 9.99, GIMP_PDB_INT32, 2, GIMP_PDB_INT32, 5, GIMP_PDB_END);
//		printf("done edge finding\n");
//		sleep(3);
//
//		//dump the gimp image data back into my own array for processing
//		printf("filling Koi array\n");
//		gimp_progress_set_text("filling Koi array\n");
//
//		for (row = 0; row < job->image.height; row++)
//		{
//			for (col = 0; col < job->image.width; col++)
//			{
//				gimp_pixel_rgn_get_pixel (&rgn_in, pixel, col,row);
//
//				job->array_in[col][row].red = pixel[0];
//				job->array_in[col][row].green = pixel[1];
//				job->array_in[col][row].blue = pixel[2];
//			}
//
//			if (row % 50 == 0)
//			{
//				gimp_progress_update ((gdouble) row / job->image.height);
//			}
//		}
//
//		pthread_cond_broadcast(&grain_cond);
//		pthread_mutex_lock(&grain_mutex);
//		printf("got lock\n");
//		grain_wait_var = 0;
//		pthread_mutex_unlock(&grain_mutex);
//	}
//	else
//	{
//		printf("thread %d waiting\n", job->thread);
//		pthread_mutex_lock(&grain_mutex);
//		while (grain_wait_var)
//		{
//			pthread_cond_wait(&grain_cond, &grain_mutex);
//		}
//		pthread_mutex_unlock(&grain_mutex);
//		printf("thread %d running\n", job->thread);
//	}
