#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "window.h"

class Widget {
	friend class Window;

	struct State {
		// bool visible: 1;
		// bool enabled: 1;
		// bool focused: 1;
		bool hovered: 1;
		bool l_pressed: 1;
		bool r_pressed: 1;
		bool m_pressed: 1;
		// bool clicked: 1;
	};

	std::string name_;
	Widget *parent_ {};
	// Window *window_ {};
	std::vector<Widget*> children_ {};
	glm::ivec2 mouse_pos_ {};
	glm::ivec2 pressed_mouse_pos_ {};
	glm::ivec2 pressed_pos_ {};
	glm::ivec2 pos_ {};
	glm::ivec2 size_ {};
	bool dirty_ {true};

	bool cursor_pos_cb(glm::ivec2 mouse);
	bool mouse_button_cb(glm::ivec2 mouse, int button, int action, Window::KeyMods mods);
	bool key_cb(int key, int scancode, int action, Window::KeyMods mods);
	bool char_cb(unsigned int codepoint, Window::KeyMods mods);
	bool scroll_cb(glm::ivec2 offset, Window::KeyMods mods);
	bool drop_cb(int path_count, const char* paths[]);
	// bool frame_buffer_size_cb(int width, int height);
	// bool window_size_cb(int width, int height);
	// bool window_refresh_cb();
	void _on_resize();
	void clean_tree();
	[[nodiscard]] bool do_update();

	virtual void update() = 0;
	virtual void draw() = 0;

protected:
	class Scissor {
		int nesting_ {};
	public:
		Scissor(Widget *widget);
		~Scissor();
	};

	State state_ {};

	void add_child(Widget &child);
	void remove_child(Widget &child);

	virtual void on_resize() {}

	virtual void on_hover() {}
	virtual void on_unhover() {}

	// TODO should these default to return true to stop event propagation unless explicitly overridden?
	virtual bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) { return false; }

	virtual bool on_cursor_pos(glm::ivec2 pos) { return false; }
	virtual bool on_drag(glm::ivec2 offset) { return false; }

	virtual bool on_key(int key, int scancode, int action, Window::KeyMods mods) { return false; }
	virtual bool on_char(unsigned int codepoint, Window::KeyMods mods) { return false; }
	virtual bool on_scroll(glm::ivec2 offset, Window::KeyMods mods) { return false; }
	virtual bool on_drop(int path_count, const char* paths[]) { return false; }

	Scissor && scissor();

public:
	Widget();
	Widget(std::string_view name);
	virtual ~Widget() = default;

	[[nodiscard]] Widget *parent() const;
	// Window *window() const;
	[[nodiscard]] glm::ivec2 mouse_pos() const;
	[[nodiscard]] glm::ivec2 pos() const;
	[[nodiscard]] glm::ivec2 size() const;
	[[nodiscard]] glm::ivec2 rel_pos(glm::ivec2 abs_pos) const;
	[[nodiscard]] bool hovered() const;
	[[nodiscard]] bool pressed(int button = GLFW_MOUSE_BUTTON_LEFT) const;

	// [[nodiscard]] std::string_view type() const;
	[[nodiscard]] std::string_view name() const;
	[[nodiscard]] std::string path() const;

	std::tuple<int, int, int, int> resize(glm::ivec2 pos, glm::ivec2 size);
	void soil();
};
