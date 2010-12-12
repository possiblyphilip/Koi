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

#include"dy_con.h"

void * dy_con(void *pArg)
{

    int size = 40;

    guchar left_slider[size];
    guchar right_slider[size];
    int ii;
    int counter = 0;
	//  guchar temp;
    int row, col, temp;
    int left_sum;
    int right_sum;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;


    for (row = 0; row < job_args->height ; row++)
    {

		//set my sliders to zero to wipe out old data at the end of each row
		for(ii = 0; ii < size; ii++)
		{
			left_slider[ii] = 0;
			right_slider[ii] = 0;
		}

		for (col = job_args->start_colum; col < job_args->start_colum+job_args->width-size; col++)
		{
			//set the current element in the sliders to our newest pixel values
			left_slider[counter] = job_args->array_in[col][row].red;
			right_slider[counter] = job_args->array_in[col+size][row].red;
			left_sum = 0;
			right_sum = 0;

			for(ii = 0; ii < size; ii++)
			{

				left_sum += left_slider[ii];
				right_sum += right_slider[ii];
			}

			temp = abs((left_sum/size)-(right_sum/size));

			temp =(1-(1/(1+.008*temp*temp)))*200;

			job_args->array_out[col][row].red = temp;
			job_args->array_out[col][row].green = temp;
			job_args->array_out[col][row].blue = temp;

			//this will reset my slider counter so i dont have to make a queue or anything slow like that
			counter++;
			counter%=size;

		}
    }


    return NULL;
}
