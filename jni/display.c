/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <android_native_app_glue.h>

#include "debug.h"
#include "display.h"

struct display {
	EGLDisplay dpy;
	EGLContext ctx;
	EGLSurface srf;
	int x;
	int y;
	int width;
	int height;
	struct android_app *app;
	EGLConfig cfg;
};

/* EGL constants */
static const EGLint srf_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE, };
static const EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, };
static const EGLint dpy_attr[] = {
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_DEPTH_SIZE, 0,
	EGL_NONE,
};

static int init_context(struct display *d)
{
	d->ctx = eglCreateContext(d->dpy, d->cfg, EGL_NO_CONTEXT, ctx_attr);
	if (!d->ctx) {
		eglerr("eglCreateContext() failed");
		return -1;
	}

	if (eglMakeCurrent(d->dpy, d->srf, d->srf, d->ctx) == EGL_FALSE) {
		eglerr("eglMakeCurrent() failed");
		return -1;
	}

	return 0;
}

static void deinit_display(struct display *d)
{
	if (d->dpy != EGL_NO_DISPLAY) {
		eglMakeCurrent(d->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
		if (d->ctx)
			eglDestroyContext(d->dpy, d->ctx);
		if (d->srf)
			eglDestroySurface(d->dpy, d->srf);
		eglTerminate(d->dpy);
		d->dpy = EGL_NO_DISPLAY;
		d->ctx = EGL_NO_CONTEXT;
		d->srf = EGL_NO_SURFACE;
	}
}

static int init_display(struct display *d)
{
	EGLint fmt;
	EGLint num;
	int err;

	eglBindAPI(EGL_OPENGL_ES_API);

	d->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (d->dpy == EGL_NO_DISPLAY) {
		eglerr("eglGetDisplay() failed");
		return -1;
	}

	eglInitialize(d->dpy, 0, 0);
	eglChooseConfig(d->dpy, dpy_attr, &d->cfg, 1, &num);
	if (num != 1) {
		eglerr("eglChooseConfig() failed, got %d configs", num);
		return -1;
	}

	d->srf = eglCreateWindowSurface(d->dpy, d->cfg, d->app->window, NULL);
	if (!d->srf) {
		eglerr("eglCreateWindowSurface() failed");
		return -1;
	}

	eglQuerySurface(d->dpy, d->srf, EGL_WIDTH, &d->width);
	eglQuerySurface(d->dpy, d->srf, EGL_HEIGHT, &d->height);

	if (!eglGetConfigAttrib(d->dpy, d->cfg, EGL_NATIVE_VISUAL_ID, &fmt)) {
		eglerr("eglGetConfigAttrib() failed");
		return -1;
	}

	if (ANativeWindow_setBuffersGeometry(d->app->window, 0, 0, fmt) < 0) {
		_err("ANativeWindow_setBuffersGeometry() failed");
		return -1;
	}

	return init_context(d);
}

void display_geom(struct display *disp, int *x, int *y, int *w, int *h)
{
	if (!disp)
		return;
	if (x)
		*x = disp->x;
	if (y)
		*y = disp->y;
	if (w)
		*w = disp->width;
	if (h)
		*h = disp->height;
}

int display_draw(struct display *disp)
{
	GLenum err;

	if (!disp) {
		errno = EFAULT;
		return -1;
	}

	if (eglSwapBuffers(disp->dpy, disp->srf) == EGL_FALSE) {
		err = eglGetError();
		if (err == EGL_BAD_SURFACE || err == EGL_BAD_ALLOC)
			return init_display(disp);
		else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT)
			return init_context(disp);

		eglerr("eglSwapBuffers() failed");
		return -1;
	}

	return 0;
}

void display_exit(struct display **disp)
{
	struct display *ptr = *disp;

	if (!ptr)
		return;

	deinit_display(ptr);
	free(ptr);
	*disp = NULL;
}

int display_init(struct display **disp, void *data)
{
	struct display *ptr;

	if (!data) {
		errno = EFAULT;
		return -1;
	}

	ptr = calloc(1, sizeof(*ptr));
	if (!ptr)
		return -1;

	ptr->app = (struct android_app *) data;

	if (init_display(ptr) < 0) {
		_err("init_display() failed\n");
		display_exit(&ptr);
		return -1;
	}

	*disp = ptr;
	return 0;
}
