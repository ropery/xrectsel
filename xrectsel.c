/* xrectsel.c -- print the geometry of a rectangular screen region.

   Copyright (C) 2011-2014  lolilolicon <lolilolicon@gmail.com>

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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#define die(args...) do {error(args); exit(EXIT_FAILURE); } while(0)

typedef struct Region Region;
struct Region {
  Window root;
  int x; /* offset from left of screen */
  int y; /* offset from top of screen */
  int X; /* offset from right of screen */
  int Y; /* offset from bottom of screen */
  unsigned int w; /* width */
  unsigned int h; /* height */
  unsigned int b; /* border_width */
  unsigned int d; /* depth */
};

static void error(const char *errstr, ...);
static int print_region_attr(const char *fmt, Region region);
static int select_region(Display *dpy, Window root, Region *region, int xgrab);

int main(int argc, const char *argv[])
{
  Display *dpy;
  Window root;
  Region sr; /* selected region */
  const char *fmt; /* format string */
  int xgrab=0; 
  int opt=1;

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    die("failed to open display %s\n", getenv("DISPLAY"));
  }

  root = DefaultRootWindow(dpy);


  if (argc > opt && !strcmp(argv[opt], "--xgrab")) {
	  xgrab = 1;
	  opt++;
  }

  fmt = argc > opt ? argv[opt] : "%wx%h+%x+%y\n";

  /* interactively select a rectangular region */
  if (select_region(dpy, root, &sr, xgrab) != EXIT_SUCCESS) {
    XCloseDisplay(dpy);
    die("failed to select a rectangular region\n");
  }

  print_region_attr(fmt, sr);

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}

static void error(const char *errstr, ...)
{
  va_list ap;

  fprintf(stderr, "xrectsel: ");
  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);
  va_end(ap);
}

static int print_region_attr(const char *fmt, Region region)
{
  const char *s;

  for (s = fmt; *s; ++s) {
    if (*s == '%') {
      switch (*++s) {
        case '%':
          putchar('%');
          break;
        case 'x':
          printf("%i", region.x);
          break;
        case 'y':
          printf("%i", region.y);
          break;
        case 'X':
          printf("%i", region.X);
          break;
        case 'Y':
          printf("%i", region.Y);
          break;
        case 'w':
          printf("%u", region.w);
          break;
        case 'h':
          printf("%u", region.h);
          break;
        case 'b':
          printf("%u", region.b);
          break;
        case 'd':
          printf("%u", region.d);
          break;
      }
    } else {
      putchar(*s);
    }
  }

  return 0;
}

static int select_region(Display *dpy, Window root, Region *region, int xgrab)
{
  XEvent ev;

  GC sel_gc;
  XGCValues sel_gv;

  int status, done = 0, btn_pressed = 0;
  int x = 0, y = 0;
  unsigned int width = 0, height = 0;
  int start_x = 0, start_y = 0;

  Cursor cursor;
  cursor = XCreateFontCursor(dpy, XC_tcross);

  /* Grab pointer for these events */
  status = XGrabPointer(dpy, root, True,
               PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);

  if (status != GrabSuccess) {
      error("failed to grab pointer\n");
      return EXIT_FAILURE;
  }

  sel_gv.function = GXinvert;
  sel_gv.subwindow_mode = IncludeInferiors;
  sel_gv.line_width = 1;
  sel_gc = XCreateGC(dpy, root, GCFunction | GCSubwindowMode | GCLineWidth, &sel_gv);

  for (;;) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
      case ButtonPress:
        btn_pressed = 1;
        x = start_x = ev.xbutton.x_root;
        y = start_y = ev.xbutton.y_root;
        width = height = 0;
		if (xgrab){
			XGrabServer(dpy);
		}
        break;
      case MotionNotify:
        /* Draw only if button is pressed */
        if (btn_pressed) {
          /* Re-draw last Rectangle to clear it */
          XDrawRectangle(dpy, root, sel_gc, x, y, width, height);

          x = ev.xbutton.x_root;
          y = ev.xbutton.y_root;

          if (x > start_x) {
            width = x - start_x;
            x = start_x;
          } else {
            width = start_x - x;
          }
          if (y > start_y) {
            height = y - start_y;
            y = start_y;
          } else {
            height = start_y - y;
          }

          /* Draw Rectangle */
          XDrawRectangle(dpy, root, sel_gc, x, y, width, height);
          XFlush(dpy);
        }
        break;
      case ButtonRelease:
        done = 1;
        break;
      default:
        break;
    }
    if (done)
      break;
  }

  /* Re-draw last Rectangle to clear it */
  XDrawRectangle(dpy, root, sel_gc, x, y, width, height);
  XFlush(dpy);

  XUngrabServer(dpy);
  XUngrabPointer(dpy, CurrentTime);
  XFreeCursor(dpy, cursor);
  XFreeGC(dpy, sel_gc);
  XSync(dpy, 1);

  Region rr; /* root region */
  Region sr; /* selected region */

  if (False == XGetGeometry(dpy, root, &rr.root, &rr.x, &rr.y, &rr.w, &rr.h, &rr.b, &rr.d)) {
    error("failed to get root window geometry\n");
    return EXIT_FAILURE;
  }
  sr.x = x;
  sr.y = y;
  sr.w = width;
  sr.h = height;
  /* calculate right and bottom offset */
  sr.X = rr.w - sr.x - sr.w;
  sr.Y = rr.h - sr.y - sr.h;
  /* those doesn't really make sense but should be set */
  sr.b = rr.b;
  sr.d = rr.d;
  *region = sr;
  return EXIT_SUCCESS;
}
