#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "../src/shader.h"

class GPShader {
	struct __attribute__((packed)) GPVertex {
		glm::ivec2 pos;
		glm::u8vec4 color;
	};

	static inline std::unique_ptr<GPShader> inst_;

	Shader shader_;
	GLuint vao_ {};
	GLuint vbo_ {};
	std::vector<GPVertex> vertices_ {};

	GPShader();
	int setup();
	void create_buffers();

public:
	static int init();
	static void clear();
	static void rect(glm::ivec2 pos, glm::ivec2 size, glm::u8vec4 color);
	static void draw();
	static void set_viewport(glm::ivec2 pos, glm::ivec2 size);
};
