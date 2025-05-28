#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "shader.h"

class GPShader {
public:
	struct __attribute__((packed)) GPVertex {
		glm::ivec2 pos;
		glm::u8vec4 color;
	};

private:
	Shader shader_;
	GLuint vao_ {};
	GLuint vbo_ {};
	std::vector<GPVertex> vertices_ {};

	void create_buffers();

public:
	GPShader();

	int setup();
	void clear() {
		vertices_.clear();
	}
	void rect(glm::ivec2 pos, glm::ivec2 size, glm::u8vec4 color) {
		// Add a rectangle to the vertex buffer
		vertices_.push_back({pos, color});
		vertices_.push_back({{pos.x + size.x, pos.y}, color});
		vertices_.push_back({pos + size, color});
		vertices_.push_back({pos, color});
		vertices_.push_back({pos + size, color});
		vertices_.push_back({{pos.x, pos.y + size.y}, color});
	}
	// std::vector<GPVertex>& vertices() {
		// return vertices_;
	// }
	// void use() const;
	void draw();
	void set_viewport(glm::uvec4 rect) const;
};
