// glew & glfw
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef _MSC_VER
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>

#include "shader.h"

static bool
load_text_file(const char *filename,
			   std::string& text,
			   std::string& errors)
{
	if(!filename){
		errors.append("filename was NULL\n");
		return false;
	}
	std::ifstream in(filename);
	if(!in){
		errors.append("file '"+std::string(filename)+"' couldn't be opened\n");
		return false;
	}
	text.clear();
	// read everything up to a NULL character, which shouldn't appear in a text
	// file, or until the end of the file
	std::getline(in, text, (char)0);
	return true;
}

static bool
compile_shader(GLenum shader_type,
			   const char *filename,
			   GLuint& shader,
			   std::string& errors)
{
	// load in source code for vertex shader
	std::string shader_src;
	if(!load_text_file(filename, shader_src, errors))
		return false;
	// create a shader for it in OpenGL
	shader=glCreateShader(shader_type);
	if(shader==0){
		errors.append("Couldn't create shader object\n");
		return false;
	}
	// attempt to compile source
	const char *src_string=shader_src.c_str();
	glShaderSource(shader, 1, &src_string, 0);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE){ // problem with compilation?
		errors.append("Errors with file '"+std::string(filename)+"'\n");
		GLint infolength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolength);
		std::vector<char> info(infolength, (char)0);
		glGetShaderInfoLog(shader, infolength, &infolength, &info[0]);
		errors.append(info.begin(), info.end());
		return false;
	}
	// we're done!
	return true;
}

bool
make_shader_program(const char *vertex_shader_filename,
					const char *fragment_shader_filename,
					GLuint& program,
					std::string& errors)
{
	// first attempt to load and compile vertex and fragment shaders
	GLuint vshader, fshader;
	if(!compile_shader(GL_VERTEX_SHADER, vertex_shader_filename, vshader,
					   errors))
		return false;
	if(!compile_shader(GL_FRAGMENT_SHADER, fragment_shader_filename, fshader,
					   errors))
		return false; // ideally should clean up vertex shader too...

	// now set up the program
	program=glCreateProgram();
	if(program==0){
		errors.append("Error creating shader program\n");
		return false; // ideally should clean up here...
	}
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	// link the program
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if(status==GL_FALSE){
		errors.append("Error when linking shader program\n");
		return false; // ideally should clean up here...
	}

	// we're done!
	return true;
}
