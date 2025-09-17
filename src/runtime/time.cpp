module runtime.time;

import <chrono>;

namespace runtime::time::detail
{
// Private implementation details
float getCurrentTimeImpl()
{
	static auto start = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<float>( now - start ).count();
}
} // namespace runtime::time::detail

namespace runtime::time
{
// Public interface implementation
float getCurrentTime()
{
	return detail::getCurrentTimeImpl();
}
} // namespace runtime::time