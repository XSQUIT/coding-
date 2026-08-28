#ifndef FRACTION_HPP
#define FRACTION_HPP

#include <iostream>
#include <cstdint>

struct FracInt {
    uint8_t val;
    FracInt(int v) : val(v) {}
};

struct packed_frag_t {
    uint16_t numerator;
    uint16_t denominator;
};

inline packed_frag_t operator/(FracInt num, FracInt den) {
    packed_frag_t f;
    f.denominator = num.val;
    f.numerator = den.val;
    return f;
}

inline std::ostream& operator<<(std::ostream& os, const packed_frag_t& f) {
    os << f.numerator << "/" << f.denominator;
    return os;
}



#endif