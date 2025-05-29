#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gp_shader.h"

#include "gp_vert.glsl.h"
#include "gp_frag.glsl.h"

using namespace glm;

GPShader::GPShader() : shader_(gp_vert_glsl, gp_frag_glsl) {
}

int GPShader::setup() {
	int ret = shader_.compile();
	if (ret != 0) {
		return ret;
	}
	shader_.use();
	create_buffers();
	return 0;
}

void GPShader::create_buffers() {
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glBindVertexArray(vao_);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, 10 *  sizeof(GPVertex), nullptr, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, sizeof(GPVertex), (void*)offsetof(GPVertex, pos));
	glEnableVertexAttribArray(0);
	// glVertexAttribDivisor(0, 1);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GPVertex), (void*)offsetof(GPVertex, color));
	glEnableVertexAttribArray(1);
	// glVertexAttribDivisor(1, 1);
}

int GPShader::init() {
	inst_ = std::unique_ptr<GPShader>(new GPShader());
	int ret = inst_->setup();
	if (ret != 0) {
		inst_.reset();
	}
	return ret;
}

void GPShader::clear() {
	inst_->vertices_.clear();
}

void GPShader::rect(ivec2 pos, ivec2 size, u8vec4 color) {
	// Add a rectangle to the vertex buffer
	inst_->vertices_.push_back({pos, color});
	inst_->vertices_.push_back({{pos.x + size.x, pos.y}, color});
	inst_->vertices_.push_back({pos + size, color});
	inst_->vertices_.push_back({pos, color});
	inst_->vertices_.push_back({pos + size, color});
	inst_->vertices_.push_back({{pos.x, pos.y + size.y}, color});
}

void GPShader::draw() {
	if (inst_->vertices_.empty()) {
		return;
	}

	inst_->shader_.use();
	glBindVertexArray(inst_->vao_);
	glBindBuffer(GL_ARRAY_BUFFER, inst_->vbo_);
	glBufferData(GL_ARRAY_BUFFER, inst_->vertices_.size() * sizeof(GPVertex), inst_->vertices_.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, inst_->vertices_.size());
}

void GPShader::set_viewport(ivec2 pos, ivec2 size) {
	auto mat = ortho<float>(
		pos.x, pos.x + size.x,
		pos.y + size.y, pos.y,
		-1.0f, 1.0f
	);
	inst_->shader_.use();
	set_uniform(Matrix4fv, inst_->shader_, "u_proj", 1, GL_FALSE, value_ptr(mat));
}
