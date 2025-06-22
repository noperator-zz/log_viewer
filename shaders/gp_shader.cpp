#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gp_shader.h"

#include "gp_vert.glsl.h"
#include "gp_frag.glsl.h"
#include "types.h"
#include "widget.h"

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

	glVertexAttribIPointer(0, 3, GL_UNSIGNED_INT, sizeof(GPVertex), (void*)offsetof(GPVertex, pos));
	glEnableVertexAttribArray(0);
	// glVertexAttribDivisor(0, 1);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GPVertex), (void*)offsetof(GPVertex, color));
	glEnableVertexAttribArray(1);
	// glVertexAttribDivisor(1, 1);
}

int GPShader::init() {
	if (inst_) {
		return -1; // Already initialized
	}
	inst_ = std::unique_ptr<GPShader>(new GPShader());
	int ret = inst_->setup();
	if (ret != 0) {
		inst_.reset();
	}
	return ret;
}

// void GPShader::clear() {
// 	inst_->vertices_.clear();
// }

std::tuple<int, int, int, int> GPShader::rect(const Widget &w, ivec2 pos_, ivec2 size_, color color, uint8_t z) {
	auto max_size = w.size() - (pos_ - w.pos());
	if (size_.x <= 0) {
		size_.x = w.size().x + size_.x;
	}
	size_.x = std::min(size_.x, max_size.x);

	if (size_.y <= 0) {
		size_.y = w.size().y + size_.y;
	}
	size_.y = std::min(size_.y, max_size.y);
	return rect(pos_, size_, color, z);
}

std::tuple<int, int, int, int> GPShader::rect(ivec2 pos_, ivec2 size_, color color, uint8_t z) {
	// Add a rectangle to the vertex buffer
	ivec3 pos = {pos_.x, pos_.y, z};
	ivec3 size = {size_.x, size_.y, 0};
	inst_->vertices_.push_back({pos, color});
	inst_->vertices_.push_back({pos + ivec3{size.x, 0, 0}, color});
	inst_->vertices_.push_back({pos + size, color});
	inst_->vertices_.push_back({pos, color});
	inst_->vertices_.push_back({pos + size, color});
	inst_->vertices_.push_back({pos + ivec3{0, size.y, 0}, color});
	return {pos.x, pos.y, size.x, size.y};
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

	inst_->vertices_.clear();
}

void GPShader::set_viewport(ivec2 size) {
	auto mat = ortho<float>(0, size.x, size.y, 0, 0, -1);
	inst_->shader_.use();
	set_uniform(Matrix4fv, inst_->shader_, "u_proj", 1, GL_FALSE, value_ptr(mat));
}
