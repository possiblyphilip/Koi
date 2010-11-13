#include "Koi.h"

#ifndef GUI_H
#define GUI_H

static gboolean koi_dialog (GimpDrawable *drawable);

static void cb_texture_check_button( GtkWidget *widget,  gpointer   data );
static void cb_clone_check_button( GtkWidget *widget,  gpointer   data );
static void cb_texture_hscale( GtkAdjustment *adj,  gpointer   data );

#endif // GUI_H
