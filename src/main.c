/*  $Id$
 *
 *  Copyright © 2008-2009 Jérôme Guelfucci <jerome.guelfucci@gmail.com>
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

#include "libscreenshooter.h"
#include <glib.h>
#include <stdlib.h>



/* Set default values for cli args */
gboolean version = FALSE;
gboolean window = FALSE;
gboolean region = FALSE;
gboolean fullscreen = FALSE;
gboolean no_save_dialog = FALSE;
gboolean hide_mouse = FALSE;
#ifdef HAVE_XMLRPC
#ifdef HAVE_CURL
gboolean upload = FALSE;
#endif
#endif
gchar *screenshot_dir;
gchar *application;
gint delay = 0;



/* Set cli options. */
static GOptionEntry entries[] =
{
  {
    "delay", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &delay,
    N_("Delay in seconds before taking the screenshot"),
    NULL
  },
  {
    "fullscreen", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &fullscreen,
    N_("Take a screenshot of the entire screen"),
    NULL
  },
  {
    "hide", 'h', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &no_save_dialog,
    N_("Do not display the save dialog"),
    NULL
  },
  {
    "mouse", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &hide_mouse,
    N_("Do not display the mouse on the screenshot"),
    NULL
  },
  {
    "open", 'o', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &application,
    N_("Application to open the screenshot"),
    NULL
  },
  {
    "region", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &region,
    N_("Select a region to be captured by clicking a point of the screen "
       "without releasing the mouse button, dragging your mouse to the "
       "other corner of the region, and releasing the mouse button."),
    NULL
  },
  {
    "save", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &screenshot_dir,
    N_("Directory where the screenshot will be saved"),
    NULL
  },
#ifdef HAVE_XMLRPC
#ifdef HAVE_CURL  
  {
    "upload", 'u', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &upload,
    N_("Upload the screenshot to ZimageZ©, a free Web hosting solution"),
    NULL
  },
#endif
#endif
  {
    "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
    N_("Version information"),
    NULL
  },
  {
    "window", 'w', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &window,
    N_("Take a screenshot of the active window"),
    NULL
  },
  {
    NULL, ' ', 0, 0, NULL,
    NULL,
    NULL
  }
};



/* Main */



int main (int argc, char **argv)
{
  GError *cli_error = NULL;
  GFile *default_save_dir;
  const gchar *rc_file;

  ScreenshotData *sd = g_new0 (ScreenshotData, 1);

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* Print a message to advise to use help when a non existing cli option is
  passed to the executable. */
  if (!gtk_init_with_args(&argc, &argv, "", entries, PACKAGE, &cli_error))
    {
      if (cli_error != NULL)
        {
          g_print (_("%s: %s\nTry %s --help to see a full list of"
                     " available command line options.\n"),
                   PACKAGE, cli_error->message, PACKAGE_NAME);

          g_error_free (cli_error);

          return EXIT_FAILURE;
        }
    }

  if (!g_thread_supported ())
    g_thread_init (NULL);

  /* Read the preferences */
  rc_file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, "xfce4/xfce4-screenshooter");
  screenshooter_read_rc_file (rc_file, sd);

  /* Check if the directory read from the preferences is valid */
  default_save_dir = g_file_new_for_uri (sd->screenshot_dir);

  if (G_UNLIKELY (!g_file_query_exists (default_save_dir, NULL)))
    {
      g_free (sd->screenshot_dir);

      sd->screenshot_dir = screenshooter_get_home_uri ();
    }

  g_object_unref (default_save_dir);

  /* Just print the version if we are in version mode */
  if (version)
    {
      g_print ("%s\n", PACKAGE_STRING);

      return EXIT_SUCCESS;
    }

  /* If a region cli option is given, take the screenshot accordingly.*/
  if (fullscreen || window || region)
    {
      sd->cli = TRUE;

      /* Set the region to be captured */
      if (window)
        {
          sd->region = ACTIVE_WINDOW;
        }
      else if (fullscreen)
        {
          sd->region = FULLSCREEN;
        }
      else if (region)
        {
          sd->region = SELECT;
        }

      /* Wether to show the save dialog allowing to choose a filename
       * and a save location */
      no_save_dialog ? (sd->show_save_dialog = 0) : (sd->show_save_dialog = 1);

      /* Whether to display the mouse pointer on the screenshot */
      hide_mouse ? (sd->show_mouse = 0) : (sd->show_mouse = 1);

      sd->delay = delay;

      if (application != NULL)
        {
          sd->app = application;
          sd->action = OPEN;
        }
#ifdef HAVE_XMLRPC
#ifdef HAVE_CURL
      else if (upload)
        {
          sd->app = g_strdup ("none");
          sd->action = UPLOAD;
        }
#endif
#endif
      else
        {
          sd->app = g_strdup ("none");
          sd->action = SAVE;
        }

      /* If the user gave a directory name, verify that it is valid */
      if (screenshot_dir != NULL)
        {
          default_save_dir = g_file_new_for_commandline_arg (screenshot_dir);

          if (G_LIKELY (g_file_query_exists (default_save_dir, NULL)))
            {
              g_free (sd->screenshot_dir);
              sd->screenshot_dir = g_file_get_uri (default_save_dir);
            }
          else
            {
              screenshooter_error ("%s",
                                   _("%s is not a valid directory, the default"
                                     " directory will be used."),
                                   screenshot_dir);
            }

          g_object_unref (default_save_dir);
          g_free (screenshot_dir);
        }
    }
  /* Else we show a dialog which allows to set the screenshot options */
  else
    {
      sd->cli = FALSE;
    }

  g_idle_add ((GSourceFunc) screenshooter_take_and_output_screenshot, sd);

  gtk_main ();

  /* Save preferences */
  if (!sd->cli)
    {
      const gchar *preferences_file =
        xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                     "xfce4/xfce4-screenshooter",
                                     TRUE);

      if (preferences_file != NULL)
        screenshooter_write_rc_file (preferences_file, sd);
    }
#ifdef HAVE_XMLRPC
#ifdef HAVE_CURL
  else if (sd->action == UPLOAD)
    {
      const gchar *preferences_file =
        xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                     "xfce4/xfce4-screenshooter",
                                     TRUE);
      if (preferences_file != NULL)
        {
          XfceRc *rc;

          TRACE ("Open the rc file");
          rc = xfce_rc_simple_open (preferences_file, FALSE);

          if (rc != NULL)
            {
                xfce_rc_write_entry (rc, "last_user", sd->last_user);

                TRACE ("Flush and close the rc file");
                xfce_rc_flush (rc);
                xfce_rc_close (rc);
            }
        }
    }
#endif
#endif

  g_free (sd->screenshot_dir);
  g_free (sd->app);
#ifdef HAVE_XMLRPC
#ifdef HAVE_CURL
  g_free (sd->last_user);
#endif
#endif
  g_free (sd);

  TRACE ("Ciao");

  return EXIT_SUCCESS;
}
