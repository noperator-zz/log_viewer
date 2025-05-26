#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "shader.h"

class GPShader {
public:
	struct __attribute__((packed)) GPVertex {
		glm::uvec2 pos;
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
	void rect(glm::uvec2 tl, glm::uvec2 br, glm::u8vec4 color) {
		// Add a rectangle to the vertex buffer
		vertices_.push_back({tl, color});
		vertices_.push_back({{br.x, tl.y}, color});
		vertices_.push_back({br, color});
		vertices_.push_back({tl, color});
		vertices_.push_back({br, color});
		vertices_.push_back({{tl.x, br.y}, color});
	}
	// std::vector<GPVertex>& vertices() {
		// return vertices_;
	// }
	// void use() const;
	void draw();
	void set_viewport(glm::uvec4 rect) const;
};
