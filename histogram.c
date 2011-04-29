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

#include"histogram.h"

void * histogram(void *pArg)
{

    int block_size;

    int block_row,block_col;
    int counter = 0;
    int temp = 0;
	int row, to_row, col, to_col;
    int num_blocks;
    int ii;

    int total_min = 300;
    int total_max = 0;
    int red_min = 300;
    int red_max = 0;
    int green_min = 300;
    int green_max = 0;
    int blue_min = 300;
    int blue_max = 0;

    int total_width;
    int red_width;
    int green_width;
    int blue_width;
    int total_overlap;
    int largest_gap;


    int max_col;

    int red_index, green_index, blue_index;

    histogram_struct histo;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;
	block_size = job_args->gui_options->histogram_block_size;






    printf("Histogram block size: %d\n",block_size);





	//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
	if(job_args->start_colum+job_args->width+block_size < job_args->width*job_args->gui_options->threads)
    {
		max_col = job_args->start_colum+job_args->width;

    }
    else
    {
		max_col = (job_args->width*job_args->gui_options->threads) - block_size;
    }


	printf("start row %d, start col %d, block size %d, max col %d\n",job_args->height-block_size,  job_args->start_colum, block_size, max_col);

    //me happy double nested loops

	for (row = 0; row < job_args->height-block_size ; row+=block_size)
    {
		for (col = job_args->start_colum; col < max_col; col+=block_size)
		{
			//set it all to zero for each block
			for(ii = 0; ii < 256; ii++)
			{
				histo.red[ii] = 0;
				histo.green[ii] = 0;
				histo.blue[ii] = 0;
			}

			total_min = 300;
			total_max = -1;
			red_min = 300;
			red_max = -1;
			green_min = 300;
			green_max = 0;
			blue_min = 300;
			blue_max = -1;
			temp = 0;
			largest_gap = -0;




			//calculate histogram by counting out how many of each color i have in the given box
			for (block_row = 0; block_row < block_size; block_row++)
			{
				for (block_col = 0; block_col < block_size; block_col++)
				{


					//i take the color value and use it as an index into my  historgram
					red_index = job_args->array_in[col+block_col][row+block_row].red;
					green_index = job_args->array_in[col+block_col][row+block_row].green;
					blue_index = job_args->array_in[col+block_col][row+block_row].blue;

					//i add another count to the given color in the array

					histo.red[red_index]++;
					histo.green[green_index]++;
					histo.blue[blue_index]++;


				}
			}

			//here i am going to figure out how wide each band of the histogram is and how wide the total histogram is and how much overlap the histogram has



			for(ii = 0; ii < 256; ii++)
			{

				if((histo.red[ii] != 0 || histo.green[ii] != 0 || histo.blue[ii] != 0 ) && ii < total_min)
				{
					total_min = ii;
				}

				//sets the single mins
				if(histo.red[ii] != 0 && ii < red_min)
				{
					red_min = ii;
				}
				if(histo.green[ii] != 0 && ii < green_min)
				{
					green_min = ii;
				}
				if(histo.blue[ii] != 0 && ii < blue_min)
				{
					blue_min = ii;
				}

			}

			for(ii = 255; ii > 0; ii--)
			{

				if((histo.red[ii] != 0 || histo.green[ii] != 0 || histo.blue[ii] != 0 ) && ii > total_max)
				{
					total_max = ii;
				}

				//sets the single mins
				if(histo.red[ii] != 0 && ii > red_max)
				{
					red_max = ii;
				}
				if(histo.green[ii] != 0 && ii > green_max)
				{
					green_max = ii;
				}
				if(histo.blue[ii] != 0 && ii > blue_max)
				{
					blue_max = ii;
				}

			}





			for(ii = total_min; ii < total_max; ii++)
			{
				if(histo.red[ii] == 0 && histo.green[ii] == 0 && histo.blue[ii] == 0 )
				{
					temp++;
				}
				else if(temp > largest_gap)
				{
					largest_gap = temp;
					temp = 0;
				}
				else
				{
					temp = 0;
				}
			}

			//	    for(ii = total_min; ii < total_max; ii++)
			//	    {
			//		if(histo.red[ii] == 0 && histo.green[ii] == 0 && histo.blue[ii] == 0 )
			//		{
			//		    temp++;
			//		}
			//		else if(temp > 0)
			//		{
			//		    largest_gap++;
			//		    temp = 0;
			//		}
			//	    }


			total_width = total_max-total_min;

			red_width = red_max-red_min;
			green_width = green_max-green_min;
			blue_width  = blue_max-blue_min;


			total_overlap = total_width - largest_gap;



			if(total_overlap < 0)
			{
				total_overlap = 0;
			}

			//write out my histogram to the output image

			for (block_row = 0; block_row < block_size; block_row++)
			{
				for (block_col = 0; block_col < block_size; block_col++)
				{



					job_args->array_out[col+block_col][row+block_row].red = red_width+green_width+blue_width;
					job_args->array_out[col+block_col][row+block_row].green = red_width+green_width+blue_width;
					job_args->array_out[col+block_col][row+block_row].blue = red_width+green_width+blue_width;

					//		    job_args->array_out[col+block_col][row+block_row].red = total_width;
					//		    job_args->array_out[col+block_col][row+block_row].green = total_width;
					//		    job_args->array_out[col+block_col][row+block_row].blue = total_width;




				}
			}


		}

		job_args->progress = (double)row / job_args->height;

    }






    return NULL;
}

GtkWidget * create_gui()
{
	label = gtk_label_new ("Histogram");

	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 75);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	histogram_check_button = gtk_check_button_new_with_label ( "Find Micro Histograms");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(histogram_check_button), FALSE);
	gtk_widget_show (histogram_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), histogram_check_button);


	histogram_block_size_spinbutton = gimp_spin_button_new (&histogram_block_size_spinbutton_value, gui_options.histogram_block_size, gui_options.histogram_block_size, 40, 4, 4, 4, 4, 0);
	gtk_container_add (GTK_CONTAINER (tab_box), histogram_block_size_spinbutton);
	gtk_widget_show (histogram_block_size_spinbutton);

	//  block_size_value = gtk_adjustment_new (90, 0, 256, 1, 1, 1);
	//  block_size_hscale = gtk_hscale_new (GTK_ADJUSTMENT (block_size_value));
	//  gtk_scale_set_digits( GTK_SCALE(texture_hscale), 3);
	////  gtk_range_set_update_policy      (GtkRange      *range,   GtkUpdateType  policy);
	//  gtk_widget_set_size_request (texture_hscale, 100, 40);
	//	 gtk_widget_show (texture_hscale);

	gtk_container_add (GTK_CONTAINER (tab_box), texture_hscale);

	//then add the page to the notbook

	return &tab_box;
}
