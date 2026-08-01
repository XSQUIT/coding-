#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "game.h"

/* Carve a rectangular room of floor tiles into the map. */
static void carve_room(Map *map, Room r) {
    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            map->tiles[y][x].type = TILE_FLOOR;
        }
    }
}

/* Carve an L-shaped corridor between two points, horizontal then vertical. */
static void carve_corridor(Map *map, int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;
    while (x != x2) {
        map->tiles[y][x].type = TILE_FLOOR;
        x += (x2 > x) ? 1 : -1;
    }
    while (y != y2) {
        map->tiles[y][x].type = TILE_FLOOR;
        y += (y2 > y) ? 1 : -1;
    }
    map->tiles[y][x].type = TILE_FLOOR;
}

static int rooms_overlap(Room a, Room b) {
    /* Pad by 1 so rooms don't end up sharing a wall with no gap. */
    return (a.x - 1 < b.x + b.w && a.x + a.w + 1 > b.x &&
            a.y - 1 < b.y + b.h && a.y + a.h + 1 > b.y);
}

void map_generate(Map *map, int depth) {
    (void)depth; /* room count/size currently fixed; depth affects monsters/items instead */

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map->tiles[y][x].type = TILE_WALL;
            map->tiles[y][x].visible = 0;
            map->tiles[y][x].discovered = 0;
        }
    }

    map->room_count = 0;
    int attempts = 0;
    while (map->room_count < MAX_ROOMS && attempts < 300) {
        attempts++;
        int w = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        int h = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        int x = 1 + rand() % (MAP_WIDTH - w - 2);
        int y = 1 + rand() % (MAP_HEIGHT - h - 2);

        Room candidate = { x, y, w, h };
        int ok = 1;
        for (int i = 0; i < map->room_count; i++) {
            if (rooms_overlap(candidate, map->rooms[i])) { ok = 0; break; }
        }
        if (!ok) continue;

        carve_room(map, candidate);

        if (map->room_count > 0) {
            Room prev = map->rooms[map->room_count - 1];
            int px = prev.x + prev.w / 2;
            int py = prev.y + prev.h / 2;
            int cx = candidate.x + candidate.w / 2;
            int cy = candidate.y + candidate.h / 2;
            carve_corridor(map, px, py, cx, cy);
        }

        map->rooms[map->room_count++] = candidate;
    }

    /* Guarantee at least one room even in a pathological failure case. */
    if (map->room_count == 0) {
        Room fallback = { 2, 2, 6, 5 };
        carve_room(map, fallback);
        map->rooms[map->room_count++] = fallback;
    }

    /* Stairs down go in the last room generated, away from the start room. */
    Room last = map->rooms[map->room_count - 1];
    map->stairs_x = last.x + last.w / 2;
    map->stairs_y = last.y + last.h / 2;
    map->tiles[map->stairs_y][map->stairs_x].type = TILE_STAIRS_DOWN;
}

int map_is_walkable(const Map *map, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    return map->tiles[y][x].type != TILE_WALL;
}

/* Bresenham line-of-sight: walk the line from (x0,y0) to (x1,y1) and fail
 * as soon as a wall is encountered before reaching the destination. The
 * destination tile itself is allowed to be a wall (so you can *see* a wall
 * you're standing next to) but nothing beyond an intermediate wall is visible. */
int has_line_of_sight(const Map *map, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0, y = y0;
    while (1) {
        if (x == x1 && y == y1) return 1;
        if (!(x == x0 && y == y0) && map->tiles[y][x].type == TILE_WALL) return 0;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

void fov_update(Map *map, int px, int py) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map->tiles[y][x].visible = 0;
        }
    }

    int y0 = py - FOV_RADIUS < 0 ? 0 : py - FOV_RADIUS;
    int y1 = py + FOV_RADIUS >= MAP_HEIGHT ? MAP_HEIGHT - 1 : py + FOV_RADIUS;
    int x0 = px - FOV_RADIUS < 0 ? 0 : px - FOV_RADIUS;
    int x1 = px + FOV_RADIUS >= MAP_WIDTH ? MAP_WIDTH - 1 : px + FOV_RADIUS;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            double dist = sqrt((double)((x - px) * (x - px) + (y - py) * (y - py)));
            if (dist > FOV_RADIUS) continue;
            if (has_line_of_sight(map, px, py, x, y)) {
                map->tiles[y][x].visible = 1;
                map->tiles[y][x].discovered = 1;
            }
        }
    }
}
