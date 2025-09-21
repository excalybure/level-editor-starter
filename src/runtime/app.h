#pragma once

#include <chrono>
#include <thread>

namespace app
{
class App
{
public:
	void tick()
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for( 16ms );
	}
};
} // namespace app
