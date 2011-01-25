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
#include "texture.h"
#include "laplace.c"

gfloat texture_threshold = 15;

void * texture_highlighter_algorithm(JOB_ARG *job)
{
	int block_size = 16;
	int temp;
	int row, col, block_row,block_col;
	int max_col;
	PIXEL **temp_array;



	printf("inside %s thread %d\n", texture_plugin.name, job->thread);

	printf("texture threshold = %f\n", texture_threshold);




	//edge find the image
		laplace(job);

		//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
		//basically this allows each thread to read the other threads data so there are not gaps between
		//the work each thread does

		if(job->start_colum+job->width+block_size < job->image.width)
		{
			max_col = job->start_colum+job->width;
		}
		else
		{
			max_col = job->image.width - block_size;
		}


			allocate_pixel_array(&temp_array,job->width*2, job->height);

//		threshold the image by blocks
		for (row = 0; row < job->height-block_size ; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{
				temp = 0;
				//loop through the block
				for (block_row = 0; block_row < block_size; block_row++)
				{
					for (block_col = 0; block_col < block_size; block_col++)
					{
						temp += job->array_out[col+block_col][row+block_row].red;
					}
				}
//if its got too little texture ill write it out to the output image
				if(temp <= block_size*block_size*texture_threshold)
				{
					for (block_row = 0; block_row < block_size; block_row++)
					{
						for (block_col = 0; block_col < block_size; block_col++)
						{
							temp_array[col+block_col-job->start_colum][row+block_row].red = job->array_in[col+block_col][row+block_row].red;
							temp_array[col+block_col-job->start_colum][row+block_row].green = job->array_in[col+block_col][row+block_row].green;
							temp_array[col+block_col-job->start_colum][row+block_row].blue = job->array_in[col+block_col][row+block_row].blue;
						}
					}

//					for (block_row = 0; block_row < block_size; block_row++)
//					{
//						for (block_col = 0; block_col < block_size; block_col++)
//						{
//							job->array_in[col+block_col][row+block_row].red = 0;
//							job->array_in[col+block_col][row+block_row].green = 0;
//							job->array_in[col+block_col][row+block_row].blue = 0;
//						}
//					}
//
//				}
////if its got enough texture ill black it out
//				else
//				{
//
				}
			}

			job->progress = (double)row / job->height;
		}

//		for (row = 0; row < job->height-block_size ; row++)
//		{
//			for (col = job->start_colum; col < max_col; col++)
//			{
//				job->array_out[col][row].red = 0;
//				job->array_out[col][row].green = temp_array[col-job->start_colum][row].green;
//				job->array_out[col][row].blue = temp_array[col-job->start_colum][row].blue;
//			}
//		}

		for (row = 0; row < job->height-block_size ; row++)
		{
			for (col = job->start_colum; col < max_col; col++)
			{
				job->array_out[col][row].red = temp_array[col-job->start_colum][row].red;
				job->array_out[col][row].green = temp_array[col-job->start_colum][row].green;
				job->array_out[col][row].blue = temp_array[col-job->start_colum][row].blue;
			}
		}


	job->progress = 1;

    return NULL;
}

/* Our usual callback function */
static void cb_texture_check_button( GtkWidget *widget,  gpointer   data )
{

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		texture_plugin.checked = TRUE;
	}
	else
	{
		texture_plugin.checked = FALSE;
	}
}

GtkWidget * create_texture_gui()
{

	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *texture_check_button;
	GtkWidget *texture_hscale;
	GtkObject *texture_threshold_value;

	label = gtk_label_new ("Texture");

	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 100);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	texture_check_button = gtk_check_button_new_with_label ( "Find Texture Loss");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(texture_check_button), FALSE);
	gtk_widget_set_size_request (texture_check_button, 100, 40);
	gtk_widget_show (texture_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), texture_check_button);
	//then add the page to the notbook
	// gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab_box, label);

	/* value, lower, upper, step_increment, page_increment, page_size */
	/* Note that the page_size value only makes a difference for
	  * scrollbar widgets, and the highest value you'll get is actually
	  * (upper - page_size). */
	//    texture_threshold_value = gtk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);


	texture_threshold_value = gtk_adjustment_new (texture_threshold, 0, 255, 1, 1, 1);
	texture_hscale = gtk_hscale_new (GTK_ADJUSTMENT (texture_threshold_value));
	gtk_scale_set_digits( GTK_SCALE(texture_hscale), 0);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (texture_hscale, 100, 40);
	gtk_widget_show (texture_hscale);

	g_signal_connect (texture_check_button, "clicked", G_CALLBACK (cb_texture_check_button), &texture_plugin.checked);
	g_signal_connect (GTK_OBJECT (texture_threshold_value), "value_changed", G_CALLBACK (gimp_float_adjustment_update), &texture_threshold);

	gtk_container_add (GTK_CONTAINER (tab_box), texture_hscale);


	//then add the page to the notbook

	return tab_box;
}

void create_texture_plugin()
{
	printf("creating texture plugin\n");
	texture_plugin.checked = FALSE;
	texture_plugin.name = "Texture Highliter";
	texture_plugin.label = gtk_label_new (texture_plugin.name);
	texture_plugin.algorithm = &texture_highlighter_algorithm;
	texture_plugin.create_gui = &create_texture_gui;

	printf("texture plugin created\n");

}
