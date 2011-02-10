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

void * speckle_highlighter_algorithm(JOB_ARG *job)
{

    int temp = 0;
	int row, col;

    int max_col;

	FLOOD_TYPE point;

	//	if(job->thread == 0)
	//	{
	//		sleep(1);
	//	}


	printf("inside %s thread %d\n", clone_plugin.name, job->thread);

    //get the argument passed in, and set our local variables
	//    JOB_ARG* job_args = (JOB_ARG*)pArg;

	//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
	//	if(job->start_colum+job->width < job->image.width)
	//	{
	max_col = job->start_colum+job->width;
	//	}
	//	else
	//	{
	//		max_col = (job->width*NUM_THREADS);
	//	}


	//edge find the image
	laplace(job);



	printf("thread %d start colum %d max col %d\n", job->thread, job->start_colum, max_col);

	//copy the output of laplace into the input of this algo
	for (row = 0; row < job->height; row++)
	{
		for (col = job->start_colum; col < max_col; col++)
		{


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


			job->array_out[col][row].red = 0;
			job->array_out[col][row].green = 0;
			job->array_out[col][row].blue = 0;
		}
	}



	for (row = 0; row < job->height; row++)
	{
		for (col = job->start_colum; col < max_col; col++)
		{
			if(job->array_in[col][row].red > 100)
			{
				point.col = col;
				point.row = row;
				job->options = &point;
				temp = flood(job);

				//				if(temp > 1)
				//				{
				//
				//					job->array_out[col][row].red = temp;
				//					job->array_out[col][row].green = temp;
				//					job->array_out[col][row].blue = temp;
				//				}
				//				else
				//				{
				//
				//
				//
				//					job->array_out[col][row].red = 255;
				//					job->array_out[col][row].green = 125;
				//					job->array_out[col][row].blue = 76;
				//				}
			}
		}
		job->progress = (double)row / job->height;
	}


	job->array_out[job->start_colum][10].red = 255;
	job->array_out[job->start_colum][10].green = 0;
	job->array_out[job->start_colum][10].blue = 0;



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

void create_speckle_plugin()
{
	printf("creating speckle plugin\n");
	speckle_plugin.checked = FALSE;
	speckle_plugin.name = "speckle Highliter";
	speckle_plugin.label = gtk_label_new (speckle_plugin.name);
	speckle_plugin.algorithm = &speckle_highlighter_algorithm;
	speckle_plugin.create_gui = &create_speckle_gui;
//	speckle_plugin.options = &speckle_block_size;
	printf("speckle plugin created\n");

}
