#include "scrollbar.h"

using namespace glm;

Scrollbar::Thumb::Thumb(Widget &parent, ivec2 pos, ivec2 size, std::function<void(double)> scroll_cb)
	: Widget(&parent, pos, size), scroll_cb_(std::move(scroll_cb)) {}


void Scrollbar::Thumb::on_drag(ivec2 offset) {
	std::cout << "Thumb dragged by " << offset.x << ", " << offset.y << "\n";
	resize({pos().x, pos().y + offset.y}, size());

	auto scroll_range = parent()->size().y - size().y;
	auto scroll_percent = (double)(pos().y - parent()->pos().y) / (double)scroll_range;
	scroll_cb_(scroll_percent);
}

void Scrollbar::Thumb::draw() {
	GPShader::rect(pos(), size(),{100, hovered() * 255, pressed() * 255, 255});
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
	auto thumb_height = std::max<int>(50, (double)size().y * visible_percent_);
	double scroll_range = size().y - thumb_height;
	int thumb_top = scroll_range * position_percent_;

	thumb_.resize({pos().x, pos().y + thumb_top}, {size().x, thumb_height});
}

void Scrollbar::draw() {
	if (visible_percent_ >= 1.0f) {
		return; // No need to draw the thumb if it covers the whole scrollbar
	}
	GPShader::rect(pos(), size(), {100, 100, 100, 255});
	thumb_.draw();
}
