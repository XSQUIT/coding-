#include "physics.h"

void draw(int item_count, ...) {
    BeginDrawing();
        ClearBackground(RAYWHITE);

        va_list args;
        va_start(args, item_count);

        for (int i = 0; i < item_count; i++) {
            RenderItem item = va_arg(args, RenderItem);

            switch (item.type) {
                case SHAPE_RECTANGLE:
                    if (item.isWireframe) {
                        DrawRectangleLinesEx(item.geometry.rect, item.thickness, item.color);
                    } else {
                        DrawRectangleRec(item.geometry.rect, item.color);
                    }
                    break;
                    
                case SHAPE_CIRCLE:
                    if (item.isWireframe) {
                        DrawCircleLines(
                            (int)item.geometry.circle.center.x, 
                            (int)item.geometry.circle.center.y, 
                            item.geometry.circle.radius, 
                            item.color
                        );
                    } else {
                        DrawCircleV(item.geometry.circle.center, item.geometry.circle.radius, item.color);
                    }
                    break;

                case SHAPE_LINE:
                    DrawLineEx(item.geometry.line.start, item.geometry.line.end, item.thickness, item.color);
                    break;
            }
        }

        va_end(args);
    EndDrawing();
}