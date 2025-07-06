#pragma once
#include <functional>
#include <glm/fwd.hpp>

#include "gp_shader.h"
#include "widget.h"


class Scrollbar : public Widget {
	class Thumb : public Widget {
		friend class Scrollbar;
		color color_ {100, 0, 0, 255};
		std::function<void(int)> scroll_cb_;

		bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
		bool on_drag(glm::ivec2 offset) override;

		void update() override;
		void draw() override;

		bool vert();

	public:
		Thumb(Widget *parent, std::function<void(int)> &&scroll_cb);
	};

	std::function<void(double)> scroll_cb_;
	Thumb thumb_;
	double position_percent_ {};
	double visible_percent_ {};
	bool vert_;

	void on_resize() override;
	bool on_cursor_pos(glm::ivec2 pos) override;
	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;

	size_t scroll_range() const;
	void thumb_cb(int offset) const;
	void scroll_to_mouse(glm::ivec2 mouse) const;
	void scroll_toward_mouse(glm::ivec2 mouse, glm::uint pages) const;
	void resize_thumb();

	void update() override;

public:
	Scrollbar(Widget *parent, bool vertical, std::function<void(double)> &&scroll_cb);

	void draw() override;
	void set(size_t position, size_t visible_extents, size_t total_extents);
};
