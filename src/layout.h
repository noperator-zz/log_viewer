#pragma once

#include <type_traits>
#include <glm/glm.hpp>

#include "resizable.h"

namespace layout {

enum class SizeType {
	Pixel,
	Percent,
	Remain,
};

struct Pixel {
	size_t px;
};

// struct Percent {
// 	size_t pct;
// };

struct Remain {
	size_t pct;
};

template<typename T>
concept IsLayoutSize = std::is_integral_v<T>
|| std::is_same_v<T, Pixel>
// || std::is_same_v<T, Percent>
|| std::is_same_v<T, Remain>
;


// concept for any class that has resize() method taking glm::ivec2 and glm::ivec2
template<typename T>
concept HasResize = requires(T t, glm::ivec2 pos, glm::ivec2 size) {
	{ t.resize(pos, size) };
};

template<bool V>
class Layout : public Resizable {
	struct WidgetSize {
		Resizable *widget;
		SizeType type;
		size_t size;
	};

	std::vector<WidgetSize> widgets_;

public:
	Layout() = default;

	template<IsLayoutSize T>
	void add(Resizable &widget, T size) {
		return add(&widget, size);
	}

	template<IsLayoutSize T>
	void add(Resizable *widget, T size) {
		if constexpr (std::is_integral_v<T>) {
			widgets_.push_back({widget, SizeType::Pixel, static_cast<size_t>(size)});
		} else if constexpr (std::is_same_v<T, Pixel>) {
			widgets_.push_back({widget, SizeType::Pixel, size.px});
		// } else if constexpr (std::is_same_v<T, Percent>) {
		// 	widgets_.push_back({widget, SizeType::Percent, size.pct});
		} else if constexpr (std::is_same_v<T, Remain>) {
			widgets_.push_back({widget, SizeType::Remain, size.pct});
		} else {
			static_assert(false, "Invalid layout size type");
		}
	}

	void apply(const Resizable &parent) {
		pos_ = parent.pos();
		size_ = parent.size();

		return apply();
	}

	void apply() {
		glm::ivec2 cursor = pos_;

		size_t total_px {};
		size_t total_share_pct {};

		for (const auto &ws : widgets_) {
			if (ws.type == SizeType::Pixel) {
				total_px += ws.size;
			} else if (ws.type == SizeType::Remain) {
				total_share_pct += ws.size;
			}
		}

		size_t remain_size = total_px < size_[V] ? size_[V] - total_px : 0;

		for (const auto &ws : widgets_) {
			size_t px {};

			if (ws.type == SizeType::Pixel) {
				px = ws.size;
			} else if (ws.type == SizeType::Remain) {
				px = remain_size * ws.size / total_share_pct;
			}

			glm::ivec2 widget_size = {px, size_[!V]};
			if (ws.widget) {
				ws.widget->resize(cursor, glm::ivec2 {widget_size[V], widget_size[!V]});
			}
			cursor[V] += px;
		}

		for (const auto &ws : widgets_) {
			if (ws.widget) {
				ws.widget->apply();
			}
		}
	}
};

using H = Layout<false>;
using V = Layout<true>;
}
