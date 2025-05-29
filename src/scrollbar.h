#pragma once
#include <functional>
#include <glm/fwd.hpp>

#include "gp_shader.h"
#include "widget.h"


class Scrollbar : public Widget {
	class Thumb : public Widget {
		friend class Scrollbar;
		glm::u8vec4 color_ {100, 0, 0, 255};
		std::function<void(double)> scroll_cb_;

		void on_drag(glm::ivec2 offset) override;
		void draw() override;

	public:
		Thumb(Widget &parent, glm::ivec2 pos, glm::ivec2 size, std::function<void(double)> scroll_cb);
	};

	Thumb thumb_;
	double position_percent_ {};
	double visible_percent_ {};

public:
	Scrollbar(Widget &parent, glm::ivec2 pos, glm::ivec2 size, std::function<void(double)> scroll_cb)
		: Widget(&parent, pos, size), thumb_(*this, pos, {30, 50}, scroll_cb) {
		add_child(&thumb_);
	}

	void on_resize() override;
	void draw() override;

	void resize_thumb();
	void set(size_t position, size_t visible_extents, size_t total_extents);
};
