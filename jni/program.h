/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

struct shader {
	int type;
	const char *name;
	unsigned int id;
	const char *code;
};

struct program;

int program_init(struct program **); /* returns program id */
void program_exit(struct program **);
int program_make(struct program *, struct shader **, int num);
void program_clean(struct program *);

#endif /* __PROGRAM_H__ */
