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
#include"speckle.h"
#include "laplace.c"
#include "flood_fill.c"
#include <assert.h>

void * speckle_highlighter_algorithm(JOB_ARG *job)
{

	int threshold = 255;
	float count = 1;
	long long size = 0;
	int row, col;
    int max_col;
	POINT_TYPE point;

	PIXEL **temp_array;

	allocate_pixel_array(&temp_array,job->width*2, job->height);

	printf("inside %s thread %d\n", speckle_plugin.name, job->thread);

	//this snipit should let the colums blend in the middle of the image without writing over the edge of the image


		max_col = job->start_colum+job->width;


	//i copy the array so i can keep it fresh and not mess it up when i do multiple revs to get
	//the auto threshold right
	for (row = 0; row < job->height; row++)
	{
		for (col = job->start_colum; col < max_col; col++)
		{
			temp_array[col-job->start_colum][row].red = job->array_in[col][row].red;
			temp_array[col-job->start_colum][row].green = job->array_in[col][row].green;
			temp_array[col-job->start_colum][row].blue = job->array_in[col][row].blue;
		}
	}

    //get the argument passed in, and set our local variables
	//    JOB_ARG* job_args = (JOB_ARG*)pArg;


	while ((size / (float)count) < 2)
	{
		//its set to 1 so i dont dev by zero
		count = 1;
		size = 0;



		//copy in the fresh array
		for (row = 0; row < job->height; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{
				job->array_in[col][row].red = temp_array[col-job->start_colum][row].red;
				job->array_in[col][row].green = temp_array[col-job->start_colum][row].green;
				job->array_in[col][row].blue = temp_array[col-job->start_colum][row].blue;
			}
		}

		//edge find the image
		laplace(job);


//		printf("thread %d start colum %d max col %d\n", job->thread, job->start_colum, max_col);

		//copy the output of laplace into the input of this algo
		for (row = 0; row < job->height; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{
				//assert(col>=0 && col < job->image.width);
				//assert(row>=0 && row < job->image.height);
				job->array_in[col][row].red = job->array_out[col][row].red;
				job->array_in[col][row].green = job->array_out[col][row].green;
				job->array_in[col][row].blue = job->array_out[col][row].blue;
			}
		}


		//write the old output to black
		for (row = 0; row < job->height; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{

				//			assert(col>=0 && col < job->image.width);
				//			assert(row>=0 && row < job->image.height);
				job->array_out[col][row].red = 0;
				job->array_out[col][row].green = 0;
				job->array_out[col][row].blue = 0;
			}
		}

		//printf("copied array and blacked out array\n");

		for (row = 0; row < job->height; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{
				if(job->array_in[col][row].red > threshold )
				{
					//				assert(col>=0 && col < job->image.width);
					//				assert(row>=0 && row < job->image.height);
					point.col = col;
					point.row = row;
					point.threshold = threshold;
//					printf("real threshold %d\n",threshold);
					job->options = &point;
					size += flood(job);
					count++;
				}
			}
			job->progress = (double)row / job->height;
		}



		printf("thread %d threshold %d size %f\n", job->thread, threshold, (size / (float)count));
		threshold--;
	}

	free_pixel_array(temp_array,job->width*2);

//printf("survived\n");

	job->progress = 1;

    return NULL;
}

/* Our usual callback function */
static void cb_speckle_check_button( GtkWidget *widget)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		speckle_plugin.checked = 1;
	}
	else
	{
		speckle_plugin.checked = 0;
	}
}

GtkWidget * create_speckle_gui()
{
	printf("creating speckle gui\n");
	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *speckle_check_button;
	GtkWidget *block_size_spinbutton;
	GtkObject *block_size_spinbutton_value;

	label = gtk_label_new ("speckle");

	//so this is the page
	//  frame = gtk_frame_new ("speckle tool use");
	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 75);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	speckle_check_button = gtk_check_button_new_with_label ( "Find speckle size");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(speckle_check_button), FALSE);
	gtk_widget_show (speckle_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), speckle_check_button);

//
//	block_size_spinbutton = gimp_spin_button_new (&block_size_spinbutton_value, speckle_block_size, 4, 40, 4, 4, 0, 4, 0);
//	gtk_container_add (GTK_CONTAINER (tab_box), block_size_spinbutton);
//	gtk_widget_show (block_size_spinbutton);
//
//	g_signal_connect (block_size_spinbutton_value, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &speckle_block_size);

	g_signal_connect (speckle_check_button, "clicked", G_CALLBACK (cb_speckle_check_button), &speckle_plugin.checked);


	//then add the page to the notbook

	printf("speckle gui created\n");

	return tab_box;
}

void * speckle_highlighter_analyze(JOB_ARG *job)
{
	int row;
	int col;

	long long top_right_height = 0;
	long long bottom_right_height = 0;
	long long top_left_height = 0;
	long long bottom_left_height = 0;

	long long top_right_width = 0;
	long long bottom_right_width = 0;
	long long top_left_width = 0;
	long long bottom_left_width = 0;

	long long size = 0;

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
				top_left_width += job->array_out[col][row].red;
				size += job->array_out[col][row].green;
				top_left_height += job->array_out[col][row].blue;

				if(job->array_out[col][row].green > 0)
				{
					top_left_filled++;
				}

			}

			//top right
			if(col > job->image.width/2 && row < job->image.height/2)
			{
				top_right_width += job->array_out[col][row].red;
				size += job->array_out[col][row].green;
				top_right_height += job->array_out[col][row].blue;

				if(job->array_out[col][row].green > 0)
				{
					top_right_filled++;
				}
			}

			//bottom left
			if(col < job->image.width/2 && row > job->image.height/2)
			{
				bottom_left_width += job->array_out[col][row].red;
				size += job->array_out[col][row].green;
				bottom_left_height += job->array_out[col][row].blue;

				if(job->array_out[col][row].green > 0)
				{
					bottom_left_filled++;
				}
			}

			//bottom right
			if(col > job->image.width/2 && row > job->image.height/2)
			{
				bottom_right_width += job->array_out[col][row].red;
				size += job->array_out[col][row].green;
				bottom_right_height += job->array_out[col][row].blue;

				if(job->array_out[col][row].green > 0)
				{
					bottom_right_filled++;
				}
			}
		}
	}

	print_log("Speckle stats\n");

	print_log("avg speckle size %f\n\n", (size/(float)num_pixels) );

	print_log("top left avg speckle width %f\n", top_left_width / top_left_filled );
	print_log("top left avg speckle height %f\n\n", top_left_height / top_left_filled );

	print_log("top right avg speckle width %f\n", top_right_width / top_right_filled );
	print_log("top right avg speckle height %f\n\n", top_right_height / top_right_filled );

	print_log("bottom left avg speckle width %f\n", bottom_left_width / bottom_left_filled );
	print_log("bottom left avg speckle height %f\n\n", bottom_left_height / bottom_left_filled );

	print_log("bottom right avg speckle width %f\n", bottom_right_width / bottom_right_filled );
	print_log("bottom right avg speckle height %f\n\n", bottom_right_height / bottom_right_filled );


}

void create_speckle_plugin()
{
	printf("creating speckle plugin\n");
	speckle_plugin.checked = FALSE;
	speckle_plugin.name = "speckle Highliter";
	speckle_plugin.label = gtk_label_new (speckle_plugin.name);
	speckle_plugin.analyze = &speckle_highlighter_analyze;
	speckle_plugin.algorithm = &speckle_highlighter_algorithm;
	speckle_plugin.create_gui = &create_speckle_gui;
	printf("speckle plugin created\n");

}
