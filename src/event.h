#pragma once

#include <GLFW/glfw3.h>

inline void send_event() {
	glfwPostEmptyEvent();
}
