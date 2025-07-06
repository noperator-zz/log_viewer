#pragma once
#include <condition_variable>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

class Widget;

class Window {
	// friend class App;
public:
	struct KeyMods {
		int shift : 1;
		int control : 1;
		int alt : 1;
		int super : 1;
		int caps_lock : 1;
		int num_lock : 1;

		bool special() const {
			return control || alt || super;
		}

		KeyMods() = default;
		KeyMods(int mods)
			: shift {(mods & GLFW_MOD_SHIFT) != 0}
			, control {(mods & GLFW_MOD_CONTROL) != 0}
			, alt {(mods & GLFW_MOD_ALT) != 0}
			, super {(mods & GLFW_MOD_SUPER) != 0}
			, caps_lock {(mods & GLFW_MOD_CAPS_LOCK) != 0}
			, num_lock {(mods & GLFW_MOD_NUM_LOCK) != 0}
		{}
	};

private:
	GLFWwindow *window_;
	Widget &root_;
	Widget *hovered_ {};
	Widget *key_active_ {};
	Widget *mouse_active_ {};

	glm::ivec2 mouse_ {};

	KeyMods key_mods_ {};
	bool fullscreen_ {};
	static inline std::mutex event_mtx_ {};
	static inline std::condition_variable event_cv_ {};
	static inline bool event_pending_ {};
	// static inline std::atomic_flag event_pending_ {};

	void update_hovered();

	void key_cb(int key, int scancode, int action, int mods);
	void char_cb(unsigned int codepoint);
	void cursor_pos_cb(double xpos, double ypos);
	void mouse_button_cb(int button, int action, int mods);
	void scroll_cb(double xoffset, double yoffset);
	void drop_cb(int path_count, const char* paths[]);
	void frame_buffer_size_cb(int width, int height);
	void window_size_cb(int width, int height);
	virtual void window_refresh_cb() {};


	template<typename T, typename... Args>
	void invoke_on(T *widget, bool (Widget::*cb)(Args...), Args... args) {
		while (widget) {
			if ((widget->*cb)(args...)) {
				return;
			}
			widget = widget->parent_;
		}
	}

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;
	Window(Window &&) = delete;
	Window &operator=(Window &&) = delete;

protected:
	glm::ivec2 fb_size_ {};
	void destroy();
	void swap_buffers() const;
	void toggle_fullscreen();
	[[nodiscard]] bool should_close() const;
	// void wait_events();
	bool draw() const;

public:
	Window(GLFWwindow *window, Widget &root);

	virtual ~Window();

	static void send_event();

	void set_key_focus(Widget *widget);
	glm::uvec2 mouse() const;
};
