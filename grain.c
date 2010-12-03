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

#include"grain.h"

void * grain(void *pArg)
{

    int radius;

    float angle;

    float best_angle;
    int highest;


    int ii;
    int counter = 0;
  //  guchar temp;
    int row, col, temp;
    int row_offset;
    int col_offset;

    int max_col;
    int min_col;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;

    radius = job_args->gui_options.radius;

    //this snipit should let the colums blend in the middle of the image without writing over the edge of the image
	if(job_args->start_colum+job_args->width+radius < job_args->width*job_args->gui_options.threads)
	{
	    max_col = job_args->start_colum+job_args->width;

	}
	else
	{
	    max_col = (job_args->width*job_args->gui_options.threads) - radius;
	}


//if im at the left wall i need to start over at least one radius so i dont run off the page
	if(job_args->start_colum == 0)
	{
	    min_col = radius;

	}
	else
	{
	    min_col = job_args->start_colum;
	}

    //set my image to black

    for (row = 0; row < job_args->height; row++)
    {
	for (col = job_args->start_colum; col < job_args->start_colum+job_args->width; col++)
	{
	    job_args->array_out[col][row][0] = 0;
	    job_args->array_out[col][row][1] = 0;
	    job_args->array_out[col][row][2] = 0;
	}
    }


    for (row = radius; row < job_args->height-radius ; row+=radius/4)
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
		    temp +=  job_args->array_in[col+col_offset][row+row_offset][0];
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
//		job_args->array_out[col+col_offset][row+row_offset][0] = 230;
//		job_args->array_out[col+col_offset][row+row_offset][1] = 230;
//		job_args->array_out[col+col_offset][row+row_offset][2] = 50;


		if(job_args->array_out[col+col_offset][row+row_offset][0] > highest/radius)
		{
		    highest = 0;
		}



	    }

	    if(best_angle > 3.14)
	    {
		best_angle-=3.14;
	    }


	    if(highest > 0)
	    {
		for(ii = 0; ii < radius; ii++)
		{
		    col_offset = (ii*cos(best_angle));
		    row_offset = (ii*sin(best_angle));
		    //		job_args->array_out[col+col_offset][row+row_offset][0] = 230;
		    //		job_args->array_out[col+col_offset][row+row_offset][1] = 230;
		    //		job_args->array_out[col+col_offset][row+row_offset][2] = 50;

		    job_args->array_out[col+col_offset][row+row_offset][0] = highest/radius;
		    job_args->array_out[col+col_offset][row+row_offset][1] = (best_angle/(3.14))*255;
		    job_args->array_out[col+col_offset][row+row_offset][2] = highest/radius;

		}
	    }


	}


	if (row % 100 == 0)
	{
	    gimp_progress_update (row/job_args->height-radius);
	}

    }


    return NULL;
}
