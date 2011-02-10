#include "Koi.h"

#ifndef FLOOD
#define FLOOD

typedef struct
{
	int row;
	int col;
}FLOOD_TYPE;

int test(PIXEL pixel)
{
	if(pixel.red > 100)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int flood(JOB_ARG *job)
{

	int offset_col, offset_row;
	int sum = 1;
	PIXEL pixel;
	FLOOD_TYPE *center_point;
	FLOOD_TYPE new_point;


	center_point = (FLOOD_TYPE*)job->options;


	//this makes sure that the center wont get recursed on
	job->array_in[center_point->col][center_point->row].red = 0;
	job->array_in[center_point->col][center_point->row].green = 0;
	job->array_in[center_point->col][center_point->row].blue = 0;

//		printf("image height %d image width %d\n",job->image.height, job->image.width);


	for(offset_row = -1; offset_row <= 1; offset_row++)
	{
		for(offset_col = -1; offset_col <= 1; offset_col++)
		{
//makes sure im not running off the edge of the image
			if(offset_col+center_point->col >= 0)
			{
				if( offset_col+center_point->col < job->image.width )
				{
					if( offset_row+center_point->row >= 0 )
					{
						if(offset_row+center_point->row < job->image.height)
						{

							pixel.red = job->array_in[offset_col+center_point->col][offset_row+center_point->row].red;
							pixel.green = job->array_in[offset_col+center_point->col][offset_row+center_point->row].green;
							pixel.blue = job->array_in[offset_col+center_point->col][offset_row+center_point->row].blue;

							if(test(pixel))
							{
//															printf("thread %d: pixel %d,%d\n",job->thread, center_point->col,center_point->row);
								job->array_in[offset_col+center_point->col][offset_row+center_point->row].red = 0;
								job->array_in[offset_col+center_point->col][offset_row+center_point->row].green = 0;
								job->array_in[offset_col+center_point->col][offset_row+center_point->row].blue = 0;
								new_point.col = offset_col+center_point->col;
								new_point.row = offset_row+center_point->row;
								job->options = &new_point;
								sum += flood(job);

								if(sum > 255)
								{
									sum = 255;
								}
							}
						}
					}
				}
			}
		}
	}

	job->array_out[center_point->col][center_point->row].red = sum;
	job->array_out[center_point->col][center_point->row].green = sum;
	job->array_out[center_point->col][center_point->row].blue = sum;

	return sum;
}
#endif //FLOOD
