#pragma once
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
	};

private:
	GLFWwindow *window_;
	Widget *root_;
	glm::ivec2 mouse_ {};
	KeyMods key_mods_ {};

	static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void char_cb(GLFWwindow* window, unsigned int codepoint);
	static void cursor_pos_cb(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_cb(GLFWwindow* window, int button, int action, int mods);
	static void scroll_cb(GLFWwindow* window, double xoffset, double yoffset);
	static void drop_cb(GLFWwindow* window, int path_count, const char* paths[]);
	static void frame_buffer_size_cb(GLFWwindow* window, int width, int height);
	static void window_size_cb(GLFWwindow* window, int width, int height);
	static void window_refresh_cb(GLFWwindow* window);

public:
	Window(GLFWwindow *window, Widget *root);
	~Window();

	void destroy();
	void swap_buffers() const;
	bool should_close() const;

	void draw();
};
