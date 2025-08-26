export module engine.math;
import std;

export namespace math
{
struct Vec3
{
	float x{}, y{}, z{};
};
export constexpr float dot( const Vec3 &a, const Vec3 &b )
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
export constexpr Vec3 cross( const Vec3 &a, const Vec3 &b )
{
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
} // namespace math
