#pragma once
#include <functional>
#include <glm/fwd.hpp>

#include "gp_shader.h"
#include "widget.h"


class Scrollbar : public Widget {
	class Thumb : public Widget {
		friend class Scrollbar;
		glm::u8vec4 color_ {100, 0, 0, 255};
		std::function<void(glm::ivec2)> scroll_cb_;

		void on_drag(glm::ivec2 offset) override;
		void draw() override;

	public:
		Thumb(std::function<void(glm::ivec2)> scroll_cb);
	};

	std::function<void(double)> scroll_cb_;
	Thumb thumb_;
	bool horizontal_;
	double position_percent_ {};
	double visible_percent_ {};

	void thumb_cb(glm::ivec2 offset);

public:
	Scrollbar(bool horizontal, std::function<void(double)> scroll_cb);
	void on_resize() override;
	void draw() override;

	void resize_thumb();
	void set(size_t position, size_t visible_extents, size_t total_extents);
};
