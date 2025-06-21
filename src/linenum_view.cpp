#include "linenum_view.h"
#include "file_view.h"
#include "gp_shader.h"

using namespace glm;

LinenumView::LinenumView(FileView &parent) : Widget("L"), parent_(parent) {}

void LinenumView::draw() {
	Scissor s {this};
	GPShader::rect(pos() + ivec2{size().x - 2, 0}, ivec2{2, size().y}, {0x37, 0x37, 0x37, 0xFF}, 254);
	GPShader::rect(pos(), size(), {0x2B, 0x2B, 0x2B, 0xFF}, 254);
	GPShader::draw();

	TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = {0, parent_.scroll_.y};
	TextShader::globals.is_foreground = false;

	TextShader::use(buf_);
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);
}
