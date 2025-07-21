#include "stripe_view.h"

#include <algorithm>
#include <cstring>


size_t StripeView::Dataset::first_line_for_tick(size_t tick, size_t num_lines) const {
	return tick * num_lines / parent_.num_ticks_;
}

size_t StripeView::Dataset::middle_tick_for_line(size_t line, size_t num_lines) const {
	return ((line + 1) * parent_.num_ticks_ / num_lines) - (parent_.num_ticks_ / (2 * num_lines));

	// Return the _last_ tick associated with this line.
	const size_t next_tick = (line + 1) * parent_.num_ticks_ / num_lines;
	return next_tick == 0 ? 0 : next_tick - 1;
}

void StripeView::Dataset::reset() {
	prev_num_lines_ = 0;
	next_poi_idx_ = 0;
	std::memset(ticks_.get(), 0, parent_.num_ticks_ * sizeof(ticks_[0]));
}

void StripeView::Dataset::update() {
	StripeShader::update(buf_, ticks_.get());
}

void StripeView::Dataset::draw() {
	StripeShader::draw(buf_, parent_.pos(), parent_.size(), parent_.tick_size_, Z_UI_FG);
}

StripeView::Dataset::Dataset(StripeView &parent,	color color)
: parent_(parent), color_(color), ticks_(new StripeShader::LineStyle[parent_.num_ticks_]) {
	StripeShader::create_buffers(buf_, parent_.num_ticks_);
	reset();
}

void StripeView::Dataset::feed(const dynarray<size_t> &line_starts, const dynarray<size_t> &poi_lines) {
	const size_t num_lines = line_starts.size();
	assert(num_lines >= prev_num_lines_);

	// Check if the number of lines has increased enough that any previously computed position would shift
	//  by more than one tick. In this case, we need to recompute all positions.
	const double mult = 1. + 1. / (double)parent_.num_ticks_;
	const size_t next_num_lines = (double)prev_num_lines_ * mult;

	if (num_lines > next_num_lines) {
		// reset state so that we recompute all positions
		reset();
		prev_num_lines_ = num_lines;
	}

	for (size_t poi_idx = next_poi_idx_; poi_idx < poi_lines.size(); ++poi_idx) {
		size_t poi_line = poi_lines[poi_idx];

		const float position = poi_line / (double)num_lines;
		size_t tick = middle_tick_for_line(poi_line, num_lines);
		ticks_[tick] = {position, color_};
	}
	if (next_poi_idx_ != poi_lines.size()) {
		next_poi_idx_ = poi_lines.size();
		parent_.soil();
	}
}


StripeView::StripeView(Widget *parent, size_t resolution, size_t tick_size)
	: Widget(parent, "StripeView"), num_ticks_(resolution), tick_size_(tick_size) {
	assert(resolution > 0);
}

void StripeView::add_dataset(void *key, color color) {
	datasets_.emplace(std::piecewise_construct,
		std::forward_as_tuple(key),
		std::forward_as_tuple(*this, color));
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
}

void StripeView::draw() {
	for (auto &[key, dataset] : datasets_) {
		dataset.draw();
	}
}
