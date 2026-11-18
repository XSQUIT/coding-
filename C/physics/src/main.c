#include "physics.h"

int main(void)
{
    InitWindow(600, 400, "Physics");
    SetTargetFPS(120);

    PhysicsObject ball = {
        {200, 0, 50, 50}, 
        {0, 0}, 
        BLUE
    };
    PhysicsObject floor = {
        {0, 300, 600, 400},
        {0, 0},
        GRAY
    };

    PhysicsObject wall = {
        {500, 0, 10, 400},
        {0, 0},
        GRAY
    };

    PhysicsObject wallL = {
        {100, 0, 10, 400},
        {0, 0},
        GRAY
    };

    const float defaultWaitTimer = .0f;
    float waitTimer = defaultWaitTimer;
    bool isWaiting = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (!isWaiting)
        {
            if (!CheckCollision(ball, floor))
            {
                ball.color = BLUE;
                ball.velocity.y += ((G * SCALE) * dt);
                ball.rect.y += (ball.velocity.y * dt);
            }
            else
            {
                ball.color = RED;
                ball.rect.y = floor.rect.y - ball.rect.height;
                ball.velocity.y = -ball.velocity.y * 0.90f;
                isWaiting = true;
            }

            if (!CheckCollision(ball, wall))
            {
                ball.color = BLUE;
                ball.velocity.x += (G * dt);
                ball.rect.x += (ball.velocity.x * dt);
            }
            else
            {
                ball.color = RED;
                ball.rect.x = wall.rect.x - ball.rect.width;
                ball.velocity.x = -ball.velocity.x * 0.90f;
                isWaiting = true;
            }

            if (!CheckCollision(ball, wallL))
            {
                ball.color = BLUE;
                ball.velocity.x += (G * dt);
                ball.rect.x += (ball.velocity.x * dt);
            }
            else
            {
                ball.color = RED;
                ball.rect.x = wallL.rect.x - ball.rect.width;
                ball.velocity.x = -ball.velocity.x * 0.90f;
                isWaiting = true;
            }
        }
        else
        {
            waitTimer -= dt;

            if (waitTimer <= 0)
            {
                isWaiting = false;
                waitTimer = defaultWaitTimer;
            }
        }

        RenderItem renderBall = {
            .type = SHAPE_RECTANGLE,
            .color = ball.color,
            .isWireframe = false,
            .geometry.rect = ball.rect
        };

        RenderItem renderFloor = {
            .type = SHAPE_RECTANGLE,
            .color = floor.color,
            .isWireframe = false,
            .geometry.rect = floor.rect
        };

        RenderItem renderWall = {
            .type = SHAPE_RECTANGLE,
            .color = wall.color,
            .isWireframe = false,
            .geometry.rect = wall.rect
        };

        RenderItem renderWallL = {
            .type = SHAPE_RECTANGLE,
            .color = wallL.color,
            .isWireframe = false,
            .geometry.rect = wallL.rect
        };

        draw(4, renderBall, renderFloor, renderWall, renderWallL);
    }
    CloseWindow();
    return 0;
}