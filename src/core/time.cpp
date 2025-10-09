#include "core/time.h"
#include <chrono>

namespace core::time
{
// Public interface implementation
float getCurrentTime()
{
	static auto start = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<float>( now - start ).count();
}
} // namespace core::time
