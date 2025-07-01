#pragma once

#include <functional>

#include "widget.h"
#include "types.h"
#include "input_view.h"
#include "button_view.h"

class FindView : public Widget {
	class HandleView : public Widget {
		friend class FindView;

		const FindView &parent_;

		void draw() override;

	public:
		HandleView(const FindView &parent);
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
	HandleView handle_ {*this};
	InputView input_ {[this](auto) { handle_text(); }};
	ButtonView but_prev_ {};
	ButtonView but_next_ {};
	ButtonView but_case_ {};
	ButtonView but_word_ {};
	ButtonView but_regex_ {};

	void handle_text();

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	void on_resize() override;

	// diable copy and move
	FindView(const FindView &) = delete;
	FindView &operator=(const FindView &) = delete;

	FindView(FindView &&) = delete;
	FindView &operator=(FindView &&) = delete;

public:
	FindView(std::function<void(FindView &)> &&on_find);

	Flags flags() const;
	color color() const;
	std::string_view text() const;
	void draw() override;
};
