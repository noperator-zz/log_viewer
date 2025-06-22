#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "types.h"
#include "../src/shader.h"

class Widget;

class GPShader {
	struct __attribute__((packed)) GPVertex {
		glm::ivec3 pos;
		color color;
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
	// static void clear();
	static std::tuple<int, int, int, int> rect(const Widget &w, glm::ivec2 pos, glm::ivec2 size, color color, uint8_t z);
	static std::tuple<int, int, int, int> rect(glm::ivec2 pos, glm::ivec2 size, color color, uint8_t z);
	static void draw();
	static void set_viewport(glm::ivec2 size);
};
