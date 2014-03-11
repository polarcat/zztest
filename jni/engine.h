/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef __ENGINE_H__
#define __ENGINE_H__

struct engine;

int engine_init(struct engine **, int argc, void **argv);
void engine_exit(struct engine **);
int engine_start(struct engine *);
void engine_stop(struct engine *);

#endif /* __ENGINE_H__ */
