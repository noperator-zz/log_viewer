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

		size_t first_line_for_tick(size_t tick, size_t num_lines) const;
		size_t middle_tick_for_line(size_t line, size_t num_lines) const;
		void reset();
		void update();
		void draw();

	public:
		Dataset(StripeView &parent,	color color);
		void feed(const dynarray<size_t> &line_starts, const dynarray<size_t> &poi_lines);
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
