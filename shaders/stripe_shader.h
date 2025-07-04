#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "types.h"
#include "../src/shader.h"

class StripeShader {
public:
	struct __attribute__((packed)) LineStyle {
		float y {};
		color fg {};

		bool operator==(const LineStyle & style) const = default;
	};

	struct UniformGlobals {
		glm::mat4 u_proj;
		glm::ivec2 pos_px;
		glm::ivec2 size_px;
		alignas(4) uint8_t width_px;
		alignas(4) uint8_t z_order;

		void set_viewport(glm::ivec2 size);
	};

	struct Buffer {
		GLuint vao {};
		GLuint vbo_style {};
		size_t size {};
	};

private:
	static inline std::unique_ptr<StripeShader> inst_;
	Shader shader_;
	GLuint globals_idx_ {};
	GLuint ubo_globals_ {};

	StripeShader();

	int setup();
	static void update_uniforms();
	static void use(const Buffer &buf);

public:
	~StripeShader();

	static inline UniformGlobals globals {};
	static int init();
	static void create_buffers(Buffer &buf, size_t size);
	static void destroy_buffers(Buffer &buf);
	static void update(const Buffer &buf, const LineStyle *data);
	static void draw(const Buffer &buf, glm::ivec2 pos, glm::ivec2 size, uint8_t width, uint8_t z);
};
