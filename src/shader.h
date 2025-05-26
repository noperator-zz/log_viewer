#pragma once
#include <iostream>
#include <GL/glew.h>

class Shader {
	const char* vs_src_;
	const char* fs_src_;
	unsigned int id_ {};

	static GLuint compile_shader(GLenum type, const char* src) {
		GLint success;
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char log[512];
			glGetShaderInfoLog(shader, 512, nullptr, log);
			std::cerr << "Shader error: " << log << "\n";
			glDeleteShader(shader);
			shader = 0; // Return 0 if compilation failed
		}
		return shader;
	}

	static GLuint create_program(const char* vs, const char* fs) {
		GLint success;
		GLuint v = compile_shader(GL_VERTEX_SHADER, vs);
		if (v == 0) {
			return 0; // Return 0 if vertex shader compilation failed
		}
		GLuint f = compile_shader(GL_FRAGMENT_SHADER, fs);
		if (f == 0) {
			glDeleteShader(v);
			return 0; // Return 0 if fragment shader compilation failed
		}
		GLuint program = glCreateProgram();
		glAttachShader(program, v);
		glAttachShader(program, f);
		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			char log[512];
			glGetProgramInfoLog(program, 512, nullptr, log);
			std::cerr << "Program link error: " << log << "\n";
			glDeleteProgram(program);
			program = 0; // Return 0 if linking failed
		}
		glDeleteShader(v);
		glDeleteShader(f);
		return program;
	}

public:
	Shader(const char* vs_src, const char* fs_src)
		: vs_src_(vs_src), fs_src_(fs_src) {
	}

	unsigned int id() const {
		return id_;
	}

	int compile() {
		id_ = create_program(vs_src_, fs_src_);
		if (id_ == 0) {
			return -1;
		}
		return 0;
	}

	void use() const {
		glUseProgram(id_);
	}

#define set_uniform(type, shader, name, ...) glUniform##type(glGetUniformLocation(shader.id(), name), __VA_ARGS__);

	// template<typename T>
	// void set_uniform(const char *name, T value) const {
	// 	return set_uniform(name, &value, 1);
	// }
	//
	// template<typename T>
	// void set_uniform(const char *name, const T *values, int count) const {
	// 	GLint location = glGetUniformLocation(id, name);
	// 	if (location == -1) {
	// 		std::cerr << "Uniform " << name << " not found in shader program\n";
	// 		return;
	// 	}
	// 	if constexpr (std::is_same_v<T, float>) {
	// 		glUniform1fv(location, count, values);
	// 	} else if constexpr (std::is_same_v<T, int>) {
	// 		glUniform1iv(location, count, values);
	// 	} else if constexpr (std::is_same_v<T, unsigned int>) {
	// 		glUniform1uiv(location, count, values);
	// 	} else {
	// 		std::cerr << "Unsupported uniform type\n";
	// 	}
	// }
};

