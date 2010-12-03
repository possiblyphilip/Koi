#include "Koi.h"

#ifndef GUI_H
#define GUI_H

typedef struct
{
    gboolean preview;
    gboolean texture_checked;
	int texture_threshold;
	int jpeg_threshold;
		float compress;
		int radius;
	int clone_block_size;
	int histogram_block_size;
	int threads;
    gboolean clone_checked;
     gboolean grain_checked;
	 gboolean speckle_checked;
     gboolean jpeg_checked;
	  gboolean histogram_checked;
} GUI_values;

static gboolean koi_dialog (GimpDrawable *drawable);

static void cb_texture_check_button( GtkWidget *widget,  gpointer   data );
static void cb_clone_check_button( GtkWidget *widget,  gpointer   data );
static void cb_jpeg_check_button( GtkWidget *widget,  gpointer   data );
static void cb_grain_check_button( GtkWidget *widget,  gpointer   data );
static void cb_histogram_check_button( GtkWidget *widget,  gpointer   data );
static void cb_speckle_check_button( GtkWidget *widget,  gpointer   data );
static void cb_jpeg_threshold_hscale( GtkAdjustment *adj,  gpointer   data );

static void cb_texture_hscale( GtkAdjustment *adj,  gpointer   data );
static void cb_radius_hscale( GtkAdjustment *adj,  gpointer   data );
static void cb_compress_hscale( GtkAdjustment *adj,  gpointer   data );

#endif // GUI_H
