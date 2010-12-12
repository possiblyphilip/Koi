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

#include"speckle.h"

void * speckle(void *pArg)
{

    int slider_size;


    int counter = 0;
    int temp = 0;
    int from_row, from_col;

    int ii;

    int amplitude;


    int max_col;

    gboolean up = FALSE;




    //get the argument passed in, and set our local variables
    JOB_ARG* job_args = (JOB_ARG*)pArg;


	slider_size = job_args->gui_options->clone_block_size;


    color_struct color_slider[slider_size];



    for(ii = 0; ii < slider_size; ii++)
    {
		color_slider[ii].red = 0;
		color_slider[ii].green = 0;
		color_slider[ii].blue = 0;
    }





	//this snipit should let the colums blend in the middle of the image without writing over the edge of the image
	if(job_args->start_colum+job_args->width+slider_size < job_args->width*job_args->gui_options->threads)
    {
		max_col = job_args->start_colum+job_args->width;

    }
    else
    {
		max_col = (job_args->width*job_args->gui_options->threads) - slider_size;
    }

	//set my whole block to black
    for (from_row = 0; from_row < job_args->height; from_row++)
    {
    	for (from_col = job_args->start_colum; from_col < max_col; from_col++)
    	{
			job_args->array_out[from_col][from_row].red = 0;
			job_args->array_out[from_col][from_row].green = 0;
			job_args->array_out[from_col][from_row].blue = 0;

		}
    }


    //me happy double nested loops

    for (from_row = 0; from_row < job_args->height-slider_size ; from_row++)
    {
		for (from_col = job_args->start_colum; from_col < max_col; from_col++)
		{

			color_slider[counter].red = job_args->array_in[from_col][from_row].red;
			color_slider[counter].green = job_args->array_in[from_col][from_row].green;
			color_slider[counter].blue = job_args->array_in[from_col][from_row].blue;


			//this should be the last element in the array because it will be the next one i will write over
			temp = counter+1;
			temp%=slider_size;

			//	    job_args->array_out[from_col][from_row].red = abs (color_slider[temp].red - color_slider[counter].red);

			//check to see if the end of the slider is brigher than the beginning
			if(color_slider[temp].red < color_slider[counter].red)
			{
				//if its brigher and we are already going up i add to my amplitude
				if(up == TRUE)
				{
					job_args->array_out[from_col][from_row].red = amplitude;
					amplitude++;
				}
				else
				{
					//it was goind down and turned up so we need to start the amplitude over again and write out the length of the drop
					job_args->array_out[from_col][from_row].blue = amplitude;

					up = TRUE;
					amplitude = 0;

				}
			}
			else
				//it is getting darker
			{
				if(up == FALSE)
					//it was already getting darker so ill add to its amplitude
				{
					job_args->array_out[from_col][from_row].blue = amplitude;
					amplitude++;
				}
				else
					//it just turned down and so i need to mark the brightness of the top
				{
					job_args->array_out[from_col][from_row].red = amplitude;

					up = FALSE;
					amplitude = 0;
				}
			}

			counter++;
			counter%=slider_size;
		}

    }






    return NULL;
}
