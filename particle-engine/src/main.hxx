#pragma once


#include <iostream>
#include <memory>
#include <vector>
#include <array>

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <fmt/format.h>


#include "config.hxx"
#include "utility/helpers.hxx"
#include "math/math.hxx"
#include "gfx/context.hxx"
#include "platform/input/input_manager.hxx"
#include "platform/window.hxx"

namespace app
{
	auto constexpr SCREEN_WIDTH = 1024u;
	auto constexpr SCREEN_HEIGHT = 768u;

	void render();
	void update(int dt);
	void on_click(int x, int y);

	void init(void);
	void term(void);
}
