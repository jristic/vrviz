#ifndef SHADER_H
#define SHADER_H

#include <string>

// Attempt to load a vertex shader and fragment shader from files, compile
// and link into a shader program. Only call this after OpenGL has started.
// Returns true and sets "program" to the linked program, otherwise
// returns false and gives error messages in "errors".
bool
make_shader_program(const char *vertex_shader_filename,
                    const char *fragment_shader_filename,
                    GLuint& program,
                    std::string& errors);

#endif
