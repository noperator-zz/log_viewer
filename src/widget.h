#pragma once
#include <vector>
#include <glm/vec2.hpp>

class Widget {
	friend class WidgetManager;

	struct State {
		// bool visible: 1;
		// bool enabled: 1;
		// bool focused: 1;
		bool hovered: 1;
		bool pressed: 1;
		// bool clicked: 1;
	};

	std::vector<Widget*> children_;
	Widget *parent_;
	glm::ivec2 pos_;
	glm::ivec2 size_;
	glm::ivec2 pressed_mouse_pos_ {};
	glm::ivec2 pressed_pos_ {};

	bool handle_mouse_button(int button, int action, int mods);
	bool handle_cursor_pos(glm::ivec2 mouse);

	void _on_resize();

protected:
	State state_ {};

	void add_child(Widget *child);
	void remove_child(Widget *child);

	virtual void on_resize() {}

	virtual void on_hover() {}
	virtual void on_unhover() {}

	virtual void on_press() {}
	virtual void on_release() {}

	virtual void on_cursor_pos(glm::ivec2 pos) {}
	virtual void on_drag(glm::ivec2 offset) {}

public:
	Widget(Widget *parent, glm::ivec2 pos, glm::ivec2 size);
	virtual ~Widget() = default;

	Widget *parent() const;
	glm::ivec2 pos() const;
	glm::ivec2 size() const;
	bool hovered() const;
	bool pressed() const;

	void resize(glm::ivec2 pos, glm::ivec2 size);
	virtual void draw() {};
};

class WidgetManager {
	friend class Widget;
	static Widget *root_;
	static glm::ivec2 mouse_;

public:
	static bool handle_mouse_button(int button, int action, int mods);
	static bool handle_cursor_pos(glm::ivec2 pos);

	static void set_root(Widget *root);
};
