#include <iostream>
#include <climits> // Contains INT_MAX
#include "/home/quinn/coding/intDivision.hpp"

int main() {
    // 1. Create a fraction using massive numbers near the integer limit
    // By wrapping the first number in FracInt(), C++ intercept the '/' sign
    FRACTION_HPP::packed_frag_t huge_fraction = FracInt(2147483640) / 2147483647;

    // 2. You can print it directly to std::cout now!
    std::cout << "The fraction is: " << huge_fraction << std::endl;

    // 3. Proof that they are stored as separate numbers within a single 64-bit structure
    std::cout << "Extracted Numerator:   " << huge_fraction.numerator << std::endl;
    std::cout << "Extracted Denominator: " << huge_fraction.denominator << std::endl;

    // Treat the struct as a raw 64-bit unsiged int to look at its combined hex bits
    uint64_t raw_bits = *reinterpret_cast<uint64_t*>(&huge_fraction);
    std::printf("Raw combined 64-bit Hex: 0x%016llX\n", (unsigned long long)raw_bits);

    return 0;
}
