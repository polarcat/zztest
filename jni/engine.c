/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <android_native_app_glue.h>

#include "debug.h"
#include "engine.h"
#include "display.h"
#include "program.h"

struct engine {
	struct display *disp;
};

int engine_start(struct engine *engine)
{
	return 0;
}

void engine_stop(struct engine *engine)
{
}

void engine_exit(struct engine **engine)
{
}

int engine_init(struct engine **engine, void *data)
{
	if (!data) {
		errno = EINVAL;
		return -1;
	}

	engine->disp = (struct display *) data;
	return 0;
}
