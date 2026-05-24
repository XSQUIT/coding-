#include <SFML/Graphics.hpp>
#include "graph.h"
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode({1000, 700}), "Graph");
    window.setFramerateLimit(60);

    // Graph fills the whole window; you could pass a sub-rect for a panel layout
    Graph graph(window, { {0,0}, {1000,700} });

    // ── add series ──
    graph.addSeries(FuncSeries{
        [](double x){ return std::sin(x); },
        sf::Color(100, 200, 255), "sin(x)"
    });
    graph.addSeries(FuncSeries{
        [](double x){ return x != 0 ? std::sin(x)/x : 1.0; },
        sf::Color(255, 160, 80), "sinc(x)"
    });
    graph.addSeries(ScatterSeries{
        { {-3,2},{-1,-1},{0,0.5},{2,3},{4,-2} },
        sf::Color(100, 255, 140), "data"
    });
    graph.addSeries(VectorFieldSeries{
        [](Vec2 p){ return Vec2{ -p.y, p.x }; },   // rotation field
        sf::Color(180, 100, 255), "rot field"
    });

    while (window.isOpen()) {
        while (const auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            graph.handleEvent(*ev);
        }
        window.clear(sf::Color(18, 18, 24));
        graph.draw();
        window.display();
    }
}
