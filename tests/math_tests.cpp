#include <catch2/catch_test_macros.hpp>
import engine.math;
TEST_CASE("Dot and cross", "[math]"){
    math::Vec3 x{1,0,0}, y{0,1,0};
    REQUIRE(math::dot(x,x)==Approx(1));
    auto c=math::cross(x,y);
    REQUIRE(c.z==Approx(1));
}
