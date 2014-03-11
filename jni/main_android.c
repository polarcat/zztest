/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>

#include <jni.h>
#include <errno.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "lux.h"
#include "debug.h"
#include "engine.h"
#include "sensor.h"
#include "display.h"
#include "program.h"

struct android_state {
	int x;
	int y;
};

struct android_ctx {
	int animating;
	struct android_app *app;
	struct android_state *state;
	struct engine *engine;
	struct sensor *sensor;
	struct display *display;
	void *argv[2];
};

/* events: read all pending events
 *
 * return -1 on exit, 0 no exit
 * */
static int android_events(struct android_ctx *ctx)
{
	int id;
	int evs;
	struct android_poll_source *src;

	/* If not animating, we will block forever waiting for events.
	 * If animating, we loop until all events are read, then
	 * continue to draw the next frame of animation.
         */
	while (1) {
		if (ctx->animating)
			id = ALooper_pollAll(0, NULL, &evs, (void **) &src);
		else
			id = ALooper_pollAll(-1, NULL, &evs, (void **) &src);

		if (id < 0)
			return 0;

		/* process event */
		if (src != NULL)
			src->process(ctx->app, src);

		/* if a sensor has data, process it now */
		if (id == LOOPER_ID_USER)
			sensor_event(ctx->sensor);

		/* check if we are exiting */
		if (ctx->app->destroyRequested != 0) {
			ctx->animating = 0;
			return -1;
		}
	}

	return 0;
}

static void android_exit(struct android_ctx *ctx)
{
	ctx->animating = 0;

	if (ctx->state)
		free(ctx->state);

	sensor_exit(&ctx->sensor);
	engine_exit(&ctx->engine);
	display_exit(&ctx->display);
}

static void android_win(struct android_app *app)
{
	struct android_ctx *ctx = (struct android_ctx *) app->userData;

	if (!app->window)
		return;

	if (!ctx->display) {
		if (display_init(&ctx->display, app) < 0) {
			_err("display_init() failed\n");
			goto err;
		}
	}

	if (!ctx->engine) {
		ctx->argv[0] = ctx->display;
		ctx->argv[1] = "/data/local/tmp/test.pkm";
		if (engine_init(&ctx->engine, 2, ctx->argv) < 0) {
			_err("engine_init() failed\n");
			goto err;
		}
	}

	engine_start(ctx->engine);
	display_draw(ctx->display);
	engine_stop(ctx->engine);
	return;

err:
	ctx->animating = 0;
	return;
}

static void android_cmd(struct android_app *app, int cmd)
{
	struct android_ctx *ctx = (struct android_ctx *) app->userData;

	switch (cmd) {
	case APP_CMD_SAVE_STATE: /* save our current state */
		_msg("APP_CMD_SAVE_STATE\n");
		app->savedState = ctx->state;
		app->savedStateSize = sizeof(struct android_state);
		break;
	case APP_CMD_INIT_WINDOW: /* window is being shown */
		_msg("APP_CMD_INIT_WINDOW\n");
		android_win(app);
		break;
	case APP_CMD_TERM_WINDOW: /* window is being hidden or closed */
		_msg("APP_CMD_TERM_WINDOW\n");
		/* android_main() will handle cleanups */
		break;
	case APP_CMD_GAINED_FOCUS: /* start monitoring accelerometer */
		_msg("APP_CMD_GAINED_FOCUS\n");
		ctx->animating = 1;
		sensor_enable(ctx->sensor);
		sensor_rate(ctx->sensor, 60); /* 60 events per second */
		break;
	case APP_CMD_LOST_FOCUS: /* stop periodic activities */
		_msg("APP_CMD_LOST_FOCUS\n");
		ctx->animating = 0;
		sensor_disable(ctx->sensor);
		break;
	case APP_CMD_INPUT_CHANGED:
		_msg("APP_CMD_INPUT_CHANGED\n");
		break;
	case APP_CMD_WINDOW_RESIZED:
		_msg("APP_CMD_WINDOW_RESIZED\n");
		break;
	case APP_CMD_WINDOW_REDRAW_NEEDED:
		_msg("APP_CMD_WINDOW_REDRAW_NEEDED\n");
		ctx->animating = 1;
		break;
	case APP_CMD_CONTENT_RECT_CHANGED:
		_msg("APP_CMD_CONTENT_RECT_CHANGED\n");
		break;
	case APP_CMD_CONFIG_CHANGED:
		_msg("APP_CMD_CONFIG_CHANGED\n");
		ctx->animating = 1;
		break;
	case APP_CMD_LOW_MEMORY:
		_msg("APP_CMD_LOW_MEMORY\n");
		break;
	case APP_CMD_START:
		_msg("APP_CMD_START\n");
		break;
	case APP_CMD_RESUME:
		_msg("APP_CMD_RESUME\n");
		break;
	case APP_CMD_PAUSE:
		_msg("APP_CMD_PAUSE\n");
		break;
	case APP_CMD_STOP:
		_msg("APP_CMD_STOP\n");
		break;
	case APP_CMD_DESTROY:
		_msg("APP_CMD_DESTROY\n");
		break;
	default:
		_msg("cmd=%x, %d\n", cmd, cmd);
	}
}

static int android_input(struct android_app *app, AInputEvent *event)
{
	struct android_ctx *ctx = (struct android_ctx *) app->userData;

	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		_log("AINPUT_EVENT_TYPE_MOTION\n");
		if (ctx->animating)
			ctx->animating = 0;
		else
			ctx->animating = 1;
		if (ctx->state) {
			ctx->state->x = AMotionEvent_getX(event, 0);
			ctx->state->y = AMotionEvent_getY(event, 0);
		}
		return 1;
	}

	return 0;
}

void android_main(struct android_app *app)
{
	int sensor_type = ASENSOR_TYPE_ACCELEROMETER;
	struct android_ctx ctx;

	_log("enter\n");

	memset(&ctx, 0, sizeof(ctx));

	ctx.state = calloc(1, sizeof(struct android_state));
	if (!ctx.state) {
		_err("memory allocation failed\n");
		goto out;
	}
	ctx.app = app;

#if ANDROID_422_CRASH_FIXME
	if (sensor_init(&ctx.sensor, sensor_type, (void *) app) < 0) {
		_err("sensor_init() failed\n");
		goto out;
	}
#endif

	/* make sure glue isn't stripped */
	app_dummy();

	app->userData = (void *) &ctx;
	app->onAppCmd = android_cmd;
	app->onInputEvent = android_input;

	_log("enter main loop\n");
	/* main loop */
	while (1) {
		if (android_events(&ctx) < 0)
			break; /* exit requested */

		if (ctx.animating) {
			if (engine_start(ctx.engine) < 0)
				break;
			if (display_draw(ctx.display) < 0)
				break;
			engine_stop(ctx.engine);
		}
	}

out:
	android_exit(&ctx);
	_log("exit\n");
}
