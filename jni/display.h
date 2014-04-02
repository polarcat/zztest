/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

struct display;

int display_init(struct display **, void *data);
void display_exit(struct display **);
int display_draw(struct display *);
void display_geom(struct display *display, int *x, int *y, int *w, int *h);

#endif /* __DISPLAY_H__ */
