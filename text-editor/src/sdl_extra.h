#ifndef __SDL_EXTRA_H__
#define __SDL_EXTRA_H__

#include <GL/glew.h>
#include "stdbool.h"

const char *shader_type_as_cstr(GLuint shader);
bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader);
bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader);
bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint *program);

#endif // __SDL_EXTRA_H__