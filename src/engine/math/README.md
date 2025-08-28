# Engine Math Library

A comprehensive mathematical utilities library for game development and level editing, providing modular C++23 modules for vector operations, matrix transformations, 3D geometry, and more.

## 🚀 Implementation Requirements
- ✅ **Unit testing**: All implementations have comprehensive test coverage
- ✅ **Const-correctness**: All functions properly use const where appropriate
- ✅ **Modular architecture**: Individual modules for selective imports
- ✅ **CMake integration**: Build system configured for all modules

## 📊 Project Status
- **Total Test Coverage**: 5,246 assertions across 70 test cases
- **All Tests Passing**: ✅ 
- **Modules**: 16+ individual C++23 modules
- **Code Quality**: Const-correct, constexpr optimized

## 📁 Core Modules

### `math.ixx` - Fundamental Math Utilities
**Status: ✅ Complete**
- Mathematical constants (π, e, √2, √3)
- Angle conversion functions (radians ↔ degrees)
- Interpolation functions (lerp, smoothstep, smootherstep)
- Clamping and utility functions (clamp, sign, abs, square)
- Power and root functions (pow, sqrt)
- Trigonometric functions (sin, cos, tan, asin, acos, atan, atan2)
- Rounding functions (floor, ceil, round, frac)
- Modulo and wrap functions
- Power-of-two utilities (isPowerOfTwo, nextPowerOfTwo)
- **Advanced utilities**: Fast approximations, number theory, bit manipulation

### `vec.ixx` - Vector Operations
**Status: ✅ Complete**
- Vec2, Vec3, Vec4 classes with full arithmetic operations
- Dot and cross products
- Vector normalization and magnitude calculations
- Distance calculations and utilities
- Vector-specific functions (saturate, min, max, nearEqual)

### `matrix.ixx` - Matrix Operations  
**Status: ✅ Complete**
- Mat2, Mat3, Mat4 matrix classes
- Matrix arithmetic and transformations
- View and projection matrices
- Matrix decomposition and utilities

### `quat.ixx` - Quaternion Operations
**Status: ✅ Complete**
- Quaternion class with full arithmetic support
- Euler angle conversions
- Spherical linear interpolation (slerp)
- Rotation utilities and factory functions
### `color.ixx` - Color Math Functions
**Status: ✅ Complete**
- HSV ↔ RGB color space conversions
- Color interpolation (RGB and HSV with hue wrapping)  
- Gamma correction (linear ↔ sRGB)
- Perceptual functions (luminance calculation)
- Color utilities (saturation adjustment, color temperature)

### `random.ixx` - Random Number Utilities
**Status: ✅ Complete**
- `Random` class with Mersenne Twister seeded generation
- Range generation for floats and integers
- Geometric sampling (unit circle, unit sphere, inside sphere, inside cube)
- Container utilities (random choice, Fisher-Yates shuffle)
- Noise functions (Perlin noise, fractal noise, turbulence)
- Global convenience functions

### `animation.ixx` - Animation/Easing Functions
**Status: ✅ Complete**
- Complete easing function library (quadratic, cubic, quartic, sine)
- Special easing functions (bounce, elastic, back) with customizable parameters
- Utility functions (inverseLerp, remap)
- Type-safe EaseType enumeration and generic ease() dispatcher

## 📐 Geometry Modules

### `math_2d.ixx` - 2D Geometry Functions
**Status: ✅ Complete**
- Point-in-shape tests (circle, rectangle, triangle, polygon)
- Line and ray intersections with geometric shapes
- Distance calculations (point to line, point to segment)
- Area calculations (triangle, polygon with shoelace formula)
- `BoundingBox2D` with containment, intersection, and utility methods

### `math_3d.ixx` - 3D Geometry Functions  
**Status: ✅ Complete & Const-Correct**
- Point-in-shape tests (sphere, AABB, OBB, tetrahedron)
- Ray-shape intersections (sphere, AABB, triangle, plane)
- Line and segment intersection algorithms
- Distance calculations (point to plane, point to line, line to line)
- Geometric utilities (tetrahedron volume, triangle area, barycentric coordinates)
- **Recent improvements**: Full const-correctness review completed

### `bounding_volumes.ixx` - Modular Bounding Volume System
**Status: ✅ Complete - Modular Architecture**

**Individual Modules:**
- `bounding_box_2d.ixx` - 2D axis-aligned bounding boxes
- `bounding_box_3d.ixx` - 3D axis-aligned bounding boxes with corner access
- `bounding_sphere.ixx` - 3D spheres with expansion algorithms  
- `oriented_bounding_box.ixx` - 3D oriented boxes with SAT collision detection
- `plane.ixx` - 3D planes with distance and projection calculations
- `frustum.ixx` - 3D view frustums for culling operations

**Architecture Benefits:**
- **Selective imports**: Use only the bounding volumes you need
- **Convenience wrapper**: Import all via `engine.bounding_volumes`
- **Optimized compilation**: Individual modules compile independently
- **Backward compatibility**: Maintains existing API while enabling modular usage
## 🧪 Testing & Quality Assurance

**Comprehensive Test Coverage:**
- **Total assertions**: 5,246 across 70 test cases
- **Test categories**: Unit tests, integration tests, edge case validation
- **Quality measures**: Boundary condition testing, mathematical property verification
- **All tests passing**: ✅

**Code Quality Standards:**
- **Const-correctness**: All functions properly qualified with const where appropriate
- **Constexpr optimization**: Compile-time evaluation where possible
- **Template support**: Full float and double precision support
- **Performance-conscious**: Fast math alternatives and optimized implementations

## 🚀 Future Enhancements

### 🔢 **Curve and Spline Functions** (Priority: Medium)
```cpp
// Bezier curves
Vec2<T> quadraticBezier(Vec2<T> p0, Vec2<T> p1, Vec2<T> p2, T t);
Vec2<T> cubicBezier(Vec2<T> p0, Vec2<T> p1, Vec2<T> p2, Vec2<T> p3, T t);

// Catmull-Rom splines
Vec3<T> catmullRom(Vec3<T> p0, Vec3<T> p1, Vec3<T> p2, Vec3<T> p3, T t);

// Arc length parameterization
T arcLength(const std::vector<Vec2<T>>& points);
Vec2<T> sampleByDistance(const std::vector<Vec2<T>>& points, T distance);
```

### 📊 **Statistics and Data Functions** (Priority: Low)
```cpp
// Basic statistics
T mean(const std::vector<T>& values);
T median(std::vector<T> values);
T standardDeviation(const std::vector<T>& values);
T variance(const std::vector<T>& values);

// Moving averages
class MovingAverage {
    void addSample(T value);
    T getAverage() const;
    void setWindowSize(int size);
};
```

## � Implementation Notes

- **C++23 Modules**: Full module system with selective imports
- **Template design**: Support for both `float` and `double` precision
- **Naming convention**: Consistent camelCase throughout
- **Error handling**: Comprehensive edge case handling and input validation
- **Performance focus**: Optimized implementations with SIMD-friendly designs

## 🏗️ Build Instructions

```bash
# Configure with CMake preset
cmake --preset vs2022-x64

# Build all modules
cmake --build build/vs2022-x64

# Run tests
./build/vs2022-x64/Debug/math_tests.exe
```
---

## 📚 Module Usage Examples

### Basic Math Operations
```cpp
import engine.math;
import engine.vec;

auto angle = math::radians(45.0f);
auto result = math::lerp(0.0f, 100.0f, 0.5f);
auto clamped = math::clamp(value, 0.0f, 1.0f);
```

### Vector Operations  
```cpp
import engine.vec;

math::Vec3f position{1.0f, 2.0f, 3.0f};
math::Vec3f direction = math::normalize(velocity);
float distance = math::length(position - target);
```

### 3D Geometry
```cpp
import engine.math_3d;
import engine.bounding_volumes;

// Ray-sphere intersection
float hitDistance;
bool hit = math::raySphereIntersection(origin, direction, center, radius, hitDistance);

// Selective bounding volume imports
import engine.bounding_sphere;
BoundingSphere sphere{center, radius};
```

### Modular Bounding Volumes
```cpp
// Import only what you need
import engine.bounding_box_3d;
import engine.plane;

// Or import everything
import engine.bounding_volumes;
```

---

*Last updated: August 27, 2025*
