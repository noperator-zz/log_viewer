#pragma once
#include <condition_variable>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

class Widget;

class Window {
	friend class Widget;
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
	Widget *root_;
	glm::ivec2 mouse_ {};
	KeyMods key_mods_ {};
	bool fullscreen_ {};
	static inline std::mutex event_mtx_ {};
	static inline std::condition_variable event_cv_ {};
	static inline bool event_pending_ {};
	// static inline std::atomic_flag event_pending_ {};

	static void key_cb(GLFWwindow *glfw_window, int key, int scancode, int action, int mods);
	static void char_cb(GLFWwindow *glfw_window, unsigned int codepoint);
	static void cursor_pos_cb(GLFWwindow *glfw_window, double xpos, double ypos);
	static void mouse_button_cb(GLFWwindow *glfw_window, int button, int action, int mods);
	static void scroll_cb(GLFWwindow *glfw_window, double xoffset, double yoffset);
	static void drop_cb(GLFWwindow *glfw_window, int path_count, const char* paths[]);
	static void frame_buffer_size_cb(GLFWwindow *glfw_window, int width, int height);
	static void window_size_cb(GLFWwindow *glfw_window, int width, int height);
	static void window_refresh_cb(GLFWwindow *glfw_window);

	static void send_event();

	void swap_buffers() const;

public:
	Window(GLFWwindow *window, Widget *root);
	~Window();

	void toggle_fullscreen();

	void destroy();
	[[nodiscard]] bool should_close() const;

	// void wait_events();
	bool draw() const;
};
