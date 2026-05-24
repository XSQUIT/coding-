#pragma once
#include <iostream>
#include <cmath>
#include <vector>
#include <SFML/Graphics.hpp>

std::vector<sf::Vector2f> fibonnaciSpiral(int steps, float a = 5.f, float b = 0.3f)
{
    std::vector<sf::Vector2f> points;
    unsigned long long thetaPrevPrev = 0;
    unsigned long long thetaPrev = 0;
    unsigned long long theta = 1;

    for (int i = 0; i < steps; i++)
    {
        thetaPrevPrev = thetaPrev;
        thetaPrev = theta;
        theta = thetaPrev + thetaPrevPrev;

        float r = a * std::pow((float)M_E, b*theta);

        int x = r * cos(theta);
        int y = r * sin(theta);

        points.push_back(sf::Vector2f(x, y));
    }
    return points;
}