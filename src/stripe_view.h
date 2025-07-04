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

		size_t get_tick_line(size_t tick, size_t num_lines) const {
			return tick * num_lines / parent_.num_ticks_;
		}

		size_t get_line_tick(size_t line, size_t num_lines) const {
			return line * parent_.num_ticks_ / num_lines;
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
			while (free_tick < parent_.num_ticks_) {
				size_t tick_line = get_tick_line(free_tick, num_lines);
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

				tick_line = std::distance(line_starts.begin(), line_it);
				// prev_poi_line_ = tick_line;

				const float position = tick_line / (double)num_lines;
				size_t tick = get_line_tick(tick_line, num_lines);
				ticks_[tick] = {position, color_};

				free_tick = tick + 1;
			}

			//
			// // Check if lastest POI is at least one tick further than the last tick.
			// // If it's not, there's nothing more to do.
			// const size_t latest_char_pos = get_location_(&POIs.back());
			// assert(latest_char_pos < line_starts.back());
			//
			// const auto latest_line_it = std::lower_bound(line_starts.begin(), line_starts.end(), latest_char_pos);
			// assert(latest_line_it < line_starts.end());
			//
			// const auto latest_line = std::distance(line_starts.begin(), latest_line_it);
			// const auto latest_tick = get_line_tick(latest_line, num_lines);
			// const auto prev_tick = get_line_tick(prev_poi_line_, num_lines);
			// assert(latest_tick < parent_.num_ticks_);
			//
			// // // If the latest tick is the previous tick, and it's already filled, no need to update.
			// // if (latest_tick <= prev_tick && ticks_[prev_tick].fg == color_) {
			// // 	prev_poi_line_ = latest_line;
			// // 	return;
			// // }
			//
			// // For each tick, compute the starting and ending line for the tick, and check if any POI falls into that tick.
			// auto start_it = line_starts.begin() + prev_poi_line_; // get_tick_line(prev_tick, num_lines);
			// for (size_t tick = prev_tick; tick <= latest_tick; tick++) {
			// 	assert(tick < parent_.num_ticks_);
			// 	const auto end_it = line_starts.begin() + get_tick_line(tick + 1, num_lines);
			// 	assert(end_it <= line_starts.end());
			//
			// 	const auto first_match_it = std::lower_bound(start_it, end_it, latest_char_pos);
			// 	const auto last_match_it = std::upper_bound(first_match_it, end_it, latest_char_pos);
			//
			// 	if (first_match_it != last_match_it) {
			// 		// If we have a match, fill the tick.
			// 		const float position = (first_match_it - line_starts.begin()) / (double)num_lines;
			// 		ticks_[tick] = {position, color_};
			// 	}
			//
			// 	start_it = end_it;
			// }
			//
			// prev_poi_line_ = latest_line;

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
	StripeView(size_t resolution, size_t tick_size);

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
