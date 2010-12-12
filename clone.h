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


#ifndef CLONE_H
#define CLONE_H

void * find_clone_job(void *pArg);



typedef struct
{
    int row;
    int col;
    int h_metric;
    int s_metric;
    int l_metric;
    int contrast_metric;
    int crc;

    int metric;
}clone_block_metric;

typedef struct
{
    int h;
    int s;
    int l;
}HSL;

HSL rgb_to_hsl( guchar r, guchar g, guchar b);

int clone_metric_comp(const void *a, const void *b);

#endif // CLONE_H
