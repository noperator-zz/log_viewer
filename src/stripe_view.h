#pragma once
#include <functional>
#include <memory>

#include "dynarray.h"
#include "stripe_shader.h"
#include "widget.h"
#include "types.h"

class StripeView : public Widget {
public:
	class Dataset {
		friend class StripeView;
		StripeView &parent_;
		color color_;
		std::function<size_t(const void*)> get_location_;
		std::unique_ptr<StripeShader::LineStyle[]> ticks_;
		StripeShader::Buffer buf_ {};

		size_t prev_num_lines_ {};
		size_t next_poi_idx_ {};

		Dataset(const Dataset &) = delete;
		Dataset &operator=(const Dataset &) = delete;
		Dataset(Dataset &&) = delete;
		Dataset &operator=(Dataset &&) = delete;

		bool compare(const void *a, const void *b) const {
			return get_location_(a) < get_location_(b);
		}

		size_t first_line_for_tick(size_t tick, size_t num_lines) const {
			return tick * num_lines / parent_.num_ticks_;
		}

		size_t middle_tick_for_line(size_t line, size_t num_lines) const {
			return ((line + 1) * parent_.num_ticks_ / num_lines) - (parent_.num_ticks_ / (2 * num_lines));

			// Return the _last_ tick associated with this line.
			const size_t next_tick = (line + 1) * parent_.num_ticks_ / num_lines;
			return next_tick == 0 ? 0 : next_tick - 1;
		}

		void reset() {
			prev_num_lines_ = 0;
			next_poi_idx_ = 0;
			std::memset(ticks_.get(), 0, parent_.num_ticks_ * sizeof(ticks_[0]));
		}

		void update() {
			StripeShader::update(buf_, ticks_.get());
		}

		void draw() {
			StripeShader::draw(buf_, parent_.pos(), parent_.size(), parent_.tick_size_, Z_UI_FG);
		}

	public:
		Dataset(StripeView &parent,	color color)
		: parent_(parent), color_(color), ticks_(new StripeShader::LineStyle[parent_.num_ticks_]) {
			StripeShader::create_buffers(buf_, parent_.num_ticks_);
			reset();
		}

		void feed(const dynarray<size_t> &line_starts, const dynarray<size_t> &poi_lines) {
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
	};

private:
	StripeView(const StripeView &) = delete;
	StripeView &operator=(const StripeView &) = delete;
	StripeView(StripeView &&) = delete;
	StripeView &operator=(StripeView &&) = delete;

	const size_t num_ticks_;
	size_t tick_size_;
	std::unordered_map<void*, Dataset> datasets_ {};

	void on_resize() override;
	void update() override;

public:
	StripeView(Widget *parent, size_t resolution, size_t tick_size);

	// void reset();
	// void add_tick(float location, color color);

	void add_dataset(void *key, color color);
	void remove_dataset(void *key);
	void feed(void *ctx, const dynarray<size_t> &line_starts, const dynarray<size_t> &poi_lines) {
		if (datasets_.find(ctx) == datasets_.end()) {
			assert(false);
			return; // no dataset for this context
		}
		datasets_.at(ctx).feed(line_starts, poi_lines);
	}

	void draw() override;
};
