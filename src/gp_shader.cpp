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

void GPShader::draw() {
	if (vertices_.empty()) {
		return;
	}

	shader_.use();
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(GPVertex), vertices_.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
}

void GPShader::set_viewport(uvec4 rect) const {
	float ortho[16] = {
		2.0f / rect.z, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / rect.w, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f + (float)rect.x / rect.z, 1.0f - (float)rect.y / rect.w, 1.0f, 1.0f
	};
	shader_.use();
	set_uniform(Matrix4fv, shader_, "u_proj", 1, GL_FALSE, ortho);
}
