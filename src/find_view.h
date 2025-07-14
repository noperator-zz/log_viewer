#pragma once

#include <functional>

#include "widget.h"
#include "types.h"
#include "input_view.h"
#include "button_view.h"
#include "label_view.h"
#include "layout.h"

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

	enum class Event {
		kCriteria,
		kPrev,
		kNext,
	};

	struct State {
		size_t total_matches {};
		size_t current_match {};
		bool bad_pattern {};
	};

private:
	const color color_;
	std::function<void(FindView &, Event)> event_cb_;
	State state_ {};
	Flags flags_ {};
	HandleView handle_ {this};
	InputView input_ {this, [this](auto) { handle_text(); }};
	LabelView match_label_ {this};
	ButtonView but_prev_ {this, TexID::previousOccurence, false, [this](bool){ event_cb_(*this, Event::kPrev); }};
	ButtonView but_next_ {this, TexID::nextOccurence, false, [this](bool){ event_cb_(*this, Event::kNext); }};
	ButtonView but_case_ {this, TexID::matchCase, true};
	ButtonView but_word_ {this, TexID::words, true};
	ButtonView but_regex_ {this, TexID::regex, true};
	layout::H layout_ {};

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
	FindView(Widget *parent, color color, std::function<void(FindView &, Event)> &&event_cb);

	void set_state(State state);

	const State &state() const;
	Flags flags() const;
	color color() const;
	std::string_view text() const;
	void draw() override;
};
