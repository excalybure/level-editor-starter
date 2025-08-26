#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
import engine.math;

using Catch::Matchers::WithinRel;
TEST_CASE("Dot and cross", "[math]"){
    math::Vec3 x{1,0,0}, y{0,1,0};
    REQUIRE_THAT(math::dot(x,x), WithinRel(1.0f));
    auto c = math::cross(x,y);
    REQUIRE_THAT(c.z, WithinRel(1.0f));
}
