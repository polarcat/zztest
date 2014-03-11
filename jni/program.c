/*
 * By Aliaksei Katovich <aliaksei.katovich at gmail.com>
 * Any copyright is dedicated to the Public Domain.
 *
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <stdlib.h>

#ifdef HAVE_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include "debug.h"
#include "program.h"

struct program {
	GLuint id;
	struct shader **shader;
	int shader_num;
};

static int make_shader(struct shader *shader)
{
	GLint res = GL_FALSE;
	GLsizei len = 0;
	GLchar log[4096] = "nil";/* FIXME: some drivers return wrong values for
				 * GL_INFO_LOG_LENGTH use a fixed size instead
				 */
	timeinit();

	if (!shader) {
		errno = EFAULT;
		return -1;
	}

	timestart();

	shader->id = glCreateShader(shader->type);
	if (shader->id == 0) {
		glerr("glCreateShader(%s) failed", shader->name);
		goto error;
	}

	glShaderSource(shader->id, 1, &shader->code, NULL);
	glCompileShader(shader->id);
	glGetShaderiv(shader->id, GL_COMPILE_STATUS, &res);
	if (res != GL_TRUE) {
		glerr("glCompileShader(%s) failed", shader->name);
		goto error;
	}

	timestop("compiled shader %s, id %d type 0x%04x\n", shader->name,
		 shader->id, shader->type);
	return 0;

error:
	if (shader->id > 0) {
		glGetShaderInfoLog(shader->id, sizeof(log), &len, log);
		_msg("len: %d, log: %s\n", len, log);
		glDeleteShader(shader->id);
		shader->id = 0;
	}
	return -1;
}

void program_clean(struct program *prog)
{
	int i;

	if (!prog) {
		errno = EFAULT;
		return;
	}

	for (i = 0; i < prog->shader_num; i++)
		glDeleteShader(prog->shader[i]->id);

	glDeleteProgram(prog->id);
}

int program_make(struct program *prog, struct shader **shader, int num)
{
	int i;
	GLchar log[256] = "nil";
	GLsizei len = 0;
	GLint res = GL_FALSE;

	if (!prog) {
		errno = EFAULT;
		return -1;
	}

	timeinit();

	prog->id = glCreateProgram();
	if (prog->id == 0) {
		glerr("glCreateProgram() failed");
		return -1;
	}

	prog->shader = shader;
	prog->shader_num = num;

	for (i = 0; i < num; i++) {
		if (make_shader(shader[i]) < 0) {
			_err("shader %s failed\n", shader[i]->name);
			goto err;
		}
		glAttachShader(prog->id, shader[i]->id);
	}

	timestart();
	glLinkProgram(prog->id);
	glGetProgramiv(prog->id, GL_LINK_STATUS, &res);
	if (res != GL_TRUE) {
		_err("failed to link program\n");
		goto err;
	}

	timestop("program %d linked\n", prog->id);
	return prog->id;

err:
	if (prog->id > 0) {
		glGetProgramInfoLog(prog->id, sizeof (log), &len, log);
		_msg("len: %d, log: %s\n", len, log);
	}

	program_clean(prog);
	return -1;
}

void program_exit(struct program **prog)
{
	struct program *ptr = *prog;

	if (!ptr)
		return;

	program_clean(ptr);
	free(ptr);
	*prog = NULL;
}

int program_init(struct program **prog)
{
	struct program *ptr = calloc(1, sizeof(*ptr));

	if (!ptr)
		return -1;

	*prog = ptr;
	return 0;
}
