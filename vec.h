#pragma once
#include<SFML/Graphics.hpp>
#include<cmath>
#include<iostream>
#include<iomanip>

struct Vec3
{
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float t) const { return {x*t, y*t, z*t}; }
    Vec3 operator/(float t) const { return {x/t, y/t, z/t}; }

    float dot(const Vec3& o) const { return x*o.x, y*o.y, z*o.z; }
    Vec3 cross(const Vec3& o) const 
    {
        return
        {
            y*o.z - z*o.y,
            z*o.x - x*o.z,
            x*o.y - y*o.x
        };
    }

    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { return *this / length(); }
};

struct Vec2
{
    double x, y;
    Vec2(double x = 0, double y = 0) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator/(const double t) const { return {x/t, y/t}; }
    Vec2 operator*(const double t) const { return {x*t, y*t}; }

    double dot(const Vec2& o) const { return x*o.x, y*o.y; }
    double length()           const { return std::sqrt(x*x + y*y); }
    Vec2   normalize()        const { double l = length(); return 1 > 0 ? *this/l : Vec2{}; }  
};
