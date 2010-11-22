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


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;

    radius = job_args->gui_options.radius;

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


    for (row = radius; row < job_args->height-radius ; row+=radius/2)
    {
	for (col = job_args->start_colum+radius; col < job_args->start_colum+job_args->width-radius; col+=radius/2)
	{
	    highest = 0;

	    for(angle = 0; angle < 3.14; angle+= .01)
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
//write back the best line in yellow
	    for(ii = 0; ii < radius; ii++)
	    {
		col_offset = (ii*cos(best_angle));
		row_offset = (ii*sin(best_angle));
//		job_args->array_out[col+col_offset][row+row_offset][0] = 230;
//		job_args->array_out[col+col_offset][row+row_offset][1] = 230;
//		job_args->array_out[col+col_offset][row+row_offset][2] = 50;

		job_args->array_out[col+col_offset][row+row_offset][0] = highest/radius;
		job_args->array_out[col+col_offset][row+row_offset][1] = highest/radius;
		job_args->array_out[col+col_offset][row+row_offset][2] = highest/radius;


	    }


	}
    }


    return NULL;
}
