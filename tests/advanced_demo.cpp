#include <iostream>
#include <iomanip>
import engine.math;

int main()
{
	std::cout << std::fixed << std::setprecision( 6 );

	std::cout << "Advanced Math Utilities Demo\n";
	std::cout << "============================\n\n";

	// Fast approximations
	std::cout << "Fast Math Functions:\n";
	std::cout << "fastSqrt(16.0) = " << math::fastSqrt( 16.0f ) << " (standard: " << math::sqrt( 16.0f ) << ")\n";
	std::cout << "fastInverseSqrt(4.0) = " << math::fastInverseSqrt( 4.0f ) << " (standard: " << ( 1.0f / math::sqrt( 4.0f ) ) << ")\n\n";

	// Number theory
	std::cout << "Number Theory Functions:\n";
	std::cout << "factorial(5) = " << math::factorial( 5u ) << "\n";
	std::cout << "gcd(48, 18) = " << math::gcd( 48u, 18u ) << "\n";
	std::cout << "lcm(12, 8) = " << math::lcm( 12u, 8u ) << "\n";
	std::cout << "isPrime(97) = " << ( math::isPrime( 97u ) ? "true" : "false" ) << "\n";
	std::cout << "isPrime(98) = " << ( math::isPrime( 98u ) ? "true" : "false" ) << "\n\n";

	// Bit manipulation
	std::cout << "Bit Manipulation Functions:\n";
	const unsigned int testValue = 0x12345678u;
	std::cout << std::hex << std::uppercase;
	std::cout << "Original value: 0x" << testValue << "\n";
	std::cout << "countBits(0x12345678) = " << std::dec << math::countBits( testValue ) << "\n";
	std::cout << "reverseBits(0x12345678) = 0x" << std::hex << math::reverseBits( testValue ) << "\n";
	std::cout << "rotateLeft(0x12345678, 4) = 0x" << math::rotateLeft( testValue, 4 ) << "\n";
	std::cout << "rotateRight(0x12345678, 4) = 0x" << math::rotateRight( testValue, 4 ) << "\n";

	return 0;
}
