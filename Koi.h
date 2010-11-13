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

#include <stdlib.h>

typedef struct
{
    gboolean preview;
    gboolean texture_checked;
	int texture_threshold;
    gboolean clone_checked;
} GUI_values;

typedef struct
{
    guchar ***array_in;
    guchar ***array_out;
    int start_row;
    int start_colum;
    int height;
    int width;
    float percent;
    GUI_values gui_options;
}JOB_ARG;

/* Set up default values for options */
static GUI_values gui_options =
{
  0,  //preview
  0,   //texture
    0   //clone
};

static void query (void);
static void run (const char *name, int nparams, const GimpParam *param,  int *nreturn_vals, GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};


static void koi (GimpDrawable  *drawable, GimpPreview  *preview);


static gboolean koi_dialog (GimpDrawable *drawable);


void * find_blur_job(void *pArg);
void * find_clone_job(void *pArg);
void free_pixel_array(guchar ***array, int width, int height, int depth);
void allocate_pixel_array(guchar ****array, int width, int height, int depth);







#endif // KOI_H
