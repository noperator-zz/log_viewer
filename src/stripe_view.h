#pragma once
#include <memory>

#include "stripe_shader.h"
#include "widget.h"
#include "types.h"

class StripeView : public Widget {

	StripeView(const StripeView &) = delete;
	StripeView &operator=(const StripeView &) = delete;
	StripeView(StripeView &&) = delete;
	StripeView &operator=(StripeView &&) = delete;

	size_t resolution_;
	size_t point_size_;
	std::unique_ptr<StripeShader::LineStyle[]> points_;
	StripeShader::Buffer buf_ {};

	void on_resize() override;

	void update() override;

public:
	StripeView(size_t resolution, size_t point_size);

	void reset();
	void add_point(float location, color color);
	void draw() override;
};
