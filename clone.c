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

void * find_clone_job(void *pArg)
{

    int block_size;

    int block_row,block_col;
    int counter = 0;
    int temp = 0;
    int from_row, to_row, from_col, to_col;
    int num_blocks;
    int ii;

    int min, max;

    HSL hsl;

    guchar red, green, blue;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;


    block_size = job_args->gui_options.clone_block_size;

    printf("block size %d\n", block_size);

    clone_block_metric *block_metric_array;

    clone_block_metric slider[block_size];





    //this should create a few more blocks than im going to need but memory is cheap :)
    num_blocks = job_args->height*job_args->width;
  //  num_blocks *=2;


    block_metric_array = (clone_block_metric*)malloc (num_blocks * sizeof(clone_block_metric));

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

    ii = 0;

    for (from_row = 0; from_row < job_args->height-block_size ; from_row++)
    {
	for (from_col = job_args->start_colum; from_col < job_args->start_colum+job_args->width-block_size; from_col++)
	{

	    //calculate metric (just summing the gren values)
	    min = 300;
	    max = 0;
	    block_metric_array[ii].row = from_row;
	    block_metric_array[ii].col = from_col;
	    for (block_row = 0; block_row < block_size; block_row++)
	    {
		for (block_col = 0; block_col < block_size; block_col++)
		{


			    //this should make the color spaces sort out different from each other
			    block_metric_array[ii].metric += job_args->array_in[from_col+block_col][from_row+block_row][0];
			    block_metric_array[ii].metric += job_args->array_in[from_col+block_col][from_row+block_row][1];
			    block_metric_array[ii].metric += job_args->array_in[from_col+block_col][from_row+block_row][2];
//
//			    red = job_args->array_in[from_col+block_col][from_row+block_row][0];
//			    green = job_args->array_in[from_col+block_col][from_row+block_row][1];
//			    blue = job_args->array_in[from_col+block_col][from_row+block_row][2];
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

	    if(ii> num_blocks)
	    {
		g_message("didnt make enough blocks\n");
	    }

	    //	    block_metric_array[ii].metric += ii;
	    ii++;

	}
    }

//		    g_message("made n blocks\n");

    /* sort array using qsort functions */
    qsort(block_metric_array, num_blocks, sizeof(clone_block_metric), clone_metric_comp);

//		    g_message("sorted them\n");





    for (from_row = 0; from_row < job_args->height-block_size; from_row++)
    {
    	for (from_col = job_args->start_colum; from_col < job_args->start_colum+job_args->width-block_size; from_col++)
    	{
	    job_args->array_out[from_col][from_row][0] = job_args->array_in[from_col][from_row][0];
	    job_args->array_out[from_col][from_row][1] = job_args->array_in[from_col][from_row][1];
	    job_args->array_out[from_col][from_row][2] = job_args->array_in[from_col][from_row][2];

	}
    }




    for(ii = 1; ii< num_blocks; ii++)
    {
	if( block_metric_array[ii].metric != 0 &&  block_metric_array[ii].metric != 255*3*block_size*block_size)
	{

	    if( block_metric_array[ii].metric ==  block_metric_array[ii-1].metric)
	    {
		if(abs(block_metric_array[ii].col-block_metric_array[ii-1].col) > block_size || abs(block_metric_array[ii].row-block_metric_array[ii-1].row) > block_size)
		{

		    temp = 0;
		    for (block_row = 0; block_row < block_size ; block_row++)
		    {
			for (block_col = 0; block_col < block_size; block_col++)
			{

			    temp += abs(job_args->array_in[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row][1] - job_args->array_in[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row][1]);

			}
		    }
		    if(temp <= block_size*10)
		    {
			for (block_row = 0; block_row < block_size ; block_row++)
			{
			    for (block_col = 0; block_col < block_size; block_col++)
			    {

				job_args->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row][0] = 50;
				job_args->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row][1] = 190;
				job_args->array_out[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row][2] = 170;

				job_args->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row][0] = 255;
				job_args->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row][1] = 115;
				job_args->array_out[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row][2] = 0;

			    }
			}
		    }

		}

	    }
	}

    }

    //		    g_message("survived\n");





    return NULL;
}

int clone_metric_comp(const void *a, const void *b)

{
	    clone_block_metric* ia = (clone_block_metric *)a; // casting pointer types
	    clone_block_metric* ib = (clone_block_metric *)b;
	    return ia->metric  - ib->metric;

		/* integer comparison: returns negative if b > a
		and positive if a > b */
}
