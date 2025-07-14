#include "label_view.h"
#include "gp_shader.h"
#include "types.h"

using namespace glm;

LabelView::LabelView(Widget *parent) : Widget(parent) {
	TextShader::create_buffers(buf_, BUFFER_SIZE);
}

void LabelView::set_text(const std::string &text) {
	text_ = text;
	soil();
}

void LabelView::on_resize() {
}

void LabelView::update() {
	std::vector<ivec2> coords {};
	TextShader::use(buf_);
	TextShader::render(buf_, text_, {{}, {}, {0xFF, 0xFF, 0xFF, 0xFF}, {}}, {}, coords);
}

void LabelView::draw() {
	TextShader::use(buf_);
	TextShader::draw(pos(), {}, 0, text_.size(), Z_UI_FG);
}

