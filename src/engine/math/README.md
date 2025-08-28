# Engine Math Library

## Implementation requirements
- Unit test all new implementation
- Ensure const-correctness
- After implementing new functionality, update this file accordingly

This directory contains the math modules f### `quat.ixx` - Quaternion Operations
**Status: ✅ Complete**
- Quaternion class with full arithmetic support
- Euler angle conversions
- Spherical linear interpolation (slerp)
- Rotation utilities and factory functions

## 🚀 Future Enhancement Roadmap

### 🎲 **Random Number Utilities** (Priority: High) ✅ **COMPLETED**
Essential for procedural generation, testing, and dynamic content.

**✅ Implemented Functions:**
- `Random` class with seeded generation using Mersenne Twister
- `seed(unsigned int)`, `random()`, `range(min, max)` for floats and integers
- `chance(probability)` - Random boolean with specified probability
- `unitCircle()`, `unitSphere()`, `insideSphere()`, `insideCube()` - Geometric sampling
- `choice(container)` - Random element selection from containers
- `shuffle(container)` - Fisher-Yates shuffle algorithm
- `perlinNoise(x, y)` - 2D Perlin noise implementation
- `fractalNoise(x, y, octaves)` - Multi-octave fractal noise
- `turbulence(x, y, octaves)` - Absolute value fractal noise for turbulence effects
- Global convenience functions: `random()`, `randomInt()`, `randomBool()`, etc.

**🧪 Test Coverage:** 3460 assertions across 5 test cases covering deterministic seeding, range validation, geometric distributions, container operations, and noise function properties.ding comprehensive mathematical utilities for game development and level editing.

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

### `random.ixx` - Random Number Utilities
**Status: ✅ Complete**
- Seeded random number generator class (Random)
- Random value generation (float ranges, integer ranges, boolean chance)
- Geometric random functions (unit circle, unit sphere, inside sphere, inside cube)
- Container utilities (random choice, Fisher-Yates shuffle)
- Noise functions (Perlin noise, fractal noise, turbulence)
- Global convenience functions for quick random access

### `animation.ixx` - Animation/Easing Functions
**Status: ✅ Complete**
- Complete easing function library (quadratic, cubic, quartic, sine)
- Special easing functions (bounce, elastic, back) with customizable parameters
- Utility functions (inverseLerp, remap)
- Type-safe EaseType enumeration and generic ease() dispatcher
- Constexpr optimization for compile-time evaluation

### `math.ixx` - Core Math Utilities (Updated)
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
- **NEW: Advanced math utilities** (fast approximations, number theory, bit manipulation)

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

### 🌊 **Easing/Animation Functions** (Priority: High) ✅ **COMPLETED**
Critical for smooth animations and user interface transitions.

**✅ Implemented Functions:**
- `easeInQuad`, `easeOutQuad`, `easeInOutQuad` - Quadratic easing
- `easeInCubic`, `easeOutCubic`, `easeInOutCubic` - Cubic easing  
- `easeInQuart`, `easeOutQuart`, `easeInOutQuart` - Quartic easing
- `easeInSine`, `easeOutSine`, `easeInOutSine` - Sine easing
- `easeInBounce`, `easeOutBounce`, `easeInOutBounce` - Bounce effects
- `easeInElastic`, `easeOutElastic`, `easeInOutElastic` - Elastic effects (with amplitude/period)
- `easeInBack`, `easeOutBack`, `easeInOutBack` - Back overshoot effects (with overshoot parameter)
- `inverseLerp(a, b, value)` - Find interpolation parameter for given value
- `remap(value, oldMin, oldMax, newMin, newMax)` - Remap value between ranges
- `ease(EaseType, t)` - Generic dispatcher with type-safe enumeration

**🧪 Test Coverage:** 214 assertions across 10 test cases covering boundary conditions, mathematical properties, parameter validation, and const-correctness.

### 🔢 **Advanced Math Utilities** (Priority: Medium) ✅ **COMPLETED**
Useful for various mathematical operations and optimizations.

**✅ Implemented Functions:**
- `fastInverseSqrt(value)` - Quake-style fast inverse square root approximation
- `fastSqrt(value)` - Fast square root approximation
- `factorial(n)` - Factorial calculation with overflow protection
- `gcd(a, b)` - Greatest Common Divisor using Euclidean algorithm
- `lcm(a, b)` - Least Common Multiple calculation
- `isPrime(n)` - Prime number test using trial division
- `countBits(value)` - Count set bits (population count)
- `reverseBits(value)` - Reverse all bits in a 32-bit integer
- `rotateLeft(value, shift)` - Rotate bits left with wrap-around
- `rotateRight(value, shift)` - Rotate bits right with wrap-around

**🧪 Test Coverage:** 287 assertions across 4 test cases covering fast approximations (1% accuracy), number theory properties, bit manipulation operations, and const-correctness.

### 🎯 **2D Geometry Functions** (Priority: Medium) ✅ **COMPLETED**
Valuable for 2D collision detection, UI hit testing, and spatial queries.

**✅ Implemented Functions:**
- `pointInCircle(point, center, radius)` - Fast circle containment test
- `pointInRect(point, min, max)` - Axis-aligned rectangle test  
- `pointInTriangle(point, a, b, c)` - Barycentric coordinate method
- `pointInPolygon(point, polygon)` - Ray casting algorithm for arbitrary polygons
- `lineLineIntersection(a1, a2, b1, b2, intersection)` - Parametric line intersection with result point
- `rayCircleIntersection(origin, direction, center, radius)` - Ray-sphere collision detection
- `segmentCircleIntersection(start, end, center, radius)` - Finite segment-circle intersection
- `distancePointToLine(point, lineStart, lineEnd)` - Perpendicular distance to infinite line
- `distancePointToSegment(point, segmentStart, segmentEnd)` - Distance to finite line segment
- `triangleArea(a, b, c)` - Cross product area calculation
- `polygonArea(polygon)` - Shoelace formula for arbitrary polygons
- `isPolygonConvex(polygon)` - Convexity test using cross products
- `BoundingBox2D` struct with contains, intersects, expand, center, size, area, and validity methods

**🧪 Test Coverage:** 115 assertions across 6 test cases covering all geometric functions, boundary conditions, mathematical properties, and const-correctness.

### 🎲 **3D Geometry Functions** (Priority: High)
Essential for 3D collision detection, ray casting, spatial queries, and level editor operations.
```cpp
// Point-in-shape tests for 3D
bool pointInSphere(Vec3<T> point, Vec3<T> center, T radius);
bool pointInAABB(Vec3<T> point, Vec3<T> min, Vec3<T> max);
bool pointInOBB(Vec3<T> point, const OrientedBoundingBox& obb);
bool pointInTetrahedron(Vec3<T> point, Vec3<T> a, Vec3<T> b, Vec3<T> c, Vec3<T> d);

// Ray-shape intersections (critical for 3D editors)
bool raySphereIntersection(Vec3<T> origin, Vec3<T> direction, Vec3<T> center, T radius, T& hitDistance);
bool rayAABBIntersection(Vec3<T> origin, Vec3<T> direction, Vec3<T> min, Vec3<T> max, T& hitDistance);
bool rayTriangleIntersection(Vec3<T> origin, Vec3<T> direction, Vec3<T> a, Vec3<T> b, Vec3<T> c, Vec3<T>& hitPoint);
bool rayPlaneIntersection(Vec3<T> origin, Vec3<T> direction, Vec3<T> planePoint, Vec3<T> planeNormal, T& hitDistance);

// Line and segment intersections
bool lineLineIntersection3D(Vec3<T> a1, Vec3<T> a2, Vec3<T> b1, Vec3<T> b2, Vec3<T>& closest1, Vec3<T>& closest2);
bool segmentSegmentIntersection(Vec3<T> a1, Vec3<T> a2, Vec3<T> b1, Vec3<T> b2, Vec3<T>& intersection);

// Distance calculations
T distancePointToPlane(Vec3<T> point, Vec3<T> planePoint, Vec3<T> planeNormal);
T distancePointToLine3D(Vec3<T> point, Vec3<T> linePoint, Vec3<T> lineDirection);
T distancePointToSegment3D(Vec3<T> point, Vec3<T> segmentStart, Vec3<T> segmentEnd);
T distanceLinesToLines(Vec3<T> a1, Vec3<T> a2, Vec3<T> b1, Vec3<T> b2);

// Bounding volume utilities
struct BoundingBox3D {
    Vec3<T> min, max;
    bool contains(Vec3<T> point);
    bool intersects(const BoundingBox3D& other);
    bool intersects(Vec3<T> sphereCenter, T radius);
    void expand(Vec3<T> point);
    void expand(const BoundingBox3D& other);
    Vec3<T> center() const;
    Vec3<T> size() const;
    T volume() const;
    Vec3<T> corner(int index) const;  // Get one of 8 corners
};

struct BoundingSphere {
    Vec3<T> center;
    T radius;
    bool contains(Vec3<T> point);
    bool intersects(const BoundingSphere& other);
    bool intersects(const BoundingBox3D& box);
    void expand(Vec3<T> point);
    void expand(const BoundingSphere& other);
};

// Oriented bounding box for rotated objects
struct OrientedBoundingBox {
    Vec3<T> center;
    Vec3<T> axes[3];      // Local coordinate system
    Vec3<T> extents;      // Half-sizes along each axis
    bool contains(Vec3<T> point);
    bool intersects(const OrientedBoundingBox& other);
    Vec3<T> corner(int index) const;
};

// Geometric calculations
T tetrahedronVolume(Vec3<T> a, Vec3<T> b, Vec3<T> c, Vec3<T> d);
T triangleArea3D(Vec3<T> a, Vec3<T> b, Vec3<T> c);
Vec3<T> triangleNormal(Vec3<T> a, Vec3<T> b, Vec3<T> c);
Vec3<T> barycentric3D(Vec3<T> point, Vec3<T> a, Vec3<T> b, Vec3<T> c);

// Plane utilities
struct Plane {
    Vec3<T> normal;
    T distance;
    Plane(Vec3<T> point, Vec3<T> normal);
    Plane(Vec3<T> a, Vec3<T> b, Vec3<T> c);  // From triangle
    T distanceToPoint(Vec3<T> point) const;
    Vec3<T> closestPoint(Vec3<T> point) const;
    bool isPointOnPlane(Vec3<T> point, T tolerance = 1e-6) const;
};

// Frustum culling for 3D rendering
struct Frustum {
    Plane planes[6];  // Left, Right, Top, Bottom, Near, Far
    bool contains(Vec3<T> point);
    bool intersects(const BoundingBox3D& box);
    bool intersects(const BoundingSphere& sphere);
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
2. ~~**Random Number Utilities**~~ ✅ **COMPLETED** - Essential for procedural generation, testing, and dynamic content
3. ~~**Easing Functions**~~ ✅ **COMPLETED** - Critical for smooth UI/UX
4. ~~**Advanced Math Utilities**~~ ✅ **COMPLETED** - Useful for mathematical operations and optimizations
5. ~~**2D Geometry**~~ ✅ **COMPLETED** - Important for spatial operations and collision detection
6. **3D Geometry** - **HIGH PRIORITY** - Essential for 3D level editor operations, ray casting, and collision detection
7. **Curve and Spline Functions** - Useful for smooth paths and procedural shapes
8. **Statistics Functions** - Useful for debugging and profiling tools

---
*Last updated: August 27, 2025*
