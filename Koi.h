

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

#ifndef KOI_H
#define KOI_H

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <pthread.h>
#include <stdlib.h>

#include "gui.h"
#include "log.c"

static void query (void);
static void run (const char *name, int nparams, const GimpParam *param,  int *nreturn_vals, GimpParam **return_vals);

//dont know what this is doing either
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

typedef struct
{
	guchar red;
	guchar green;
	guchar blue;
}PIXEL;


typedef struct
{
	int height;
	int width;
}IMAGE;

typedef struct
{
	PIXEL **array_in;
	PIXEL **array_out;
	gint32 image_id;
	GimpDrawable *drawable;
	int start_row;
	int start_colum;
	int height;
	int width;
	double progress;
	int thread;
	IMAGE image;
	void *options;
	gchar *file_name;
}JOB_ARG;

typedef struct
{
	int checked;
	GtkWidget *label;
	char* name;
	void * (*algorithm)(JOB_ARG *);
	void * (*analyze)(JOB_ARG *);
	GtkWidget* (*create_gui)();
	void *options;
}KOI_PLUGIN;

static void koi (GimpDrawable  *drawable, GimpPreview  *preview);
static gboolean koi_dialog (GimpDrawable *drawable);

void allocate_pixel_array(PIXEL ***array, int width, int height);
void free_pixel_array(PIXEL **array, int width);

int NUM_THREADS = 4;

#endif // KOI_H
