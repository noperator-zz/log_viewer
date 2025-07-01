#include "stripe_view.h"

#include <algorithm>
#include <cstring>

StripeView::StripeView(size_t resolution, size_t point_size)
	: Widget("StripeView"), resolution_(resolution), point_size_(point_size), points_(new StripeShader::LineStyle[resolution]) {
	assert(resolution > 0);
	StripeShader::create_buffers(buf_);
	reset();
}

void StripeView::reset() {
	// for (size_t i = 0; i < resolution_; ++i) {
	// 	auto pos = static_cast<float>(i) / resolution_;
	// 	points_[i] = StripeShader::LineStyle{pos, {0xFF, 0, 0, 0xFF * pos}};
	// }
	std::memset(points_.get(), 0, resolution_ * sizeof(points_[0]));
	dirty_ = true;
}

void StripeView::add_point(float location, color color) {
	ssize_t index = std::round(location * resolution_);
	index = std::clamp(index, 0LL, static_cast<ssize_t>(resolution_) - 1);

	auto style = StripeShader::LineStyle(location, color);

	if (points_[index] == style) {
		return; // no change
	}
	points_[index] = style;
	dirty_ = true;
}

void StripeView::on_resize() {
	dirty_ = true;
}

void StripeView::render() {
	if (!dirty_) {
		return; // no changes to draw
	}
	dirty_ = false;

	StripeShader::update(buf_, points_.get(), resolution_);

	// glBindTexture(GL_TEXTURE_2D, tex_);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size().x, size().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//
	// GLuint zero = 0.0f;
	// glClearTexImage(tex_, 0, GL_RGBA, GL_UNSIGNED_BYTE, &zero);

	// glTexSubImage2D(GL_TEXTURE_2D, 0,
	// 	x + g->bitmap_left,
	// 	style_idx * size.y + (size.y - g->bitmap_top - max_descent),
	// 	g->bitmap.width,
	// 	g->bitmap.rows,
	// 	GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
}

void StripeView::draw() {
	render();
	StripeShader::draw(buf_, resolution_, pos(), size(), point_size_, Z_UI_FG);
}
