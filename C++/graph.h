#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <variant>
#include <string>
#include <optional>
#include <cmath>
#include "../vec.h"

struct FuncSeries {
  std::function<double(double)> fn;
  sf::Color color;
  std::string label;
  int samples = 800;
};

struct ScatterSeries {
  std::vector<Vec2> points;
  sf::Color color;
  std::string label;
  float pointradius = 4.f;
};


struct VectorFieldSeries {
    std::function<Vec2(Vec2)> fn;
    sf::Color color;
    std::string label;
    int gridW = 20, gridH = 20;   // arrows per axis
    double arrowScale = 0.04;     // world-space max arrow length
};

using Series = std::variant<FuncSeries, ScatterSeries, VectorFieldSeries>;

// ── Graph ───────────────────────────────────────────────────────────────────

class Graph {
public:
    Graph(sf::RenderWindow& window, sf::FloatRect viewport);
    // viewport = pixel rect this graph occupies in the window

    void addSeries(Series s);
    void clearSeries();

    // Call these from your event loop
    void handleEvent(const sf::Event& event);

    // Call every frame
    void draw();

private:
    // ── coordinate transforms ──
    sf::Vector2f worldToScreen(Vec2 w) const;
    Vec2         screenToWorld(sf::Vector2f s) const;

    // ── sub-draw routines ──
    void drawGrid();
    void drawAxes();
    void drawSeries();
    void drawFuncSeries   (const FuncSeries&        s);
    void drawScatterSeries(const ScatterSeries&      s);
    void drawVectorField  (const VectorFieldSeries&  s);
    void drawTooltip();
    void drawArrow(Vec2 from, Vec2 to, sf::Color color);

    // ── grid helpers ──
    double niceStep(double range, int targetLines) const;

    // ── inspect ──
    std::optional<std::pair<Vec2, std::string>> nearestPoint(Vec2 worldMouse) const;

    // ── state ──
    sf::RenderWindow& m_win;
    sf::FloatRect     m_viewport;   // pixel rect
    sf::View          m_view;       // SFML view matching viewport

    // world-space visible range
    double m_xMin = -10, m_xMax = 10;
    double m_yMin = -6,  m_yMax = 6;

    std::vector<Series> m_series;

    // pan state
    bool            m_panning = false;
    sf::Vector2f    m_panStart{};
    double          m_panX0{}, m_panY0{};  // world coords at pan start

    // inspect state
    std::optional<std::pair<Vec2, std::string>> m_tooltip;

    sf::Font m_font;
    bool     m_fontLoaded = false;
};
