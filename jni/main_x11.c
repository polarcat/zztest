/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define DISPLAY_WIDTH 1280
#define DISPLAY_HEIGHT 720

#include "debug.h"
#include "display.h"

void display_geom(struct display *display, int *x, int *y, int *w, int *h)
{
	if (x)
		*x = 0;
	if (y)
		*y = 0;
	if (w)
		*w = DISPLAY_WIDTH;
	if (h)
		*h = DISPLAY_HEIGHT;
}

static void event_loop(Display *dpy)
{
	XEvent evt;

	while (1) {
		XNextEvent(dpy, &evt);

		if (evt.type == KeyPress) {
			if (XLookupKeysym(&evt.xkey, 0) == XK_Escape)
				return;
		} else if (evt.type == DestroyNotify) {
			return;
		} else if (evt.type == ClientMessage) {
			return;
		}
	}
}

int main(int argc, char **argv)
{
	struct engine *engine = NULL;
	Display *dpy;
	int num;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	Window win;
	XVisualInfo *info = NULL;
	GLXContext ctx  = NULL;
	int conf[] = { GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER, GL_FALSE,
		GLX_DEPTH_SIZE, 1,
		None,
	};

	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		_err("failed to open X display\n");
		return 1;
	}

	num = DefaultScreen(dpy);
	_inf("use GLX_SGIX_pbuffer on screen %d\n", num);

	root = RootWindow(dpy, num);
	info = glXChooseVisual(dpy, num, conf);
	if (!info) {
		_err("glXChooseVisual() failed\n");
		goto out;
	}

	/* window attributes */
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, info->visual, AllocNone);
	attr.event_mask = ButtonPressMask | ExposureMask | KeyPressMask;
	mask = CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
		0, info->depth, InputOutput, info->visual, mask, &attr);

	ctx = glXCreateContext(dpy, info, NULL, GL_TRUE);
	if (!ctx) {
		_err("glXCreateContext() failed\n");
		goto out;
	}

	XFree(info);
	info = NULL;
	XMapWindow(dpy, win);

	_msg("call glXMakeCurrent()\n");
	glXMakeCurrent(dpy, win, ctx);

	_inf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
	_inf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
	_inf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
	_inf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
	_inf("GL_SHADING_LANGUAGE_VERSION = %s\n",
		(char *) glGetString(GL_SHADING_LANGUAGE_VERSION));

	_msg("clear window\n");
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glXSwapBuffers(dpy, win);

	_msg("init engine\n");
	if (engine_init(&engine, argc, argv) < 0) {
		_err("engine_init() failed\n");
		goto out;
	}

	_msg("start engine\n");
	engine_start(engine);

	glXSwapBuffers(dpy, win);

	engine_stop(engine);
	event_loop(dpy);

	_msg("exit engine\n");
	engine_exit(&engine);

out:
	glXMakeCurrent(dpy, 0, 0);

	if (info)
		XFree(info);

	if (ctx)
		glXDestroyContext(dpy, ctx);

	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	return 0;
}
