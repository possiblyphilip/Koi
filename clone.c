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

#include "clone.h"
#include "Koi.h"



//
//typedef struct
//{
//	int block_size;
//}CLONE_PARAMS;
//
//static CLONE_PARAMS clone_params;

int                 clone_condition_met = 0;
pthread_cond_t      clone_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     clone_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int wait_var = 1;

typedef struct
{
	int row;
	int col;
	int h_metric;
	int s_metric;
	int l_metric;
	int contrast_metric;
	int crc;

	int metric;
}CLONE_BLOCK_METRIC;

typedef struct
{
	int h;
	int s;
	int l;
}HSL;

HSL rgb_to_hsl( guchar r, guchar g, guchar b);

int clone_metric_comp(const void *a, const void *b);

gdouble clone_block_size = 16;
CLONE_BLOCK_METRIC *block_metric_array;

//#########################################################3

void * clone_highlighter_algorithm(JOB_ARG *job)
{

	int block_size;

	int block_row,block_col;
	int job_blocks;
	int temp = 0;
	int from_row, from_col;
	int num_blocks;
	int ii;

	int min, max;

	int max_col;

	HSL hsl;

	guchar red, green, blue;


	printf("inside %s thread %d\n", clone_plugin.name, job->thread);

	block_size = clone_block_size;




	//i only need to have the memory malloced once so i just have thread zero do it
	if(job->thread == 0)
	{

	//here i am creating my block metric array to store the metric that i am sorting each block by
		num_blocks = job->image.height*job->image.width;
		block_metric_array = (CLONE_BLOCK_METRIC*)malloc (num_blocks * sizeof(CLONE_BLOCK_METRIC));

		//set them all to zero
		for(ii = 0; ii < num_blocks; ii++)
		{
			block_metric_array[ii].h_metric = 0;
			block_metric_array[ii].s_metric = 0;
			block_metric_array[ii].l_metric = 0;
			block_metric_array[ii].metric = 0;
			block_metric_array[ii].col = 0;
			block_metric_array[ii].row = 0;
		}


		printf("numb blocks = %d\n", num_blocks);

		pthread_cond_broadcast(&clone_cond);
		pthread_mutex_lock(&clone_mutex);
		wait_var = 0;
		pthread_mutex_unlock(&clone_mutex);

	}
	else
	{
		printf("thread %d waiting\n", job->thread);
		while (wait_var)
		{
			pthread_mutex_lock(&clone_mutex);
			if (wait_var) pthread_cond_wait(&clone_cond, &clone_mutex);
		}
		pthread_mutex_unlock(&clone_mutex);
		printf("thread %d running\n", job->thread);
	}




//this should put the counter at the right place for each thread so each thread works on filling up its little slice of the array
	ii = job->image.height*job->image.width*(job->thread/(double)NUM_THREADS);

	printf("thread %d index start = %d\n", job->thread, ii);

	//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
	//basically this allows each thread to read the other threads data so there are not gaps between
	//the work each thread does
	if(job->start_colum+job->width+block_size < job->width* NUM_THREADS)
	{
		max_col = job->start_colum+job->width;
	}
	else
	{
		max_col = (job->width*NUM_THREADS) - block_size;
	}


	//me happy double nested loops
	//i just run through things right to left, bottom to top
	for (from_row = 0; from_row < job->height-block_size ; from_row++)
	{
		for (from_col = job->start_colum; from_col < max_col; from_col++)
		{

			//calculate metric
			min = 300;
			max = 0;
			//this way the block knows where it came from
			block_metric_array[ii].row = from_row;
			block_metric_array[ii].col = from_col;
			//loop through the block
			for (block_row = 0; block_row < block_size; block_row++)
			{
				for (block_col = 0; block_col < block_size; block_col++)
				{


					//summing the RGB color values should make blocks sort out different from each other
					block_metric_array[ii].metric += job->array_in[from_col+block_col][from_row+block_row].red;
					block_metric_array[ii].metric += job->array_in[from_col+block_col][from_row+block_row].green;
					block_metric_array[ii].metric += job->array_in[from_col+block_col][from_row+block_row].blue;


					//
					//			    red = job->array_in[from_col+block_col][from_row+block_row].red;
					//			    green = job->array_in[from_col+block_col][from_row+block_row].green;
					//			    blue = job->array_in[from_col+block_col][from_row+block_row].blue;
					//
					//
					//
					//			//    (red-blue)/255
					//
					////				    blue/255
					//
					//
					//			    if(red > max)
					//			    {
					//				max = red;
					//			    }
					//
					//			    if(green > max)
					//			    {
					//				max = green;
					//			    }
					//
					//			    if(blue > max)
					//			    {
					//				max = blue;
					//			    }
					//			//min
					//			    if(red < min)
					//			    {
					//				min = red;
					//			    }
					//			    if(green < min)
					//			    {
					//				min = green;
					//			    }
					//			    if(blue < min)
					//			    {
					//				min = blue;
					//			    }
					//
					//
					//	//		    block_metric_array[ii].metric += green;
					//			//	block_metric_array[ii].metric += 1;
					//
					//			    hsl = RGBtoHSL(red, green, blue);
					//
					//			    block_metric_array[ii].h_metric += hsl.h;
					//			    block_metric_array[ii].s_metric += hsl.s;
					//			    block_metric_array[ii].l_metric += hsl.l;

				}
			}
			ii++;
		}


//the height * 2 is because this is the first of 2 for loops
		job->progress = (double)from_row / (job->height * 2);
	}

	pthread_mutex_lock(&clone_mutex);
	wait_var++;
	pthread_mutex_unlock(&clone_mutex);



	if(job->thread == 0)
	{
		printf("starting  thread %d qsort\n", job->thread);
		//sort array using qsort functions

		//hang out and do nothign till all the threads are done
		while(wait_var < NUM_THREADS)
		{
		}

		qsort(block_metric_array, num_blocks, sizeof(CLONE_BLOCK_METRIC), clone_metric_comp);

		printf("done with qsort\n");

		pthread_cond_broadcast(&clone_cond);

		pthread_mutex_lock(&clone_mutex);
		wait_var = 0;
		pthread_mutex_unlock(&clone_mutex);
	}
	else
	{
		printf("thread %d waiting\n", job->thread);
		while (wait_var)
		{
			pthread_mutex_lock(&clone_mutex);
			if (wait_var) pthread_cond_wait(&clone_cond, &clone_mutex);
		}
		pthread_mutex_unlock(&clone_mutex);
		printf("thread %d running\n", job->thread);
	}




	//copying the original to my output so i can have the original image as my output image with the blocks written over it
		for (from_row = 0; from_row < job->height-block_size; from_row++)
		{
			for (from_col = job->start_colum; from_col < max_col; from_col++)
			{
				job->array_out[from_col][from_row].red = job->array_in[from_col][from_row].red;
				job->array_out[from_col][from_row].green = job->array_in[from_col][from_row].green;
				job->array_out[from_col][from_row].blue = job->array_in[from_col][from_row].blue;
			}
		}


	printf("matching in thread %d\n", job->thread);

	//this should put the counter at the right place for each thread so each thread works on filling up its little slice of the array
	ii = job->image.height*job->image.width*(job->thread/(double)NUM_THREADS);
	job_blocks = job->image.height*job->image.width*((job->thread+1)/(double)NUM_THREADS);

	printf("thread %d index start = %d job blocks %d\n", job->thread, ii, job_blocks);

	//finding matches and writing them out to the output image
	for(; ii< job_blocks; ii++)
	{
		//this disallows any blocks that are totally white or black
		if( block_metric_array[ii].metric != 0 &&  block_metric_array[ii].metric != 255*3*block_size*block_size)
		{
			//this checks to see if two blocks have the same metric
			if(block_metric_array[ii].metric == block_metric_array[ii-1].metric)
				//	    if( abs(block_metric_array[ii].metric - block_metric_array[ii-1].metric) < block_size*10)
			{
				//dissalows blocks from matching if they are too close
				if(abs(block_metric_array[ii].col-block_metric_array[ii-1].col) > block_size || abs(block_metric_array[ii].row-block_metric_array[ii-1].row) > block_size)
				{

					temp = 0;
					//subtracts the two blocks to see if they go to zero (perfect match)
					for (block_row = 0; block_row < block_size ; block_row++)
					{
						for (block_col = 0; block_col < block_size; block_col++)
						{

							temp += abs(job->array_in[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row].green - job->array_in[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row].green);

						}
					}
					//if the block matches close enough i let it through
					if(temp <= block_size*10)
					{
						//highlighting the blocks that match
						for (block_row = 0; block_row < block_size ; block_row++)
						{
							for (block_col = 0; block_col < block_size; block_col++)
							{

								job->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row].red = 50;
								job->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row].green = 190;
								job->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row].blue = 170;

								job->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row].red = 255;
								job->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row].green = 115;
								job->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row].blue = 0;

							}
						}
					}

				}

			}
		}

		job->progress = (double)ii / (num_blocks * 2) + .5;

	}

	pthread_mutex_lock(&clone_mutex);
	wait_var++;
	pthread_mutex_unlock(&clone_mutex);

	if(job->thread == 0)
	{
		//hang out and do nothign till all the threads are done
		while(wait_var < NUM_THREADS)
		{
		}
		free(block_metric_array);
	}

	job->progress = 1;

	return NULL;
}

//this is my function that qsort uses to compare things
int clone_metric_comp(const void *a, const void *b)

{
	CLONE_BLOCK_METRIC* ia = (CLONE_BLOCK_METRIC *)a;
	CLONE_BLOCK_METRIC* ib = (CLONE_BLOCK_METRIC *)b;
	return ia->metric  - ib->metric;
}

HSL rgb_to_hsl( guchar r, guchar g, guchar b)
{

	HSL temp;

	double L = 0;
	double S = 0;
	double H = 0;

	double max_color = 0;

	double min_color = 0;

	double r_percent = ((double)r)/255;
	double g_percent = ((double)g)/255;
	double b_percent = ((double)b)/255;


	if((r_percent >= g_percent) && (r_percent >= b_percent))
	{
	max_color = r_percent;
	}

	if((g_percent >= r_percent) && (g_percent >= b_percent))

	{
	max_color = g_percent;
	}

	if((b_percent >= r_percent) && (b_percent >= g_percent))
	{

	max_color = b_percent;
	}

	if((r_percent <= g_percent) && (r_percent <= b_percent))
	{
	min_color = r_percent;
	}

	if((g_percent <= r_percent) && (g_percent <= b_percent))
	{
	min_color = g_percent;
	}

	if((b_percent <= r_percent) && (b_percent <= g_percent))
	{
	min_color = b_percent;
	}

	L = (max_color + min_color)/2;

	if(max_color == min_color)
	{
	S = 0;
	H = 0;
	}
	else
	{
	if(L < .50)
	{
		S = (max_color - min_color)/(max_color + min_color);
	}
	else
	{
		S = (max_color - min_color)/(2 - max_color - min_color);
	}

	if(max_color == r_percent)
	{
		H = (g_percent - b_percent)/(max_color - min_color);
	}

	if(max_color == g_percent)
	{
		H = 2 + (b_percent - r_percent)/(max_color - min_color);
	}

	if(max_color == b_percent)
	{
		H = 4 + (r_percent - g_percent)/(max_color - min_color);
	}
	}

	temp.s = (int)(S*100);
	temp.l = (int)(L*100);

	H = H*60;
	if(H < 0)
	{
	H += 360;
	}

	temp.h  = (int)H;

	return temp;
}

/* Our usual callback function */
static void cb_clone_check_button( GtkWidget *widget)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))
	{
		clone_plugin.checked = 1;
	}
	else
	{
		clone_plugin.checked = 0;
	}
}

GtkWidget* create_clone_gui()
{

	printf("creating clone gui\n");
	GtkWidget *label;
	GtkWidget *tab_box;
	GtkWidget *clone_check_button;
	GtkWidget *block_size_spinbutton;
	GtkObject *block_size_spinbutton_value;

	label = gtk_label_new ("Clone");

	//so this is the page
	//  frame = gtk_frame_new ("Clone tool use");
	tab_box = gtk_vbox_new (FALSE, 6);

	gtk_container_border_width (GTK_CONTAINER (tab_box), 10);
	gtk_widget_set_size_request (tab_box, 200, 75);
	gtk_widget_show (tab_box);
	//this is the button i want to add to the page
	clone_check_button = gtk_check_button_new_with_label ( "Find Cloning");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clone_check_button), FALSE);
	gtk_widget_show (clone_check_button);
	//i add the button to the page
	gtk_container_add (GTK_CONTAINER (tab_box), clone_check_button);


	block_size_spinbutton = gimp_spin_button_new (&block_size_spinbutton_value, clone_block_size, 4, 40, 4, 4, 0, 4, 0);
	gtk_container_add (GTK_CONTAINER (tab_box), block_size_spinbutton);
	gtk_widget_show (block_size_spinbutton);

	g_signal_connect (clone_check_button, "clicked", G_CALLBACK (cb_clone_check_button), &clone_plugin.checked);
	g_signal_connect (block_size_spinbutton_value, "value_changed", G_CALLBACK (gimp_int_adjustment_update), &clone_block_size);

	//then add the page to the notbook

	printf("clone gui created\n");

	return tab_box;
}

void create_clone_plugin()
{
	printf("creating clone plugin\n");
	clone_plugin.checked = FALSE;
	clone_plugin.name = "Clone Highliter";
	clone_plugin.label = gtk_label_new (clone_plugin.name);
	clone_plugin.algorithm = &clone_highlighter_algorithm;
	clone_plugin.create_gui = &create_clone_gui;
//	clone_plugin.options = &clone_block_size;
	printf("clone plugin created\n");

}

