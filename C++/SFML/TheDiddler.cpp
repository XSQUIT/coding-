#include<SFML/Graphics.hpp>
#include<cmath>
#include<vector>
#include<functional>
#include "../../vec.h"
#include "Fibonnaci.h"

Vec3 fieldFunc(int x, int y, int z)
{
    float fx = x - 10.f;
    float fy = y - 10.f;
    float fz = z * 0.1f;
    return Vec3(
        std::cos(fz) * fx - std::sin(fz) * fy,
        std::sin(fz) * fx + std::cos(fz) * fy,
        fz
    ).normalized();
}
//struct VectorField
//{
//    int width, height, depth;
//    std::vector<Vec3> data;
//
//    VectorField(int w, int h, int d) : width(w), height(h), depth(d), data(w * h * d) {}
//
//    Vec3& at(int x, int y, int z)
//    {
//        return data[x + y * width + z * width * height];
//    }
//
//    const Vec3& at(int x, int y, int z) const  
//        {
//            return data[x + y * width + z * width * height];
//        }
//
//    void fill(std::function<Vec3(int, int, int)> fn)
//    {
//        for (int z = 0; z < depth; z++)
//            for (int y = 0; y < height; y++)
//                for (int x = 0; x < width; x++)
//                    at(x, y, z) = fn(x, y, z); 
//    }
//};

void drawArrow(sf::RenderWindow& window, float x, float y, Vec3 v, float scale = 20.f)
{
    float ex = x + v.x * scale;
    float ey = y + v.y * scale;

    sf::Vertex line[2];
    line[0].position = sf::Vector2f(x, y);
    line[0].color = sf::Color::White;
    line[1].position = sf::Vector2f(ex, ey);
    line[1].color = sf::Color::White;
    window.draw(line, 2, sf::PrimitiveType::Lines);
    
    float angle = std::atan2(v.y, v.x);
    float headLen = 5.f;
    sf::Vertex head[3];
    head[0].position = sf::Vector2f(ex, ey);
    head[0].color = sf::Color::Yellow;
    head[1].position = sf::Vector2f(
        ex - headLen * std::cos(angle - 0.4f),
        ey - headLen * std::sin(angle - 0.4f));
    head[1].color = sf::Color::Yellow;
    head[2].position = sf::Vector2f(
        ex - headLen * std::cos(angle + 0.4f),
        ey - headLen * std::sin(angle + 0.4f));
    head[2].color = sf::Color::Yellow;
    window.draw(head, 3, sf::PrimitiveType::Triangles);
    
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({1000, 1000}), "Vector Field");

    //VectorField field(20, 20, 20);
    //field.fill([](int x, int y, int z) 
    //{
    //    return Vec3(-(float)y + 10, (float)x - 10, z * 0.1f).normalized();
    //});

    int slice = 0;
    float spacing = 50.f;
    sf::Vector2f center(500.f, 500.f);

    auto spiralPoints = fibonnaciSpiral(20);

    while (window.isOpen())
    {
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Up) slice++; //= (slice + 1) % field.depth;
                if (key->code == sf::Keyboard::Key::Down) slice--; // = (slice - 1 + field.depth) % field.depth;
            }
        }

        window.clear();

        for (int y = 0; y < 20 /*field.depth*/; y++)
            for (int x = 0; x < 20/*field.width*/; x++)
            {
                Vec3 v = /*field.at*/fieldFunc(x, y, slice);
                drawArrow(window, x * spacing + 25, y * spacing + 25, v);
            }
        for (const auto& p : spiralPoints)
        {
            sf::CircleShape dot(3.f);
            dot.setFillColor(sf::Color::Cyan);
            dot.setPosition(center + p);
            window.draw(dot);
        }
        window.display();
    }
}