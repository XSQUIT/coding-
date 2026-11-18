#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include "raygui.h"
#include <stdlib.h>
#include <stdio.h>
#include <cmath>

// Define constant values
const int screenWidth = 600;
const int screenHeight = 400;
const int targetFPS = 60;
#define G 9.81f
#define SCALE 100.00f

// Core shape identifiers for physics and rendering abstraction
typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_LINE
} ShapeType;

// Unified render command package
typedef struct {
    ShapeType type;
    Color color;
    bool isWireframe;
    float thickness;
    union {
        Rectangle rect;
        struct { Vector2 center; float radius; } circle;
        struct { Vector2 start; Vector2 end; } line;
    } geometry;
} RenderItem;

typedef struct {
    Rectangle rect;
    int rotation;
} RectangleObject;

typedef struct {
    Vector2 circleCenter;
    float radius;
} CircleObject;


// Function Declarations
class PhysicsObject {
    public:
    PhysicsObject(float coordOffset);
    void draw();
    void update();
    ~PhysicsObject();

    void setTargetPosition(float x, float y) {
        circle.circleCenter.x = x;
        circle.circleCenter.y = y;
    }

    // ADD THIS: Allows main.cpp to check if the mouse clicked the circle
    Vector2 getTargetPosition() const { return circle.circleCenter; }
    float getTargetRadius() const { return circle.radius; }

    private:
    RectangleObject rect;
    CircleObject circle;
    Vector2 pivot;
};

#endif
