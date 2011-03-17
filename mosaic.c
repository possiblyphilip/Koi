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
#include "mosaic.h"
#include "laplace.c"

gfloat mosaic_threshold = 180;

void * mosaic_highlighter_algorithm(JOB_ARG *job)
{
	int block_size = 1;
	int temp;
	int row, col, offset_row,offset_col;
	int max_col;

	int max_green_delta;
	int max_red_delta;
	int max_blue_delta;
//	PIXEL **temp_array;

//	allocate_pixel_array(&temp_array,job->width*2, job->height);

	printf("inside %s thread %d\n", mosaic_plugin.name, job->thread);





	//edge find the image
//		laplace(job);

		//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
		//basically this allows each thread to read the other threads data so there are not gaps between
		//the work each thread does

		if(job->start_colum+job->width+block_size < job->image.width)
		{
			max_col = job->start_colum+job->width+block_size;
		}
		else
		{
			max_col = job->image.width - block_size;
		}

//run through the image
		for (row = block_size; row < job->height-block_size ; row++)
		{
			for (col = job->start_colum+block_size; col < max_col; col++)
			{
				temp = 0;
				max_green_delta = 0;
				max_red_delta = 0;
				max_blue_delta = 0;

				//loop through the block
				for(offset_row = -1; offset_row <= 1; offset_row++)
				{
					for(offset_col = -1; offset_col <= 1; offset_col++)
					{
						temp = abs(job->array_in[col][row].red - job->array_in[col+offset_col][row+offset_row].red);
						if(temp > max_red_delta)
						{
							max_red_delta = temp;
						}

						temp = abs(job->array_in[col][row].green - job->array_in[col+offset_col][row+offset_row].green);
						if(temp > max_green_delta)
						{
							max_green_delta = temp;
						}

						temp = abs(job->array_in[col][row].blue - job->array_in[col+offset_col][row+offset_row].blue);
						if(temp > max_blue_delta)
						{
							max_blue_delta = temp;
						}

					}
				}


				if(max_red_delta > mosaic_threshold)
				{
					job->array_out[col][row].red = max_red_delta;
				}
				if(max_green_delta > mosaic_threshold)
				{
					job->array_out[col][row].green = max_green_delta;
				}
				if(max_blue_delta > mosaic_threshold)
				{
					job->array_out[col][row].blue = max_blue_delta;
				}

			}

			job->progress = (double)row / job->height;
		}

		//the x2 is sketch but i have it in there because it was crashing for some reason ... really aught to go fix it

//	free_pixel_array(temp_array,job->width*2);

	job->progress = 1;

	return NULL;
}

/* Our usual callback function */
static void cb_mosaic_check_button( GtkWidget *widget,  gpointer   data )
{

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		mosaic_plugin.checked = TRUE;
	}
	else
	{
		mosaic_plugin.checked = FALSE;
	}
}

GtkWidget * create_mosaic_gui()
{

	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *mosaic_check_button;
	GtkWidget *mosaic_hscale;
	GtkObject *mosaic_threshold_value;

	label = gtk_label_new ("mosaic");

	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 100);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	mosaic_check_button = gtk_check_button_new_with_label ( "Find mosaic Loss");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mosaic_check_button), FALSE);
	gtk_widget_set_size_request (mosaic_check_button, 100, 40);
	gtk_widget_show (mosaic_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), mosaic_check_button);
	//then add the page to the notbook
	// gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);

	/* value, lower, upper, step_increment, page_increment, page_size */
	/* Note that the page_size value only makes a difference for
	  * scrollbar widgets, and the highest value you'll get is actually
	  * (upper - page_size). */
	//    mosaic_threshold_value = gtk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);


	mosaic_threshold_value = gtk_adjustment_new (mosaic_threshold, 0, 255, 1, 1, 1);
	mosaic_hscale = gtk_hscale_new (GTK_ADJUSTMENT (mosaic_threshold_value));
	gtk_scale_set_digits( GTK_SCALE(mosaic_hscale), 0);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (mosaic_hscale, 100, 40);
	gtk_widget_show (mosaic_hscale);

	g_signal_connect (mosaic_check_button, "clicked", G_CALLBACK (cb_mosaic_check_button), &mosaic_plugin.checked);
	g_signal_connect (GTK_OBJECT (mosaic_threshold_value), "value_changed", G_CALLBACK (gimp_float_adjustment_update), &mosaic_threshold);

	gtk_container_add (GTK_CONTAINER (tab_box), mosaic_hscale);


	//then add the page to the notbook

	return tab_box;
}

void * mosaic_highlighter_analyze(JOB_ARG *job)
{
	int row;
	int col;

	int top_right_red_max = 0;
	int bottom_right_red_max = 0;
	int top_left_red_max = 0;
	int bottom_left_red_max = 0;

	int top_right_green_max = 0;
	int bottom_right_green_max = 0;
	int top_left_green_max = 0;
	int bottom_left_green_max = 0;

	int top_right_blue_max = 0;
	int bottom_right_blue_max = 0;
	int top_left_blue_max = 0;
	int bottom_left_blue_max = 0;

	int temp;

	int num_pixels;


	float top_right_filled = 0;
	float bottom_right_filled = 0;
	float top_left_filled = 0;
	float bottom_left_filled = 0;

	num_pixels = job->image.height * job->image.width;

	for (row = 0; row < job->image.height; row++)
	{
		for (col = 0; col < job->image.width; col++)
		{

			//top left
			if(col < job->image.width/2 && row < job->image.height/2)
			{

				if(job->array_out[col][row].red > top_left_red_max)
				{
					top_left_red_max = job->array_out[col][row].red;
				}
				if(job->array_out[col][row].green > top_left_green_max)
				{
					top_left_green_max = job->array_out[col][row].green;
				}
				if(job->array_out[col][row].blue > top_left_blue_max)
				{
					top_left_blue_max = job->array_out[col][row].blue;
				}

				if(job->array_out[col][row].red + job->array_out[col][row].green + job->array_out[col][row].blue)
				{
					top_left_filled++;
				}

			}

			//top right
			if(col > job->image.width/2 && row < job->image.height/2)
			{
				if(job->array_out[col][row].red > top_right_red_max)
				{
					top_right_red_max = job->array_out[col][row].red;
				}
				if(job->array_out[col][row].green > top_right_green_max)
				{
					top_right_green_max = job->array_out[col][row].green;
				}
				if(job->array_out[col][row].blue > top_right_blue_max)
				{
					top_right_blue_max = job->array_out[col][row].blue;
				}

				if(job->array_out[col][row].red + job->array_out[col][row].green + job->array_out[col][row].blue)
				{
					top_right_filled++;
				}
			}

			//bottom left
			if(col < job->image.width/2 && row > job->image.height/2)
			{
				if(job->array_out[col][row].red > bottom_left_red_max)
				{
					bottom_left_red_max = job->array_out[col][row].red;
				}
				if(job->array_out[col][row].green > bottom_left_green_max)
				{
					bottom_left_green_max = job->array_out[col][row].green;
				}
				if(job->array_out[col][row].blue > bottom_left_blue_max)
				{
					bottom_left_blue_max = job->array_out[col][row].blue;
				}

				if(job->array_out[col][row].red + job->array_out[col][row].green + job->array_out[col][row].blue)
				{
					bottom_left_filled++;
				}
			}

			//bottom right
			if(col > job->image.width/2 && row > job->image.height/2)
			{
				if(job->array_out[col][row].red > bottom_right_red_max)
				{
					bottom_right_red_max = job->array_out[col][row].red;
				}
				if(job->array_out[col][row].green > bottom_right_green_max)
				{
					bottom_right_green_max = job->array_out[col][row].green;
				}
				if(job->array_out[col][row].blue > bottom_right_blue_max)
				{
					bottom_right_blue_max = job->array_out[col][row].blue;
				}

				if(job->array_out[col][row].red + job->array_out[col][row].green + job->array_out[col][row].blue)
				{
					bottom_right_filled++;
				}
			}
		}
	}


	print_log("mosaic max contrast stats\n");


	print_log("top left: red %d, green %d, blue %d\n", top_left_red_max,top_left_green_max,top_left_blue_max);
	print_log("top right: red %d, green %d, blue %d\n", top_right_red_max,top_right_green_max,top_right_blue_max);
	print_log("bottom left: red %d, green %d, blue %d\n", bottom_left_red_max,bottom_left_green_max,bottom_left_blue_max);
	print_log("bottom right: red %d, green %d, blue %d\n", bottom_right_red_max,bottom_right_green_max,bottom_right_blue_max);
	print_log("overall max\n");

	//must be a better way to do this
	temp = top_left_red_max;
	if(top_right_red_max > temp)
	{
		temp = top_right_red_max;
	}
	if(bottom_left_red_max > temp)
	{
		temp = bottom_left_red_max;
	}
	if(bottom_right_red_max > temp)
	{
		temp = bottom_right_red_max;
	}
	print_log("red %d, ", temp);

	temp = top_left_green_max;
	if(top_right_green_max > temp)
	{
		temp = top_right_green_max;
	}
	if(bottom_left_green_max > temp)
	{
		temp = bottom_left_green_max;
	}
	if(bottom_right_green_max > temp)
	{
		temp = bottom_right_green_max;
	}
	print_log("green %d, ", temp);

	temp = top_left_blue_max;
	if(top_right_blue_max > temp)
	{
		temp = top_right_blue_max;
	}
	if(bottom_left_blue_max > temp)
	{
		temp = bottom_left_blue_max;
	}
	if(bottom_right_blue_max > temp)
	{
		temp = bottom_right_blue_max;
	}
	print_log("blue %d\n", temp);

}

void create_mosaic_plugin()
{
	printf("creating mosaic plugin\n");
	mosaic_plugin.checked = FALSE;
	mosaic_plugin.name = "mosaic Highliter";
	mosaic_plugin.label = gtk_label_new (mosaic_plugin.name);
		mosaic_plugin.analyze = &mosaic_highlighter_analyze;
	mosaic_plugin.algorithm = &mosaic_highlighter_algorithm;
	mosaic_plugin.create_gui = &create_mosaic_gui;

	printf("mosaic plugin created\n");

}
