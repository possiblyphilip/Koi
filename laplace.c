#ifndef LAPLACE
#define LAPLACE

void laplace(JOB_ARG *job)
{

	int offset_col, offset_row;
	int SUM;
	int	MASK[5][5];
	int row, col;
	int temp;
	int max_col;

	/* 5x5 Laplace mask.  Ref: Myler Handbook p. 135 */
	MASK[0][0] = -1; MASK[0][1] = -1; MASK[0][2] = -1; MASK[0][3] = -1; MASK[0][4] = -1;
	MASK[1][0] = -1; MASK[1][1] = -1; MASK[1][2] = -1; MASK[1][3] = -1; MASK[1][4] = -1;
	MASK[2][0] = -1; MASK[2][1] = -1; MASK[2][2] = 24; MASK[2][3] = -1; MASK[2][4] = -1;
	MASK[3][0] = -1; MASK[3][1] = -1; MASK[3][2] = -1; MASK[3][3] = -1; MASK[3][4] = -1;
	MASK[4][0] = -1; MASK[4][1] = -1; MASK[4][2] = -1; MASK[4][3] = -1; MASK[4][4] = -1;

	if(job->start_colum+job->width+2 < job->image.width)
	{
		max_col = job->start_colum+job->width+2;

	}
	else
	{
		max_col = job->image.width-2;
	}

	for(row = 2; row < job->height-2; row++)
	{
		for(col = job->start_colum+2; col < max_col; col++)
		{
			SUM = 0;

			for(offset_row = -2; offset_row <= 2; offset_row++)
			{
				for(offset_col = -2; offset_col <= 2; offset_col++)
				{
					temp = 0;

					temp += job->array_in[col+offset_col][row+offset_row].red;
					temp += job->array_in[col+offset_col][row+offset_row].green;
					temp += job->array_in[col+offset_col][row+offset_row].blue;

					temp /= 3.0;

					SUM += temp * MASK[offset_col+2][offset_row+2];

				}
			}

			SUM = abs(SUM);

			if(SUM > 255)
			{
				SUM = 255;
			}


			job->array_out[col][row].red = SUM;
			job->array_out[col][row].green = SUM;
			job->array_out[col][row].blue = SUM;
		}
	}
}
#endif //LAPLACE
