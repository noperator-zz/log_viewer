#pragma once
#include <vector>
#include <glm/vec2.hpp>

class Window;

class Widget {
	friend class Window;

	struct State {
		// bool visible: 1;
		// bool enabled: 1;
		// bool focused: 1;
		bool hovered: 1;
		bool pressed: 1;
		// bool clicked: 1;
	};

	Widget *parent_ {};
	// Window *window_ {};
	std::vector<Widget*> children_ {};
	glm::ivec2 pressed_mouse_pos_ {};
	glm::ivec2 pressed_pos_ {};
	glm::ivec2 pos_ {};
	glm::ivec2 size_ {};

	bool cursor_pos_cb(glm::ivec2 mouse);
	bool mouse_button_cb(glm::ivec2 mouse, int button, int action, int mods);
	bool key_cb(int key, int scancode, int action, int mods);
	bool scroll_cb(glm::ivec2 offset);
	bool drop_cb(int path_count, const char* paths[]);
	// bool frame_buffer_size_cb(int width, int height);
	// bool window_size_cb(int width, int height);
	// bool window_refresh_cb();

	void draw_cb();

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

	virtual bool on_key(int key, int scancode, int action, int mods) { return false; }
	virtual bool on_scroll(glm::ivec2 offset) { return false; }
	virtual bool on_drop(int path_count, const char* paths[]) { return false; }

public:
	Widget() = default;
	virtual ~Widget() = default;

	Widget *parent() const;
	// Window *window() const;
	glm::ivec2 pos() const;
	glm::ivec2 size() const;
	bool hovered() const;
	bool pressed() const;

	void resize(glm::ivec2 pos, glm::ivec2 size);
	virtual void draw() {};
};
