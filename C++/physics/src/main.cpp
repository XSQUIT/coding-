#include "physics.h"

// 1. Force the compiler to build the UI binary code here
#ifndef RAYGUI_IMPLEMENTATION
    #define RAYGUI_IMPLEMENTATION
#endif

// 2. Wrap it inside extern "C" so the C++ compiler links the function symbols correctly
extern "C" {
    #include "raygui.h"
}

int main() {
    InitWindow(screenWidth, screenHeight, "Physics Sim");

    SetTargetFPS(targetFPS);

    // PhysicsObject is the CLASS (the blueprint)
    // myObject is the INSTANCE (the actual item in memory)
    PhysicsObject arrow(200); 


    // UI elements generated with AI to save time and a headache
    // 1. Raygui uses plain character arrays for strings.
    // We allocate enough space for our numeric strings.
    char inputXStr[16] = "400.0";
    char inputYStr[16] = "225.0";

    // Variables to toggle focus states (managed entirely by raygui)
    bool editModeX = false;
    bool editModeY = false;

    bool isDragging = false;

    while (!WindowShouldClose())
    {
        Vector2 mousePos = GetMousePosition();
        Vector2 currentCirclePos = arrow.getTargetPosition();
        // Give a generous +5.0f grab buffer so it's easy to click
        float grabRadius = arrow.getTargetRadius() + 5.0f; 

        // --- 1. HANDLE MOUSE DRAGGING ---
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointCircle(mousePos, currentCirclePos, grabRadius)) {
                isDragging = true;
                // Unfocus textboxes if the user grabs the circle
                editModeX = false; 
                editModeY = false;
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            isDragging = false;
        }

        // --- 2. SYNC POSITION (DRAG VS. TEXTBOX) ---
        if (isDragging) {
            // Move circle directly to mouse
            arrow.setTargetPosition(mousePos.x, mousePos.y);
            
            // Update the raygui text arrays so the UI reflects the drag
            snprintf(inputXStr, sizeof(inputXStr), "%.1f", mousePos.x);
            snprintf(inputYStr, sizeof(inputYStr), "%.1f", mousePos.y);
        } else {
            // Otherwise, let the textboxes drive the position
            arrow.setTargetPosition((float)atof(inputXStr), (float)atof(inputYStr));
        }

        // 1. --- EXTRACT UI STRINGS & UPDATE PHYSICS STATE ---
        float targetX = (float)atof(inputXStr);
        float targetY = (float)atof(inputYStr);
        
        // Pass the GUI values straight into your private class structure
        arrow.setTargetPosition(targetX, targetY);

        // 2. --- RUN YOUR NATIVE PHYSICS ENGINE UPDATE ---
        arrow.update();

        BeginDrawing();
            ClearBackground(RAYWHITE);

            arrow.draw();

            // --- DELEGATE UI TO RAYGUI ---
            // Draw X Coordinate Input
            DrawText("Center X:", 40, 55, 20, DARKGRAY);
            if (GuiTextBox((Rectangle){ 150, 50, 120, 35 }, inputXStr, 16, editModeX)) {
                editModeX = !editModeX; // Toggles typing focus when clicked or Enter is pressed
            }

            // Draw Y Coordinate Input
            DrawText("Center Y:", 40, 105, 20, DARKGRAY);
            if (GuiTextBox((Rectangle){ 150, 100, 120, 35 }, inputYStr, 16, editModeY)) {
                editModeY = !editModeY;
            }

            // --- SYNC UI STRINGS TO PHYSICS ENGINE ---
            // Convert the string array to float values and pass them through your setter
            arrow.setTargetPosition((float)atof(inputXStr), (float)atof(inputYStr));

        EndDrawing();
    }
    
    
}