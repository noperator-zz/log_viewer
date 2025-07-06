#pragma once

#include <functional>

#include "widget.h"
#include "types.h"
#include "input_view.h"
#include "button_view.h"

class FindView : public Widget {
	class HandleView : public Widget {
		friend class FindView;

		void update() override;
		void draw() override;

	public:
		HandleView(Widget *parent);
	};

public:
	struct Flags {
		bool case_sensitive {};
		bool whole_word {};
		bool regex {};
	};

private:
	std::function<void(FindView &)> on_find_ {};
	color color_ {};
	Flags flags_ {};
	HandleView handle_ {this};
	InputView input_ {this, [this](auto) { handle_text(); }};
	ButtonView but_prev_ {this};
	ButtonView but_next_ {this};
	ButtonView but_case_ {this};
	ButtonView but_word_ {this};
	ButtonView but_regex_ {this};

	void handle_text();

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	void on_resize() override;

	// diable copy and move
	FindView(const FindView &) = delete;
	FindView &operator=(const FindView &) = delete;

	FindView(FindView &&) = delete;
	FindView &operator=(FindView &&) = delete;

	void update() override;

public:
	FindView(Widget *parent, std::function<void(FindView &)> &&on_find);

	Flags flags() const;
	color color() const;
	std::string_view text() const;
	void draw() override;
};
