#include "stripe_view.h"

#include <algorithm>
#include <cstring>

StripeView::StripeView(size_t resolution, size_t tick_size)
	: Widget("StripeView"), num_ticks_(resolution), tick_size_(tick_size) {
	assert(resolution > 0);
}

// void StripeView::reset() {
// 	// for (size_t i = 0; i < resolution_; ++i) {
// 	// 	auto pos = static_cast<float>(i) / resolution_;
// 	// 	ticks_[i] = StripeShader::LineStyle{pos, {0xFF, 0, 0, 0xFF * pos}};
// 	// }
// 	std::memset(ticks_.get(), 0, num_ticks_ * sizeof(ticks_[0]));
// 	soil();
// }
//
// void StripeView::add_tick(float location, color color) {
// 	ssize_t index = std::round(location * num_ticks_);
// 	index = std::clamp(index, 0LL, static_cast<ssize_t>(num_ticks_) - 1);
//
// 	auto style = StripeShader::LineStyle(location, color);
//
// 	if (ticks_[index] == style) {
// 		return; // no change
// 	}
// 	ticks_[index] = style;
// 	soil();
// }

void StripeView::add_dataset(void *key, color color, std::function<size_t(const void*)> &&get_location) {
	// FIXME
	datasets_.try_emplace(key, *this, color, std::move(get_location));
}

void StripeView::remove_dataset(void *key) {
	datasets_.erase(key);
}

void StripeView::on_resize() {
	soil();
}

void StripeView::update() {
	for (auto &[key, dataset] : datasets_) {
		dataset.update();
	}

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
	for (auto &[key, dataset] : datasets_) {
		dataset.draw();
	}
}
