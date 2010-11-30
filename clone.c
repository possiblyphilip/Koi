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

    int max_col;

    HSL hsl;

    guchar red, green, blue;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;

    block_size = job_args->gui_options.clone_block_size;

    clone_block_metric *block_metric_array;
    clone_block_metric slider[block_size];

//here i am creating my block metric array to store the metric that i am sorting each block by
    num_blocks = job_args->height*job_args->width;
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


//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
//basically this allows each thread to read the other threads data so there are not gaps between
//the work each thread does
    if(job_args->start_colum+job_args->width+block_size < job_args->width*job_args->gui_options.threads)
    {
	max_col = job_args->start_colum+job_args->width;
    }
    else
    {
	max_col = (job_args->width*job_args->gui_options.threads) - block_size;
    }


//me happy double nested loops
//i just run through things right to left, bottom to top
    for (from_row = 0; from_row < job_args->height-block_size ; from_row++)
    {
	for (from_col = job_args->start_colum; from_col < max_col; from_col++)
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
	    ii++;	    
	}
    }



//sort array using qsort functions
    qsort(block_metric_array, num_blocks, sizeof(clone_block_metric), clone_metric_comp);

//copying the original to my output so i can have the original image as my output image with the blocks written over it
    for (from_row = 0; from_row < job_args->height-block_size; from_row++)
    {
    	for (from_col = job_args->start_colum; from_col < max_col; from_col++)
    	{
	    job_args->array_out[from_col][from_row][0] = job_args->array_in[from_col][from_row][0];
	    job_args->array_out[from_col][from_row][1] = job_args->array_in[from_col][from_row][1];
	    job_args->array_out[from_col][from_row][2] = job_args->array_in[from_col][from_row][2];
	}
    }



//finding matches and writing them out to the output image
    for(ii = 1; ii< num_blocks; ii++)
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

			    temp += abs(job_args->array_in[block_metric_array[ii].col+block_col][ block_metric_array[ii].row+block_row][1] - job_args->array_in[ block_metric_array[ii-1].col+block_col][block_metric_array[ii-1].row+block_row][1]);

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

    return NULL;
}

//this is my function that qsort uses to compare things
int clone_metric_comp(const void *a, const void *b)

{
	    clone_block_metric* ia = (clone_block_metric *)a;
	    clone_block_metric* ib = (clone_block_metric *)b;
	    return ia->metric  - ib->metric;
}
