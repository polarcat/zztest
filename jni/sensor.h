/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

struct sensor;

int sensor_init(struct sensor **, int type, void *data);
void sensor_exit(struct sensor **);
int sensor_enable(struct sensor *);
void sensor_disable(struct sensor *);
void sensor_rate(struct sensor *, int rate);
void sensor_event(struct sensor *);

#endif /* __SENSOR_H__ */
