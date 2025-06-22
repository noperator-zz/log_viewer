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
	color color_ {};
	Flags flags_ {};
	HandleView handle_ {*this};
	InputView input_ {nullptr};
	ButtonView but_prev_ {nullptr};
	ButtonView but_next_ {nullptr};
	ButtonView but_case_ {nullptr};
	ButtonView but_word_ {nullptr};
	ButtonView but_regex_ {nullptr};

	bool on_char(unsigned int codepoint, Window::KeyMods mods) override;
	void on_resize() override;

public:
	FindView(std::function<void(const FindView &)> on_find);

	Flags flags() const;
	color color() const;
	void draw() override;
};
