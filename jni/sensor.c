/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <android_native_app_glue.h>
#include <android/sensor.h>

#include "debug.h"
#include "sensor.h"

struct sensor {
	int enabled;
	const ASensor *sensor;
	ASensorManager *mgr;
	ASensorEventQueue *queue;
};

int sensor_enable(struct sensor *sensor)
{
	int rc;

	if (!sensor) {
		errno = EFAULT;
		return -1;
	}

	if (sensor->enabled)
		return 0;

	rc = ASensorEventQueue_enableSensor(sensor->queue, sensor->sensor);
	if (rc < 0)
		sensor->enabled = 0;
	else
		sensor->enabled = 1;

	return rc;
}

void sensor_disable(struct sensor *sensor)
{
	if (!sensor)
		return;

	sensor->enabled = 0;
	ASensorEventQueue_disableSensor(sensor->queue, sensor->sensor);
}

void sensor_rate(struct sensor *sensor, int rate)
{
	if (!sensor)
		return;

	ASensorEventQueue_setEventRate(sensor->queue, sensor->sensor,
		(1000L / rate) * 1000);
}

void sensor_event(struct sensor *sensor)
{
	ASensorEvent ev;

	if (!sensor)
		return;

	while (ASensorEventQueue_getEvents(sensor->queue, &ev, 1) > 0) {
#if 0
		_log("accel: x=%f y=%f z=%f", ev.acceleration.x,
			ev.acceleration.y, ev.acceleration.z);
#endif
	}
}

void sensor_exit(struct sensor **sensor)
{
	struct sensor *ptr = *sensor;

	if (!ptr)
		return;

	ASensorManager_destroyEventQueue(ptr->mgr, ptr->queue);
	free(ptr);
	*sensor = NULL;
}

#define getDefaultSensor(a, b) ASensorManager_getDefaultSensor(a, b)
#define createEventQueue(a, b) \
	ASensorManager_createEventQueue(a, b, LOOPER_ID_USER, NULL, NULL)

int sensor_init(struct sensor **sensor, int type, void *data)
{
	struct android_app *app = (struct android_app *) data;
	struct sensor *ptr;

	if (!app) {
		errno = EFAULT;
		return -1;
	}

	if (!(ptr = calloc(1, sizeof(*ptr))))
		return -1;

	ptr->mgr = ASensorManager_getInstance();
	if (!ptr->mgr)
		goto err;

	ptr->sensor = getDefaultSensor(ptr->mgr, type);
	if (!ptr->sensor)
		goto err;

	ptr->queue = createEventQueue(ptr->mgr, app->looper);
	if (!ptr->queue)
		goto err;

	*sensor = ptr;
	return 0;
err:
	sensor_exit(&ptr);
	return -1;
}
