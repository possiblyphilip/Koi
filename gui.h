#include "Koi.h"

#ifndef GUI_H
#define GUI_H

typedef struct
{
    gboolean preview;
    gboolean texture_checked;
	int texture_threshold;
		float compress;
		int radius;
	int clone_block_size;
    gboolean clone_checked;
     gboolean grain_checked;
     gboolean jpeg_checked;
} GUI_values;

static gboolean koi_dialog (GimpDrawable *drawable);

static void cb_texture_check_button( GtkWidget *widget,  gpointer   data );
static void cb_clone_check_button( GtkWidget *widget,  gpointer   data );
static void cb_jpeg_check_button( GtkWidget *widget,  gpointer   data );
static void cb_grain_check_button( GtkWidget *widget,  gpointer   data );

static void cb_texture_hscale( GtkAdjustment *adj,  gpointer   data );
static void cb_radius_hscale( GtkAdjustment *adj,  gpointer   data );
static void cb_compress_hscale( GtkAdjustment *adj,  gpointer   data );

#endif // GUI_H
