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
 * */

#include "screenshooter-actions.h"

void screenshooter_take_and_output_screenshot (ScreenshotData *sd)
{
  GdkPixbuf *screenshot = 
    screenshooter_take_screenshot (sd->mode, sd->delay);
  
  if (sd->action == SAVE)
    {
      screenshooter_save_screenshot (screenshot, 
                                     sd->show_save_dialog, 
                                     sd->screenshot_dir);
    }
  else if (sd->action == CLIPBOARD)
    {
      screenshooter_copy_to_clipboard (screenshot);
    }
  #ifdef HAVE_GIO
  else
    {
      gchar *screenshot_path =
        screenshooter_save_screenshot (screenshot, 
                                       sd->show_save_dialog, 
                                       sd->screenshot_dir);
      if (screenshot_path != NULL)
        {
          screenshooter_open_screenshot (screenshot_path, sd->app);
          g_free (screenshot_path);
        }
    }
  #endif
  
  g_object_unref (screenshot);
}
      