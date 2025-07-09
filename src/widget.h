#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "resizable.h"
#include "window.h"

class Widget : public Resizable {
	friend class Window;

	struct State {
		// bool visible: 1;
		// bool enabled: 1;
		// bool focused: 1;
		// bool hovered: 1;
		// bool l_pressed: 1;
		// bool r_pressed: 1;
		// bool m_pressed: 1;
		// bool clicked: 1;
	};

	std::string name_;
	Widget *parent_;
	Window *window_;
	std::vector<Widget*> children_ {};
	std::atomic<bool> dirty_ {true};
	// State state_ {};


	bool hit_test(const glm::uvec2 &point) const;
	Widget *deepest_hit_child(glm::uvec2 point);
	bool soil_if_handled(bool handled);

	void hover_cb(bool state);
	bool cursor_pos_cb(glm::ivec2 mouse);
	bool mouse_button_cb(glm::ivec2 mouse, int button, int action, Window::KeyMods mods);
	bool key_cb(int key, int scancode, int action, Window::KeyMods mods);
	bool char_cb(unsigned int codepoint, Window::KeyMods mods);
	bool scroll_cb(glm::ivec2 offset, Window::KeyMods mods);
	bool drop_cb(int path_count, const char* paths[]);
	// void hover_cb();
	// void unhover_cb();
	void drag_cb(glm::ivec2 offset);
	void _on_resize();
	[[nodiscard]] bool do_update();

	virtual void update() = 0;
	virtual void draw() = 0;

	Widget(const Widget &) = delete;
	Widget &operator=(const Widget &) = delete;
	Widget(Widget &&) = delete;
	Widget &operator=(Widget &&) = delete;

protected:
	class Scissor {
		int nesting_ {};
	public:
		Scissor(Widget *widget);
		~Scissor();
	};

	// const Window &window() const;
	void add_child(Widget &child);
	void remove_child(Widget &child);

	virtual void on_resize() {}


	// TODO should these default to return true to stop event propagation unless explicitly overridden?
	virtual void on_hover(bool state) {}
	virtual bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) { return false; }
	virtual bool on_cursor_pos(glm::ivec2 pos) { return false; }
	virtual bool on_drag(glm::ivec2 offset) { return false; }
	virtual bool on_key(int key, int scancode, int action, Window::KeyMods mods) { return false; }
	virtual bool on_char(unsigned int codepoint, Window::KeyMods mods) { return false; }
	virtual bool on_scroll(glm::ivec2 offset, Window::KeyMods mods) { return false; }
	virtual bool on_drop(int path_count, const char* paths[]) { return false; }

	// Scissor && scissor();

public:
	Widget(Widget *parent);
	Widget(Widget *parent, std::string_view name);
	Widget(Window &window, Widget *parent, std::string_view name);
	virtual ~Widget() = default;

	template<typename T=Widget>
	[[nodiscard]] T *parent() { return static_cast<T *>(parent_); }
	[[nodiscard]] Window *window();
	[[nodiscard]] glm::ivec2 mouse_pos() const;
	[[nodiscard]] glm::ivec2 pos() const;
	[[nodiscard]] glm::ivec2 size() const;
	[[nodiscard]] glm::ivec2 rel_pos(glm::ivec2 abs_pos) const;
	[[nodiscard]] bool hovered() const;
	[[nodiscard]] bool pressed(int button = GLFW_MOUSE_BUTTON_LEFT) const;

	// [[nodiscard]] std::string_view type() const;
	[[nodiscard]] std::string_view name() const;
	[[nodiscard]] std::string path() const;

	std::tuple<int, int, int, int> resize(glm::ivec2 pos, glm::ivec2 size) override;
	void soil();
};
