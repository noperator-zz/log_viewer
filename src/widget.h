#pragma once
#include <vector>
#include <glm/vec2.hpp>

class IWidget {
	friend class App;

	struct State {
		// bool visible: 1;
		// bool enabled: 1;
		// bool focused: 1;
		bool hovered: 1;
		bool pressed: 1;
		// bool clicked: 1;
	};

	std::vector<IWidget*> children_;
	glm::uvec2 pos_;
	glm::uvec2 size_;

	bool handle_mouse_button(int button, int action, int mods);
	bool handle_cursor_pos(glm::uvec2 pos);

protected:
	State state_ {};

	glm::uvec2 pos() const;
	glm::uvec2 size() const;
	bool hovered() const;
	bool pressed() const;

	void add_child(IWidget *child);
	void remove_child(IWidget *child);

	virtual void on_resize() {}

	virtual void on_hover() {}
	virtual void on_unhover() {}

	virtual void on_press() {}
	virtual void on_release() {}

	virtual void on_cursor_pos(glm::uvec2 pos) {}

public:
	IWidget(glm::uvec2 pos, glm::uvec2 size);
	virtual ~IWidget() = default;

	void resize(glm::uvec2 pos, glm::uvec2 size);
	virtual void draw() {};
};
