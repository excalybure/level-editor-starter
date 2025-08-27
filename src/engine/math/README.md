# Engine Math Library

This directory contains the math modules for the engine, providing comprehensive mathematical utilities for game development and level editing.

## 📁 Current Modules

### `math.ixx` - Core Math Utilities
**Status: ✅ Complete**
- Mathematical constants (π, e, √2, √3)
- Angle conversion functions (radians ↔ degrees)
- Interpolation functions (lerp)
- Clamping and utility functions (clamp, sign, abs, square)
- Power and root functions (pow, sqrt)
- Trigonometric functions (sin, cos, tan, asin, acos, atan, atan2)
- Smoothing functions (smoothstep, smootherstep)
- Rounding functions (floor, ceil, round, frac)
- Modulo and wrap functions
- Step function
- Power-of-two utilities (isPowerOfTwo, nextPowerOfTwo)

### `color.ixx` - Color Math Functions
**Status: ✅ Complete**
- HSV ↔ RGB color space conversions
- Color interpolation (RGB and HSV with hue wrapping)
- Gamma correction (linear ↔ sRGB)
- Perceptual functions (luminance calculation)
- Color utilities (saturation adjustment, color temperature)

### `vec.ixx` - Vector Operations
**Status: ✅ Complete**
- Vec2, Vec3, Vec4 classes with arithmetic operations
- Dot and cross products
- Vector normalization and magnitude
- Distance calculations
- Vector-specific utilities (saturate, min, max, nearEqual)

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

## 🚀 Future Enhancement Roadmap

### 🎨 **Color Math Functions** (Priority: High) ✅ **COMPLETED**
Essential for UI, rendering, and visual effects in a level editor.

**✅ Implemented Functions:**
- `hsvToRgb(hue, saturation, value)` - HSV to RGB color conversion
- `rgbToHsv(red, green, blue)` - RGB to HSV color conversion  
- `lerpRgb(r1, g1, b1, r2, g2, b2, t)` - RGB color interpolation
- `lerpHsv(h1, s1, v1, h2, s2, v2, t)` - HSV color interpolation with hue wrapping
- `linearToGamma(linear)` & `gammaToLinear(gamma)` - sRGB gamma correction
- `luminance(red, green, blue)` - Perceptual luminance calculation
- `adjustSaturation(red, green, blue, factor)` - Color saturation adjustment
- `temperatureToRgb(kelvin)` - Color temperature to RGB conversion

**🧪 Test Coverage:** 96 assertions across 5 test cases covering all functions, edge cases, and round-trip conversions.

### 🎲 **Random Number Utilities** (Priority: High)
Essential for procedural generation, testing, and dynamic content.
```cpp
// Seeded random number generator class
class Random {
    void seed(unsigned int seed);
    T range(T min, T max);           // Random value in range
    int range(int min, int max);     // Random integer in range
    Vec2<T> unitCircle();            // Random point on unit circle
    Vec3<T> unitSphere();            // Random point on unit sphere
    Vec3<T> insideSphere();          // Random point inside unit sphere
};

// Noise functions for terrain/texture generation
T perlinNoise(T x, T y);
T simplexNoise(T x, T y);
T fractalNoise(T x, T y, int octaves);
```

### 🌊 **Easing/Animation Functions** (Priority: High)
Critical for smooth animations and user interface transitions.
```cpp
// Standard easing functions
T easeInQuad(T t);      T easeOutQuad(T t);      T easeInOutQuad(T t);
T easeInCubic(T t);     T easeOutCubic(T t);     T easeInOutCubic(T t);
T easeInQuart(T t);     T easeOutQuart(T t);     T easeInOutQuart(T t);
T easeInSine(T t);      T easeOutSine(T t);      T easeInOutSine(T t);

// Special easing functions
T easeInBounce(T t);    T easeOutBounce(T t);    T easeInOutBounce(T t);
T easeInElastic(T t);   T easeOutElastic(T t);   T easeInOutElastic(T t);
T easeInBack(T t);      T easeOutBack(T t);      T easeInOutBack(T t);

// Utility functions
T inverseLerp(T a, T b, T value);    // Given value, find t
T remap(T value, T oldMin, T oldMax, T newMin, T newMax);
```

### 🔢 **Advanced Math Utilities** (Priority: Medium)
Useful for various mathematical operations and optimizations.
```cpp
// Fast math functions
T fastInverseSqrt(T value);          // Quake-style fast inverse square root
T fastSqrt(T value);                 // Fast square root approximation

// Number theory functions
unsigned int factorial(unsigned int n);
unsigned int gcd(unsigned int a, unsigned int b);
unsigned int lcm(unsigned int a, unsigned int b);
bool isPrime(unsigned int n);

// Bit manipulation utilities
unsigned int countBits(unsigned int value);
unsigned int reverseBits(unsigned int value);
unsigned int rotateLeft(unsigned int value, int shift);
unsigned int rotateRight(unsigned int value, int shift);
```

### 🎯 **2D Geometry Functions** (Priority: Medium)
Valuable for 2D collision detection, UI hit testing, and spatial queries.
```cpp
// Point-in-shape tests
bool pointInCircle(Vec2<T> point, Vec2<T> center, T radius);
bool pointInRect(Vec2<T> point, Vec2<T> min, Vec2<T> max);
bool pointInTriangle(Vec2<T> point, Vec2<T> a, Vec2<T> b, Vec2<T> c);
bool pointInPolygon(Vec2<T> point, const std::vector<Vec2<T>>& polygon);

// Line intersection
bool lineLineIntersection(Vec2<T> a1, Vec2<T> a2, Vec2<T> b1, Vec2<T> b2, Vec2<T>& intersection);
bool rayCircleIntersection(Vec2<T> origin, Vec2<T> direction, Vec2<T> center, T radius);

// Bounding box utilities
struct BoundingBox2D {
    Vec2<T> min, max;
    bool contains(Vec2<T> point);
    bool intersects(const BoundingBox2D& other);
    void expand(Vec2<T> point);
};
```

### 🔢 **Curve and Spline Functions** (Priority: Low)
Useful for smooth paths, camera movements, and procedural shapes.
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
Helpful for debugging, profiling, and data analysis.
```cpp
// Basic statistics
T mean(const std::vector<T>& values);
T median(std::vector<T> values);  // Note: modifies input for sorting
T standardDeviation(const std::vector<T>& values);
T variance(const std::vector<T>& values);

// Moving averages for smooth data
class MovingAverage {
    void addSample(T value);
    T getAverage() const;
    void setWindowSize(int size);
};
```

## 🧪 Testing

All math functions include comprehensive unit tests in `tests/math_utils_tests.cpp`. When adding new functionality:

1. Add the implementation to the appropriate `.ixx` file
2. Add corresponding unit tests with edge case coverage
3. Update this README with the new functions
4. Ensure all tests pass before committing

## 📝 Implementation Notes

- All functions are templated to support both `float` and `double` precision
- Functions are marked `constexpr` where possible for compile-time evaluation
- Consistent naming convention using camelCase
- Comprehensive edge case handling and input validation
- Performance-conscious implementations with fast math alternatives where appropriate

## 🎯 Next Steps

**Recommended implementation order:**
1. ~~**Color Math Functions**~~ ✅ **COMPLETED** - Essential for any visual editor
2. **Easing Functions** - Critical for smooth UI/UX
3. **Random Utilities** - Needed for procedural content and testing
4. **2D Geometry** - Important for spatial operations and collision detection
5. **Advanced Math Utilities** - Nice-to-have optimizations
6. **Statistics Functions** - Useful for debugging and profiling tools

---
*Last updated: August 27, 2025*
