#pragma once

#include <memory>
#include <vector>
#include "file_view.h"
#include "text_shader.h"
#include "gp_shader.h"
#include "widget.h"

struct GLFWwindow;

class App : public Widget {
	static std::unique_ptr<App> app_;

	std::unique_ptr<Font> font_ {};
	std::unique_ptr<TextShader> text_shader_ {};
	std::unique_ptr<GPShader> gp_shader_ {};
	GLFWwindow *window_ {};
	glm::ivec2 mouse_ {};
	glm::ivec2 fb_size_ {};

	bool shift_held_ {};

	std::vector<std::unique_ptr<FileView>> file_views_;

	int start();
	int add_file(const char *path);
	void draw() override;
	int run();

	static void static_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
	void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void static_cursor_pos_cb(GLFWwindow* window, double xpos, double ypos);
	void cursor_pos_cb(GLFWwindow* window, double xpos, double ypos);
	static void static_mouse_button_cb(GLFWwindow* window, int button, int action, int mods);
	void mouse_button_cb(GLFWwindow* window, int button, int action, int mods);
	static void static_scroll_cb(GLFWwindow* window, double xoffset, double yoffset);
	void scroll_cb(GLFWwindow* window, double xoffset, double yoffset);
	static void static_drop_cb(GLFWwindow* window, int path_count, const char* paths[]);
	void drop_cb(GLFWwindow* window, int path_count, const char* paths[]);
	static void static_frame_buffer_size_cb(GLFWwindow* window, int width, int height);
	void frame_buffer_size_cb(GLFWwindow* window, int width, int height);
	static void static_window_size_cb(GLFWwindow* window, int width, int height);
	void window_size_cb(GLFWwindow* window, int width, int height);
	static void static_window_refresh_cb(GLFWwindow* window);
	void window_refresh_cb(GLFWwindow* window);

	void on_cursor_pos(glm::ivec2 pos) override;
	void on_resize() override;

	FileView& active_file_view();

public:
	App();
	static void create(int argc, char *argv[]);
};
