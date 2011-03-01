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


#include "Koi.h"

#include "queue.c"
#include <assert.h>

#ifndef FLOOD
#define FLOOD

typedef struct
{
	int row;
	int col;
}POINT_TYPE;

//$$$$$$$$$$$$$$$$$$$$$$$$$44

typedef struct NODE
{
	struct NODE *p_next;
	struct NODE *p_prev;
	POINT_TYPE point;
}NODE_TYPE;

typedef struct
{
	NODE_TYPE *p_first;
	NODE_TYPE *p_last;
}QUEUE_TYPE;

void make_queue(QUEUE_TYPE *queue)
{
	queue->p_first = 0;
	queue->p_last = 0;

}


void enqueue(QUEUE_TYPE *queue, POINT_TYPE point)
{
	NODE_TYPE *new_node;

	new_node = (NODE_TYPE*)malloc (sizeof(NODE_TYPE));
//	printf("malloc %p\n",new_node);
	new_node->point = point;

	if(queue->p_first == 0)
	{
		queue->p_first = new_node;
		queue->p_last = new_node;
		new_node->p_next = 0;
		new_node->p_prev = 0;
	}
	else
	{

		queue->p_last->p_next = new_node;
		new_node->p_next = 0;
		queue->p_last = new_node;
	}
}

POINT_TYPE dequeue(QUEUE_TYPE *queue)
{

	NODE_TYPE temp_node;

	assert(queue != NULL && queue->p_first != NULL);

	if (queue->p_first == NULL)
	{
		printf("oops\n");
		exit(-1);
	}
	else
	{
		temp_node.p_next = queue->p_first->p_next;
		temp_node.point = queue->p_first->point;


//		printf("free   %p\n",queue->p_first);

		free(queue->p_first);
		queue->p_first = temp_node.p_next;
		if (queue->p_first==NULL)
		{
			queue->p_last = NULL;
		}
		return temp_node.point;
	}
}

int queue_is_empty(QUEUE_TYPE queue)
{
	if(queue.p_first == 0)
	{
		if(queue.p_last != 0)
		{
			printf("queue got messed up\n");
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

//$$$$$$$$$$$$$$$$$$$$$$$$$44

int test(PIXEL pixel)
{
	if(pixel.red > 250)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void * flood(JOB_ARG *job)
{
	int max_col, min_col;
	int max_row, min_row;
	int offset_col, offset_row;
	int count = 0;
	PIXEL pixel;
	POINT_TYPE *start_point;
	POINT_TYPE new_point;
	POINT_TYPE temp_point;

	QUEUE_TYPE search_queue;
	QUEUE_TYPE flood_queue;

	max_col = 0;
	max_row = 0;

	min_col = INT_MAX;
	min_row = INT_MAX;

	make_queue(&search_queue);
	make_queue(&flood_queue);

	start_point = (POINT_TYPE*)job->options;


	//this makes sure that the center wont get re called
//	job->array_in[start_point->col][start_point->row].red = 255;
//	job->array_in[start_point->col][start_point->row].green = 255;
//	job->array_in[start_point->col][start_point->row].blue = 255;

	job->array_in[start_point->col][start_point->row].red = 0;
	job->array_in[start_point->col][start_point->row].green = 0;
	job->array_in[start_point->col][start_point->row].blue = 0;


//	printf("start point %d %d\n", start_point->col, start_point->row);

	enqueue(&search_queue, *start_point);

	while(!queue_is_empty(search_queue))
	{
		count++;
		//		printf("count %d\n", count);

		temp_point = dequeue(&search_queue);
		//		printf("temp point %d %d\n", temp_point.col, temp_point.row);

		enqueue(&flood_queue, temp_point);
		//		printf("added point to flood fill list\n");




		for(offset_row = -1; offset_row <= 1; offset_row++)
		{
			for(offset_col = -1; offset_col <= 1; offset_col++)
			{

				//makes sure im not running off the edge of the image
				if(offset_col+temp_point.col >= 0 &&
				   offset_col+temp_point.col < job->image.width &&
				   offset_row+temp_point.row >= 0 &&
				   offset_row+temp_point.row < job->image.height)
				{
					if(offset_row+offset_col == 1 || offset_row+offset_col == -1)
					{

						//					printf("col %d row %d\n",offset_col+temp_point.col,offset_row+temp_point.row);

						pixel.red = job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].red;
						pixel.green = job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].green;
						pixel.blue = job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].blue;

						if(test(pixel))
						{
							//this makes sure that this point wont get re called
//							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].red = 255;
//							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].green = 255;
//							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].blue = 255;

							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].red = 0;
							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].green = 0;
							job->array_in[offset_col+temp_point.col][offset_row+temp_point.row].blue = 0;





							new_point.col = offset_col+temp_point.col;
							new_point.row = offset_row+temp_point.row;


							//i will use this later to write out the width and height of the speckle
							if(new_point.col > max_col)
							{
								max_col = new_point.col;
							}
							if(new_point.col < min_col)
							{
								min_col = new_point.col;
							}
							if(new_point.row > max_row)
							{
								max_row = new_point.row;
							}
							if(new_point.row < min_row)
							{
								min_row = new_point.row;
							}

							enqueue(&search_queue,new_point);

							//						printf("enqueued a new search point\n");


						}
					}
				}
			}
		}
	}

	while(!queue_is_empty(flood_queue))
	{
		temp_point = dequeue(&flood_queue);

// size of the speckle is written out as the green value
		if(count < 765)
		{
			job->array_out[temp_point.col][temp_point.row].green = count/3;
		}
		else
		{
			job->array_out[temp_point.col][temp_point.row].green = 255;
		}

		//width of the speckle is written out as its red value
		if(max_col-min_col < 256)
		{
			job->array_out[temp_point.col][temp_point.row].red = max_col-min_col;
		}
		else
		{
			job->array_out[temp_point.col][temp_point.row].red = 255;
		}

		//height of the speckle is written out as its blue value
		if(max_row-min_row < 256)
		{
			job->array_out[temp_point.col][temp_point.row].blue = max_row-min_row;
		}
		else
		{
			job->array_out[temp_point.col][temp_point.row].blue = 255;
		}

	}
}
#endif //FLOOD
