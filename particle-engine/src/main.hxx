#pragma once


#include <iostream>
#include <sstream>
#include <array>
#include <vector>

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <fmt/format.h>


#include "config.hxx"
#include "utility/helpers.hxx"
#include "gfx/context.hxx"
#include "platform/input/input_manager.hxx"
#include "platform/window.hxx"

namespace app
{
	const float SCREEN_WIDTH = 1024;
	const float SCREEN_HEIGHT = 768;

	void render(void); // Only platform::drawPoint should be used
	void update(int dt); // dt in milliseconds
	void on_click(int x, int y); // x, y - in pixels

	void init(void);
	void term(void);
}
