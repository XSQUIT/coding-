#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib.h"
#include <stdio.h>

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

// Standard Physics Object structure for your simulation loops
typedef struct {
    Rectangle rect;
    Vector2 velocity;
    Color color;
} PhysicsObject;

// Preprocessor macro for gravity constant
#define G 9.81f
#define SCALE 100.00f

// Function Declarations
bool CheckCollision(PhysicsObject a, PhysicsObject b);
void draw(int item_count, ...);

#endif
