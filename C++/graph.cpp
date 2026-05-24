#include "graph.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

// ── helpers ──────────────────────────────────────────────────────────────────

static std::string fmt(double v, int prec = 3) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

static sf::Vector2f toSF(Vec2 v) { return {(float)v.x, (float)v.y}; }

// ── ctor ─────────────────────────────────────────────────────────────────────

Graph::Graph(sf::RenderWindow& window, sf::FloatRect viewport)
    : m_win(window), m_viewport(viewport)
{
    // Try to load a font; label drawing is skipped gracefully if unavailable
    m_fontLoaded = m_font.openFromFile("assets/font.ttf");

    m_view.setViewport({
        viewport.position.x / window.getSize().x,
        viewport.position.y / window.getSize().y,
        viewport.size.x     / window.getSize().x,
        viewport.size.y     / window.getSize().y
    });
    m_view.setSize({ viewport.size.x, viewport.size.y });
    m_view.setCenter({ viewport.size.x / 2.f, viewport.size.y / 2.f });
}

// ── public ───────────────────────────────────────────────────────────────────

void Graph::addSeries(Series s)   { m_series.push_back(std::move(s)); }
void Graph::clearSeries()         { m_series.clear(); m_tooltip.reset(); }

// ── coordinate transforms ─────────────────────────────────────────────────

sf::Vector2f Graph::worldToScreen(Vec2 w) const {
    double tx = (w.x - m_xMin) / (m_xMax - m_xMin);
    double ty = (w.y - m_yMin) / (m_yMax - m_yMin);
    return {
        (float)(tx * m_viewport.size.x),
        (float)((1.0 - ty) * m_viewport.size.y)   // y flipped
    };
}

Vec2 Graph::screenToWorld(sf::Vector2f s) const {
    double tx = s.x / m_viewport.size.x;
    double ty = s.y / m_viewport.size.y;
    return {
        m_xMin + tx * (m_xMax - m_xMin),
        m_yMin + (1.0 - ty) * (m_yMax - m_yMin)
    };
}

// ── events ────────────────────────────────────────────────────────────────

void Graph::handleEvent(const sf::Event& event) {

    // Helper: map raw window coords → viewport-local coords
    auto toLocal = [&](sf::Vector2i winPos) -> sf::Vector2f {
        return { winPos.x - m_viewport.position.x,
                 winPos.y - m_viewport.position.y };
    };
    auto inViewport = [&](sf::Vector2i p) {
        return p.x >= m_viewport.position.x &&
               p.x <= m_viewport.position.x + m_viewport.size.x &&
               p.y >= m_viewport.position.y &&
               p.y <= m_viewport.position.y + m_viewport.size.y;
    };

    // ── zoom ──
    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (!inViewport({scroll->position.x, scroll->position.y})) return;
        sf::Vector2f local = toLocal({scroll->position.x, scroll->position.y});
        Vec2 pivot = screenToWorld(local);

        double factor = (scroll->delta > 0) ? 0.85 : 1.0 / 0.85;
        double hw = (m_xMax - m_xMin) * 0.5 * factor;
        double hh = (m_yMax - m_yMin) * 0.5 * factor;
        // zoom anchored to pivot
        double rx = (pivot.x - m_xMin) / (m_xMax - m_xMin);
        double ry = (pivot.y - m_yMin) / (m_yMax - m_yMin);
        double cx = pivot.x - (2*rx - 1) * hw;
        double cy = pivot.y - (2*ry - 1) * hh;
        m_xMin = cx - hw; m_xMax = cx + hw;
        m_yMin = cy - hh; m_yMax = cy + hh;
    }

    // ── pan start ──
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left &&
            inViewport({mb->position.x, mb->position.y}))
        {
            m_panning  = true;
            m_panStart = toLocal({mb->position.x, mb->position.y});
            m_panX0    = m_xMin;
            m_panY0    = m_yMin;
            m_tooltip.reset();
        }
        // right-click inspect
        if (mb->button == sf::Mouse::Button::Right &&
            inViewport({mb->position.x, mb->position.y}))
        {
            Vec2 w = screenToWorld(toLocal({mb->position.x, mb->position.y}));
            m_tooltip = nearestPoint(w);
        }
    }

    // ── pan drag ──
    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (m_panning) {
            sf::Vector2f local = toLocal({mm->position.x, mm->position.y});
            double dx = (local.x - m_panStart.x) / m_viewport.size.x * (m_xMax - m_xMin);
            double dy = (local.y - m_panStart.y) / m_viewport.size.y * (m_yMax - m_yMin);
            double w  = m_xMax - m_xMin, h = m_yMax - m_yMin;
            m_xMin = m_panX0 - dx;   m_xMax = m_xMin + w;
            m_yMin = m_panY0 + dy;   m_yMax = m_yMin + h;
        }
    }

    // ── pan end ──
    if (const auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mb->button == sf::Mouse::Button::Left)
            m_panning = false;
    }
}

// ── draw ─────────────────────────────────────────────────────────────────────

void Graph::draw() {
    m_win.setView(m_view);
    drawGrid();
    drawAxes();
    drawSeries();
    drawTooltip();
    m_win.setView(m_win.getDefaultView());  // restore
}

// ── grid ─────────────────────────────────────────────────────────────────────

double Graph::niceStep(double range, int targetLines) const {
    double rough = range / targetLines;
    double mag   = std::pow(10.0, std::floor(std::log10(rough)));
    double norm  = rough / mag;
    double nice  = (norm < 1.5) ? 1 : (norm < 3.5) ? 2 : (norm < 7.5) ? 5 : 10;
    return nice * mag;
}

void Graph::drawGrid() {
    float W = m_viewport.size.x, H = m_viewport.size.y;
    double xStep = niceStep(m_xMax - m_xMin, 10);
    double yStep = niceStep(m_yMax - m_yMin, 8);

    sf::Color gridColor(50, 50, 60);

    // vertical lines
    double xStart = std::ceil(m_xMin / xStep) * xStep;
    for (double x = xStart; x <= m_xMax; x += xStep) {
        auto s = worldToScreen({x, 0});
        sf::Vertex line[2] = {
            { {s.x, 0},  gridColor },
            { {s.x, H},  gridColor }
        };
        m_win.draw(line, 2, sf::PrimitiveType::Lines);

        if (m_fontLoaded) {
            sf::Text lbl(m_font, fmt(x, 2), 10);
            lbl.setFillColor(sf::Color(120,120,140));
            lbl.setPosition({s.x + 2, H - 14});
            m_win.draw(lbl);
        }
    }

    // horizontal lines
    double yStart = std::ceil(m_yMin / yStep) * yStep;
    for (double y = yStart; y <= m_yMax; y += yStep) {
        auto s = worldToScreen({0, y});
        sf::Vertex line[2] = {
            { {0,  s.y},  gridColor },
            { {W,  s.y},  gridColor }
        };
        m_win.draw(line, 2, sf::PrimitiveType::Lines);

        if (m_fontLoaded) {
            sf::Text lbl(m_font, fmt(y, 2), 10);
            lbl.setFillColor(sf::Color(120,120,140));
            lbl.setPosition({2, s.y - 12});
            m_win.draw(lbl);
        }
    }
}

// ── axes ─────────────────────────────────────────────────────────────────────

void Graph::drawAxes() {
    float W = m_viewport.size.x, H = m_viewport.size.y;
    sf::Color axisColor(200, 200, 210);

    // x-axis (y=0)
    if (m_yMin <= 0 && m_yMax >= 0) {
        auto s = worldToScreen({0, 0});
        sf::Vertex line[2] = { {{0, s.y}, axisColor}, {{W, s.y}, axisColor} };
        m_win.draw(line, 2, sf::PrimitiveType::Lines);
    }
    // y-axis (x=0)
    if (m_xMin <= 0 && m_xMax >= 0) {
        auto s = worldToScreen({0, 0});
        sf::Vertex line[2] = { {{s.x, 0}, axisColor}, {{s.x, H}, axisColor} };
        m_win.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

// ── series dispatch ───────────────────────────────────────────────────────────

void Graph::drawSeries() {
    for (const auto& s : m_series) {
        std::visit([&](const auto& v){ 
            using T = std::decay_t<decltype(v)>;
            if      constexpr (std::is_same_v<T, FuncSeries>)        drawFuncSeries(v);
            else if constexpr (std::is_same_v<T, ScatterSeries>)     drawScatterSeries(v);
            else if constexpr (std::is_same_v<T, VectorFieldSeries>) drawVectorField(v);
        }, s);
    }
}

// ── func series ───────────────────────────────────────────────────────────────

void Graph::drawFuncSeries(const FuncSeries& s) {
    std::vector<sf::Vertex> verts;
    verts.reserve(s.samples);
    double step = (m_xMax - m_xMin) / s.samples;
    for (int i = 0; i <= s.samples; ++i) {
        double x = m_xMin + i * step;
        double y = s.fn(x);
        if (!std::isfinite(y)) { 
            // break the polyline on discontinuities
            if (verts.size() > 1)
                m_win.draw(verts.data(), verts.size(), sf::PrimitiveType::LineStrip);
            verts.clear();
            continue;
        }
        verts.push_back({ worldToScreen({x, y}), s.color });
    }
    if (verts.size() > 1)
        m_win.draw(verts.data(), verts.size(), sf::PrimitiveType::LineStrip);
}

// ── scatter series ────────────────────────────────────────────────────────────

void Graph::drawScatterSeries(const ScatterSeries& s) {
    for (const auto& p : s.points) {
        auto sc = worldToScreen(p);
        sf::CircleShape c(s.pointRadius);
        c.setFillColor(s.color);
        c.setOrigin({s.pointRadius, s.pointRadius});
        c.setPosition(sc);
        m_win.draw(c);
    }
}

// ── vector field ──────────────────────────────────────────────────────────────

void Graph::drawVectorField(const VectorFieldSeries& s) {
    double xStep = (m_xMax - m_xMin) / s.gridW;
    double yStep = (m_yMax - m_yMin) / s.gridH;
    for (int i = 0; i <= s.gridW; ++i) {
        for (int j = 0; j <= s.gridH; ++j) {
            Vec2 origin{ m_xMin + i * xStep, m_yMin + j * yStep };
            Vec2 dir = s.fn(origin);
            // normalize then scale by arrowScale
            double len = dir.length();
            if (len < 1e-12) continue;
            Vec2 tip = origin + dir.normalize() * s.arrowScale * (m_xMax - m_xMin);
            drawArrow(origin, tip, s.color);
        }
    }
}

void Graph::drawArrow(Vec2 from, Vec2 to, sf::Color color) {
    auto sf_  = worldToScreen(from);
    auto st   = worldToScreen(to);

    // shaft
    sf::Vertex shaft[2] = { {sf_, color}, {st, color} };
    m_win.draw(shaft, 2, sf::PrimitiveType::Lines);

    // arrowhead: two short lines at ±30°
    float dx = st.x - sf_.x, dy = st.y - sf_.y;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 1.f) return;
    float ux = dx/len, uy = dy/len;
    float hw = std::min(len * 0.35f, 8.f);  // head size capped at 8px
    float cos30 = 0.866f, sin30 = 0.5f;
    sf::Vector2f h1 = { st.x - hw*(ux*cos30 - uy*sin30),
                         st.y - hw*(uy*cos30 + ux*sin30) };
    sf::Vector2f h2 = { st.x - hw*(ux*cos30 + uy*sin30),
                         st.y - hw*(uy*cos30 - ux*sin30) };
    sf::Vertex head1[2] = { {st, color}, {h1, color} };
    sf::Vertex head2[2] = { {st, color}, {h2, color} };
    m_win.draw(head1, 2, sf::PrimitiveType::Lines);
    m_win.draw(head2, 2, sf::PrimitiveType::Lines);
}

// ── inspect / tooltip ─────────────────────────────────────────────────────────

std::optional<std::pair<Vec2, std::string>>
Graph::nearestPoint(Vec2 worldMouse) const {
    double bestDist = std::numeric_limits<double>::max();
    std::optional<std::pair<Vec2, std::string>> result;

    for (const auto& s : m_series) {
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ScatterSeries>) {
                for (const auto& p : v.points) {
                    Vec2 d = p - worldMouse;
                    double dist = d.length();
                    if (dist < bestDist) {
                        bestDist = dist;
                        result = { p, v.label + " (" + fmt(p.x) + ", " + fmt(p.y) + ")" };
                    }
                }
            } else if constexpr (std::is_same_v<T, FuncSeries>) {
                // snap to curve sample nearest in x
                double y = v.fn(worldMouse.x);
                if (std::isfinite(y)) {
                    Vec2 p{worldMouse.x, y};
                    Vec2 d = p - worldMouse;
                    double dist = d.length();
                    if (dist < bestDist) {
                        bestDist = dist;
                        result = { p, v.label + " (" + fmt(p.x) + ", " + fmt(p.y) + ")" };
                    }
                }
            }
        }, s);
    }
    return result;
}

void Graph::drawTooltip() {
    if (!m_tooltip || !m_fontLoaded) return;
    auto [pt, label] = *m_tooltip;
    auto sc = worldToScreen(pt);

    // dot
    sf::CircleShape dot(5.f);
    dot.setFillColor(sf::Color::Yellow);
    dot.setOrigin({5.f, 5.f});
    dot.setPosition(sc);
    m_win.draw(dot);

    // box + text
    sf::Text txt(m_font, label, 12);
    txt.setFillColor(sf::Color::White);
    auto bounds = txt.getLocalBounds();
    sf::RectangleShape box({ bounds.size.x + 10, bounds.size.y + 8 });
    box.setFillColor(sf::Color(30,30,40,210));
    box.setOutlineColor(sf::Color(180,180,200));
    box.setOutlineThickness(1);
    box.setPosition({sc.x + 8, sc.y - 28});
    txt.setPosition({sc.x + 13, sc.y - 25});
    m_win.draw(box);
    m_win.draw(txt);
}
