#pragma once

#include <glm/glm.hpp>

static constexpr uint8_t Z_FILEVIEW_BG = 0xFE;
static constexpr uint8_t Z_FILEVIEW_TEXT_FG = 0xFD;

static constexpr uint8_t Z_UI_BG_B = 0xFB;
static constexpr uint8_t Z_UI_BG_A = 0xFA;
static constexpr uint8_t Z_UI_BG_9 = 0xF9;
static constexpr uint8_t Z_UI_BG_8 = 0xF8;
static constexpr uint8_t Z_UI_BG_7 = 0xF7;
static constexpr uint8_t Z_UI_BG_6 = 0xF6;
static constexpr uint8_t Z_UI_BG_5 = 0xF5;
static constexpr uint8_t Z_UI_BG_4 = 0xF4;
static constexpr uint8_t Z_UI_BG_3 = 0xF3;
static constexpr uint8_t Z_UI_BG_2 = 0xF2;
static constexpr uint8_t Z_UI_BG_1 = 0xF1;
static constexpr uint8_t Z_UI_FG = 0xF0;


template <typename T>
struct rect {
	glm::vec<2, T> tl;
	glm::vec<2, T> br;

	static rect from_tl_br(const glm::vec<2, T> &tl, const glm::vec<2, T> &br) {
		return {tl, br};
	}
	static rect from_tl_size(const glm::vec<2, T> &tl, const glm::vec<2, T> &size) {
		return {tl, tl + size};
	}
	static rect from_center_size(const glm::vec<2, T> &center, const glm::vec<2, T> &size) {
		glm::vec<2, T> half_size = size / static_cast<T>(2);
		return {center - half_size, center + size - half_size};
	}

	bool contains(const glm::vec<2, T> &p) const {
		return p.x >= tl.x && p.x <= br.x && p.y >= tl.y && p.y <= br.y;
	}
	bool contains(const rect &other) const {
		return contains(other.tl) && contains(other.br);
	}
	glm::vec<2, T> size() const {
		return br - tl;
	}
	glm::vec<2, T> center() const {
		return (tl + br) / static_cast<T>(2);
	}
};
