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

#include"texture.h"

int texture_threshold = 110;

void * texture_highlighter_algorithm(JOB_ARG *job)
{

    int size = 20;

    guchar slider[20];
    int ii;
    int counter = 0;
    guchar temp;
    int row, col, channel;

	printf("inside %s thread %d\n", texture_plugin.name, job->thread);

	printf("texture threshold = %d\n", texture_threshold);


    //get the argument passed in, and set our local variables
//	JOB_ARG* job = (JOB_ARG*)pArg;

	//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
		slider[ii] = 0;
    }


// need to do a lot of pre processing on this one



	for (row = 0; row < job->height ; row++)
    {
		for (col = job->start_colum; col < job->start_colum+job->width; col++)
		{
			//set the current element in the slider to our newest pixel value
			slider[counter] = job->array_in[col][row].red;
			temp = 0;
			//look through the slider to see if we have any bright spots
			for(ii = 0; ii < size; ii++)
			{
				if(slider[ii] > temp)
				{
					temp = slider[ii];
				}
			}
			//		    temp =+ slider[ii];
			//
			//	    }
			//
			//	    temp /= size;

			//set the color to red because its been messed with
			if(temp < texture_threshold)
			{
				if(job->array_out[col][row].red == 255 && job->array_out[col][row].green == 255 && job->array_out[col][row].blue == 255)
				{
					//pixels are all white and info is gone
					job->array_out[col][row].red = 110;
					job->array_out[col][row].green = 255;
					job->array_out[col][row].blue = 90;


				}

				else
				{
					if(job->array_out[col][row].red == 0 && job->array_out[col][row].green == 0 && job->array_out[col][row].blue == 0)
					{
						//pixels are all black and info is gone
						job->array_out[col][row].red = 30;
						job->array_out[col][row].green = 75;
						job->array_out[col][row].blue = 30;
					}
					else
					{
						job->array_out[col][row].red = 255;
						job->array_out[col][row].green = 0;
						job->array_out[col][row].blue = 0;
						//			job->array_out[col][row].red = temp;
						//			job->array_out[col][row].green = temp;
						//			job->array_out[col][row].blue = temp;

					}
				}
			}
			//pixels are good
			else
			{
				job->array_out[col][row].red = 80;
				job->array_out[col][row].green = 190;
				job->array_out[col][row].blue = 70;

			}

			//this will reset my slider counter so i dont have to make a queue or anything slow like that
			counter++;
			counter%=size;

		}

		job->progress = (double)row / (job->height * 2);

    }

    //###################
	// now do the same image block virtically
	//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
		slider[ii] = 0;
    }


	for (col = job->start_colum; col < job->start_colum+job->width; col++)
    {
		for (row = 0; row < job->height ; row++)
		{

			//set the current element in the slider to our newest pixel value
			slider[counter] = job->array_in[col][row].red;
			temp = 0;
			//look through the slider to see if we have any bright spots
			for(ii = 0; ii < size; ii++)
			{
				if(slider[ii] > temp)
				{
					temp = slider[ii];
				}
			}
			//		temp =+ slider[ii];
			//
			//	}
			//
			//	temp /= size;
			//set the color to red because its been messed with
			if(temp < texture_threshold)
			{
				//if the pixel is already green we dont want to set it to red, we will set it to grey instead
				if(job->array_out[col][row].green == 75 || job->array_out[col][row].green == 255  || job->array_out[col][row].green == 190 )
				{
					job->array_out[col][row].red = 80;
					job->array_out[col][row].green = 190;
					job->array_out[col][row].blue = 70;
				}
				else
				{
					job->array_out[col][row].red = 255;
					job->array_out[col][row].green = 0;
					job->array_out[col][row].blue = 0;
					//		    job->array_out[col][row].red = temp;
					//		    job->array_out[col][row].green = temp;
					//		    job->array_out[col][row].blue = temp;
				}
			}
			else
			{
				job->array_out[col][row].red = 80;
				job->array_out[col][row].green = 190;
				job->array_out[col][row].blue = 70;
			}

			//this will reset my slider counter so i dont have to make a queue or anything slow like that
			counter++;
			counter%=size;

		}

		job->progress = (double)col / (job->width) +.5;

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


	texture_threshold_value = gtk_adjustment_new (texture_threshold, 0, 256, 1, 1, 1);
	texture_hscale = gtk_hscale_new (GTK_ADJUSTMENT (texture_threshold_value));
	gtk_scale_set_digits( GTK_SCALE(texture_hscale), 0);
	//  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	gtk_widget_set_size_request (texture_hscale, 100, 40);
	gtk_widget_show (texture_hscale);

	g_signal_connect (texture_check_button, "clicked", G_CALLBACK (cb_texture_check_button), &texture_plugin.checked);
	g_signal_connect (GTK_OBJECT (texture_threshold_value), "value_changed", G_CALLBACK (gimp_int_adjustment_update), &texture_threshold);

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
