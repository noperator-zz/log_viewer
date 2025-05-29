#include "scrollbar.h"

using namespace glm;

Scrollbar::Thumb::Thumb(std::function<void(ivec2)> scroll_cb)
	: scroll_cb_(std::move(scroll_cb)) {}


void Scrollbar::Thumb::on_drag(ivec2 offset) {
	scroll_cb_(offset);
}

void Scrollbar::Thumb::draw() {
	GPShader::rect(pos(), size(),{100, hovered() * 255, pressed() * 255, 255});
}

Scrollbar::Scrollbar(bool horizontal, std::function<void(double)> scroll_cb)
	: scroll_cb_(std::move(scroll_cb)), thumb_([this](ivec2 o){thumb_cb(o);}), horizontal_(horizontal) {
	add_child(&thumb_);
}

void Scrollbar::thumb_cb(ivec2 offset) {
	if (horizontal_) {
		thumb_.resize({thumb_.pos().x + offset.x, thumb_.pos().y}, thumb_.size());

		auto scroll_range = size().x - thumb_.size().x;
		auto scroll_percent = (double)(thumb_.pos().x - pos().x) / (double)scroll_range;
		scroll_cb_(scroll_percent);
	} else {
		thumb_.resize({thumb_.pos().x, thumb_.pos().y + offset.y}, thumb_.size());

		auto scroll_range = size().y - thumb_.size().y;
		auto scroll_percent = (double)(thumb_.pos().y - pos().y) / (double)scroll_range;
		scroll_cb_(scroll_percent);
	}
}

void Scrollbar::on_resize() {
	resize_thumb();
}

void Scrollbar::set(size_t position, size_t visible_extents, size_t total_extents) {
	position_percent_ = (double)position / (double)total_extents;
	visible_percent_ = (double)visible_extents / (double)total_extents;

	resize_thumb();
}

void Scrollbar::resize_thumb() {
	if (horizontal_) {
		auto thumb_size = std::max<int>(50, (double)size().x * visible_percent_);
		double scroll_range = size().x - thumb_size;
		int thumb_top = scroll_range * position_percent_;
		thumb_.resize({pos().x + thumb_top, pos().y}, {thumb_size, size().y});
	} else {
		auto thumb_size = std::max<int>(50, (double)size().y * visible_percent_);
		double scroll_range = size().y - thumb_size;
		int thumb_top = scroll_range * position_percent_;
		thumb_.resize({pos().x, pos().y + thumb_top}, {size().x, thumb_size});
	}
}

void Scrollbar::draw() {
	if (visible_percent_ >= 1.0f) {
		return; // No need to draw the thumb if it covers the whole scrollbar
	}
	GPShader::rect(pos(), size(), {100, 100, 100, 255});
	thumb_.draw();
}
