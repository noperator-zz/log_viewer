#include "linenum_view.h"
#include "file_view.h"
#include "gp_shader.h"

using namespace glm;

LinenumView::LinenumView(FileView &parent) : Widget("L"), parent_(parent) {}

void LinenumView::draw() {
	Scissor s {this};
	GPShader::rect(pos() + ivec2{size().x - 2, 0}, ivec2{2, size().y}, {0x37, 0x37, 0x37, 0xFF}, Z_FILEVIEW_BG);
	GPShader::rect(pos(), size(), {0x2B, 0x2B, 0x2B, 0xFF}, Z_FILEVIEW_BG);
	GPShader::draw();

	TextShader::use(buf_);
	TextShader::draw(pos(), {0, parent_.scroll_.y}, render_range_.x, render_range_.y - render_range_.x, Z_FILEVIEW_TEXT_FG, Z_FILEVIEW_TEXT_BG);
}
