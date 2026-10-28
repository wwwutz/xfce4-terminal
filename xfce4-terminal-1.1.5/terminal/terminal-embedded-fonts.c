/*-
 * Copyright (c) 2026 Peter Marquardt <wwwutz@users.noreply.github.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Register the fonts compiled into xfce4-terminal (see mkembedfont.sh) with
 * fontconfig, so that the default font works without installing them.
 *
 * fontconfig only adds application fonts from files, so each font is copied
 * into an anonymous memory file (memfd) and added by its /proc/self/fd path.
 * fontconfig then sees all of its styles, and Pango opens them through that
 * path.  The descriptors stay open for the life of the process, and are not
 * inherited by the shell.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* memfd_create */
#endif

#include "terminal-embedded-fonts.h"

#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "embedded-fonts.h"



void
terminal_embedded_fonts_register (void)
{
  gsize n;

  for (n = 0; n < G_N_ELEMENTS (embedded_fonts); ++n)
    {
      const unsigned char *data = embedded_fonts[n].data;
      gsize left = embedded_fonts[n].size;
      gchar path[40];
      gint fd;

      fd = memfd_create ("xfce4-terminal-font", MFD_CLOEXEC);
      if (fd < 0)
        {
          g_warning ("Cannot create memory file for embedded font");
          return;
        }

      while (left != 0)
        {
          gssize wrote = write (fd, data, left);
          if (wrote <= 0)
            break;
          data += wrote;
          left -= wrote;
        }

      g_snprintf (path, sizeof (path), "/proc/self/fd/%d", fd);
      if (left != 0 || !FcConfigAppFontAddFile (NULL, (const FcChar8 *) path))
        {
          g_warning ("Cannot add embedded font %s", path);
          close (fd);
        }
    }
}
