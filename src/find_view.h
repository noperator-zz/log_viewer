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
		bool filtered {};
	};

	enum class Event {
		kCriteria,
		kPrev,
		kNext,
		kFilter,
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
	ButtonView but_prev_   {this, TexID::previousOccurence_dark, [this]{ on_prev(); }};
	ButtonView but_next_   {this, TexID::nextOccurence_dark,     [this]{ on_next(); }};
	ButtonView but_case_   {this, TexID::matchCase_dark,         [this]{ on_case(); }};
	ButtonView but_word_   {this, TexID::words_dark,             [this]{ on_word(); }};
	ButtonView but_regex_  {this, TexID::regex_dark,             [this]{ on_regex(); }};
	ButtonView but_filter_ {this, {TexID::radio_off, TexID::radio_off, TexID::radio_on, TexID::radio_off}, [this]{ on_filter(); }};
	layout::H layout_ {};

	void handle_text();

	void on_prev();
	void on_next();
	void on_case();
	void on_word();
	void on_regex();
	void on_filter();

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	void on_resize() override;

	void update() override;

public:
	FindView(Widget *parent, color color, std::function<void(FindView &, Event)> &&event_cb);

	// diable copy and move
	FindView(const FindView &) = delete;
	FindView &operator=(const FindView &) = delete;

	FindView(FindView &&) = delete;
	FindView &operator=(FindView &&) = delete;

	void set_state(State state);
	void set_filtered(bool filtered);

	const State &state() const;
	Flags flags() const;
	color color() const;
	std::string_view text() const;
	void draw() override;
};
