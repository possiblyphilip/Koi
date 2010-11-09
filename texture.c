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

void * find_blur_job(void *pArg)
{

    int size = 20;

    guchar slider[20];
    int ii;
    int counter = 0;
    guchar temp;
    int row, col, channel;


    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;

//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
	slider[ii] = 0;
    }



    for (row = 0; row < job_args->height ; row++)
    {
	for (col = job_args->start_colum; col < job_args->start_colum+job_args->width; col++)
	{
//set the current element in the slider to our newest pixel value
	    slider[counter] = job_args->array_in[col][row][0];
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
	    if(temp < 90)
	    {
		if(job_args->array_out[col][row][0] == 255 && job_args->array_out[col][row][1] == 255 && job_args->array_out[col][row][2] == 255)
		{
				//pixels are all white and info is gone
		    job_args->array_out[col][row][0] = 110;
		    job_args->array_out[col][row][1] = 255;
		    job_args->array_out[col][row][2] = 90;


		}

		else
		{
		    if(job_args->array_out[col][row][0] == 0 && job_args->array_out[col][row][1] == 0 && job_args->array_out[col][row][2] == 0)
		    {
			//pixels are all black and info is gone
			job_args->array_out[col][row][0] = 30;
			job_args->array_out[col][row][1] = 75;
			job_args->array_out[col][row][2] = 30;
		    }
		    else
		    {
			job_args->array_out[col][row][0] = 255;
			job_args->array_out[col][row][1] = 0;
			job_args->array_out[col][row][2] = 0;
//			job_args->array_out[col][row][0] = temp;
//			job_args->array_out[col][row][1] = temp;
//			job_args->array_out[col][row][2] = temp;

		    }
		}
	    }
	    //pixels are good
	    else
	    {
		job_args->array_out[col][row][0] = 80;
		job_args->array_out[col][row][1] = 190;
		job_args->array_out[col][row][2] = 70;

	    }

	    //this will reset my slider counter so i dont have to make a queue or anything slow like that
	    counter++;
	    counter%=size;

	}
    }

    //###################
// now do the same image block virtically
//set my slider to zero
    for(ii = 0; ii < size; ii++)
    {
	slider[ii] = 0;
    }


    for (col = job_args->start_colum; col < job_args->start_colum+job_args->width; col++)
    {
	for (row = 0; row < job_args->height ; row++)
	{

	    //set the current element in the slider to our newest pixel value
	    slider[counter] = job_args->array_in[col][row][0];
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
	    if(temp < 90)
	    {
//if the pixel is already green we dont want to set it to red, we will set it to grey instead
		if(job_args->array_out[col][row][1] == 75 || job_args->array_out[col][row][1] == 255  || job_args->array_out[col][row][1] == 190 )
		{
		    job_args->array_out[col][row][0] = 80;
		    job_args->array_out[col][row][1] = 190;
		    job_args->array_out[col][row][2] = 70;
		}
		else
		{
		    job_args->array_out[col][row][0] = 255;
		    job_args->array_out[col][row][1] = 0;
		    job_args->array_out[col][row][2] = 0;
//		    job_args->array_out[col][row][0] = temp;
//		    job_args->array_out[col][row][1] = temp;
//		    job_args->array_out[col][row][2] = temp;
		}
	    }
	    else
	    {
		job_args->array_out[col][row][0] = 80;
		job_args->array_out[col][row][1] = 190;
		job_args->array_out[col][row][2] = 70;
	    }

	    //this will reset my slider counter so i dont have to make a queue or anything slow like that
	    counter++;
	    counter%=size;

	}
    }

    return NULL;
}
