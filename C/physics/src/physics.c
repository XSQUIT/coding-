#include <physics.h>

bool CheckCollision(PhysicsObject a, PhysicsObject b)
{
    return CheckCollisionRecs(a.rect, b.rect);
}