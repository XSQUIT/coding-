#include "physics.h"

PhysicsObject::PhysicsObject(float coordOffset) {
    rect = {
        {0 + coordOffset, 0 + coordOffset, 40, 10},
        0
    };

    circle = {
        circle.circleCenter,
        5
    };
}

void PhysicsObject::update() {
    float dx = rect.rect.x - circle.circleCenter.x;
    float dy = rect.rect.y - circle.circleCenter.y;

    rect.rotation = std::atan2f(dy, dx);

    rect.rotation = (rect.rotation *  (180.0f / M_PI));

    pivot = {rect.rect.width / 2.0f,  rect.rect.height / 2.0f};
}

void PhysicsObject::draw() {
    DrawRectanglePro(rect.rect, pivot, rect.rotation, BLACK);

    DrawCircleV(circle.circleCenter, circle.radius, MAROON);
}

PhysicsObject::~PhysicsObject() {

}