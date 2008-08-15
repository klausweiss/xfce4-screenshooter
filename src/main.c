/*  $Id$
 *
 *  Copyright © 2008 Jérôme Guelfucci <jerome.guelfucci@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "screenshooter-utils.h"

gboolean version = FALSE;
gboolean window = FALSE;
gboolean no_save_dialog = FALSE;
gboolean preferences = FALSE;
gchar * screenshot_dir;
gint delay = 0;

static GOptionEntry entries[] =
{
    {    "version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
    {   "window", 'w', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &window,
        N_("Take a screenshot of the active window"),
        NULL
    },
    {		"delay", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &delay,
       N_("Delay in seconds before taking the screenshot"),
       NULL
    
    },
    {   "hide", 'h', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &no_save_dialog,
        N_("Do not display the save dialog"),
        NULL
    },
    {   "save", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &screenshot_dir,
        N_("Directory where the screenshot will be saved"),
        NULL
    },
    {   "preferences", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &preferences,
        N_("Dialog to set the default save folder"),
        NULL
    },
    { NULL }
};

int main(int argc, char **argv)
{
  GError *cli_error = NULL;
  GdkPixbuf * screenshot;
  ScreenshotData * sd = g_new0 (ScreenshotData, 1);
  XfceRc *rc;
  gchar * rc_file;

  rc_file = g_build_filename( xfce_get_homedir(), ".config", "xfce4", 
                              "xfce4-screenshooter", NULL);

  if ( g_file_test(rc_file, G_FILE_TEST_EXISTS) )
  {
    rc = xfce_rc_simple_open (rc_file, TRUE);
    screenshot_dir = g_strdup ( xfce_rc_read_entry (rc, "screenshot_dir", 
                                xfce_get_homedir () ) );
    sd->screenshot_dir = screenshot_dir;
    xfce_rc_close (rc);
  }
  else
  {
    screenshot_dir = g_strdup( xfce_get_homedir () );
    sd->screenshot_dir = screenshot_dir;
  }

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
  if (!gtk_init_with_args(&argc, &argv, _(""), entries, PACKAGE, &cli_error))
  {
    if (cli_error != NULL)
    {
      g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), 
               PACKAGE, cli_error->message, PACKAGE_NAME);
      g_error_free (cli_error);
      return 1;
    }
  }
  
  if (version)
  {
    g_print("%s\n", PACKAGE_STRING);
    return 0;
  }
  
  if (window)
  {
    sd->whole_screen = 0;    
  }
  else
  {
    sd->whole_screen = 1;
  }
  
  if (no_save_dialog)
  {
    sd->show_save_dialog = 0;
  }
  else
  {
    sd->show_save_dialog = 1;
  }

  sd->screenshot_delay = delay;
  
  if ( g_file_test (screenshot_dir, G_FILE_TEST_IS_DIR) )
  {
    if ( g_path_is_absolute ( screenshot_dir ) )
    { 
      sd->screenshot_dir = screenshot_dir;
    }
    else
    {
      screenshot_dir = g_build_filename(g_get_current_dir(), screenshot_dir, NULL);
      sd->screenshot_dir = screenshot_dir;
    }
  }
  else
  {
    g_warning ("%s is not a valid directory, the default directory will be used.", screenshot_dir);
  }
  
  if ( !preferences )
  {
    screenshot = take_screenshot( sd );
    save_screenshot (screenshot, sd); 
  }
  else
  {
    GtkWidget * chooser;
    gint dialog_response;
    gchar * dir;

    dir = screenshot_dir;

    chooser = gtk_file_chooser_dialog_new ( _("Default save folder"),
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_dialog_set_default_response (GTK_DIALOG ( chooser ), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER ( chooser ), dir);

    dialog_response = gtk_dialog_run( GTK_DIALOG ( chooser ) );

    if ( dialog_response == GTK_RESPONSE_ACCEPT )
	  {
	    dir = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( chooser ) );
      
      rc = xfce_rc_simple_open (rc_file, FALSE);
      xfce_rc_write_entry (rc, "screenshot_dir", dir);
      xfce_rc_close ( rc );
      
      g_free( dir );
	  }
    gtk_widget_destroy( GTK_WIDGET ( chooser ) );
  }
  
  g_free( sd->screenshot_dir );
  g_free ( sd );
  g_free( rc_file );
  return 0;
}
