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
		// template <typename T>
		// std::function<bool(const T&, const T&)> compare_;
		std::function<size_t(const void*)> get_location_;
		std::unique_ptr<StripeShader::LineStyle[]> ticks_;
		StripeShader::Buffer buf_ {};

		size_t prev_num_lines_ {};
		size_t prev_num_pois_ {};
		size_t prev_poi_idx_ {};
		// size_t prev_poi_line_ {};
		size_t prev_free_tick_ {};

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
			prev_num_pois_ = 0;
			prev_poi_idx_ = 0;
			// prev_poi_line_ = 0;
			prev_free_tick_ = 0;
			std::memset(ticks_.get(), 0, parent_.num_ticks_ * sizeof(ticks_[0]));
		}

		void update() {
			StripeShader::update(buf_, ticks_.get());
		}

		void draw() {
			StripeShader::draw(buf_, parent_.pos(), parent_.size(), parent_.tick_size_, Z_UI_FG);
		}

	public:
		// template <typename T>
		Dataset(StripeView &parent,	color color, std::function<size_t(const void*)> &&get_location)
		: parent_(parent), color_(color), get_location_(std::move(get_location))
		, ticks_(new StripeShader::LineStyle[parent_.num_ticks_]) {
			StripeShader::create_buffers(buf_, parent_.num_ticks_);
			reset();
		}

		// const dynarray<size_t> &line_starts_;
		// const dynarray<T> POIs_;
		// line_starts_(line_starts), POIs_(POIs),
		template <typename T>
		void feed(const dynarray<size_t> &line_starts, const dynarray<T> &POIs) {
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

			size_t free_tick = prev_free_tick_;
			size_t tick_line = 0;
			bool first = true;
			while (free_tick < parent_.num_ticks_) {
				const auto line = first_line_for_tick(free_tick, num_lines);
				if (first) {
					tick_line = line;
					first = false;
				} else {
					tick_line = std::max(tick_line + 1, line);
				}

				size_t char_pos = line_starts[tick_line];
				const auto poi_it = std::lower_bound(POIs.begin() + prev_poi_idx_, POIs.end(), char_pos);

				if (poi_it == POIs.end()) {
					prev_poi_idx_ = POIs.size();
					break;
				}
				prev_poi_idx_ = std::distance(POIs.begin(), poi_it);

				char_pos = get_location_(poi_it);
				const auto line_it = std::lower_bound(line_starts.begin() + tick_line, line_starts.end(), char_pos);
				if (line_it == line_starts.end()) {
					// This could happen due to line_starts being out of date compared to POIs.
					// TODO what best to do here?
					break;
				}
				// assert(line_it != line_starts.end());

				const size_t tick_line2 = std::distance(line_starts.begin(), line_it);
				// prev_poi_line_ = tick_line2;

				const float position = tick_line2 / (double)num_lines;
				size_t tick = middle_tick_for_line(tick_line2, num_lines);
				ticks_[tick] = {position, color_};

				// // If there are more ticks than lines, tick could be < free_tick here, so std::max() them to ensure
				// //  we don't loop forever.
				// // TODO This solution is hacky and inefficient since it will cause every single tick to be processed
				// //  even if there are fewer lines than ticks.
				// //  Better would be to update get_line_tick() to return the _last_ tick associated with a line.
				// free_tick = std::max(tick, free_tick) + 1;
				free_tick = tick + 1;
			}

			if (free_tick != prev_free_tick_) {
				prev_free_tick_ = free_tick;
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

	void add_dataset(void *key, color color, std::function<size_t(const void*)> &&get_location);
	void remove_dataset(void *key);
	template <typename T>
	void feed(void *ctx, const dynarray<size_t> &line_starts, const dynarray<T> &POIs) {
		if (datasets_.find(ctx) == datasets_.end()) {
			assert(false);
			return; // no dataset for this context
		}
		datasets_.at(ctx).feed(line_starts, POIs);
	}

	void draw() override;
};
