#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "game.h"

typedef struct {
    const char *name;
    char glyph;
    int base_hp, base_attack, base_defense, xp_reward;
    int min_depth; /* first depth this kind can appear on */
} MonsterTemplate;

static const MonsterTemplate MONSTER_TEMPLATES[MONSTER_TYPE_COUNT] = {
    { "rat",    'r',  4,  2, 0,  2, 1 },
    { "goblin", 'g',  9,  4, 1,  5, 1 },
    { "orc",    'o', 16,  6, 2, 10, 3 },
    { "troll",  'T', 28,  9, 3, 20, 5 },
    { "dragon", 'D', 55, 14, 5, 60, 8 },
};

static int random_floor_tile(const Map *map, int *out_x, int *out_y) {
    for (int tries = 0; tries < 500; tries++) {
        int room_idx = rand() % map->room_count;
        Room r = map->rooms[room_idx];
        int x = r.x + rand() % r.w;
        int y = r.y + rand() % r.h;
        if (map->tiles[y][x].type == TILE_FLOOR) {
            *out_x = x;
            *out_y = y;
            return 1;
        }
    }
    return 0;
}

void monster_spawn_wave(Monster monsters[], int *count, const Map *map, int depth) {
    *count = 0;

    /* More monsters, and tougher ones unlocked, as depth increases. */
    int n = 3 + depth;
    if (n > MAX_MONSTERS) n = MAX_MONSTERS;

    for (int i = 0; i < n; i++) {
        /* Build a small pool of kinds eligible at this depth and pick one. */
        int eligible[MONSTER_TYPE_COUNT];
        int eligible_count = 0;
        for (int k = 0; k < MONSTER_TYPE_COUNT; k++) {
            if (MONSTER_TEMPLATES[k].min_depth <= depth) eligible[eligible_count++] = k;
        }
        /* Boss: guarantee a dragon on the deepest floor. */
        int kind;
        if (depth >= MAX_DEPTH && i == n - 1) {
            kind = MONSTER_DRAGON;
        } else {
            kind = eligible[rand() % eligible_count];
        }

        int x, y;
        if (!random_floor_tile(map, &x, &y)) continue;

        const MonsterTemplate *t = &MONSTER_TEMPLATES[kind];
        Monster *m = &monsters[*count];
        m->kind = kind;
        strncpy(m->name, t->name, sizeof(m->name) - 1);
        m->name[sizeof(m->name) - 1] = '\0';
        m->glyph = t->glyph;
        m->x = x;
        m->y = y;
        /* Light scaling with depth so early monsters don't stay trivial forever. */
        m->max_hp = t->base_hp + depth;
        m->hp = m->max_hp;
        m->attack = t->base_attack + depth / 2;
        m->defense = t->base_defense + depth / 3;
        m->xp_reward = t->xp_reward;
        m->alive = 1;
        m->awake = 0;
        (*count)++;
    }
}

/* Very small step-toward-target helper used by monster AI: moves at most
 * one tile closer to (tx,ty), preferring whichever axis has the larger
 * gap, and falling back to the other axis if the first choice is blocked. */
static void step_toward(int *x, int *y, int tx, int ty, const Map *map) {
    int dx = (tx > *x) - (tx < *x);
    int dy = (ty > *y) - (ty < *y);

    if (abs(tx - *x) >= abs(ty - *y)) {
        if (dx != 0 && map_is_walkable(map, *x + dx, *y)) { *x += dx; return; }
        if (dy != 0 && map_is_walkable(map, *x, *y + dy)) { *y += dy; return; }
    } else {
        if (dy != 0 && map_is_walkable(map, *x, *y + dy)) { *y += dy; return; }
        if (dx != 0 && map_is_walkable(map, *x + dx, *y)) { *x += dx; return; }
    }
}

void monster_take_turn(Monster *m, struct Player *player, Map *map, char messages_out[][80], int *msg_count) {
    if (!m->alive) return;

    int adjacent = (abs(m->x - player->x) <= 1 && abs(m->y - player->y) <= 1);
    int visible = has_line_of_sight(map, m->x, m->y, player->x, player->y) &&
                  map->tiles[m->y][m->x].discovered == 1;

    if (!m->awake) {
        double dist_sq = (m->x - player->x) * (m->x - player->x) +
                          (m->y - player->y) * (m->y - player->y);
        if (visible && dist_sq <= (double)(FOV_RADIUS * FOV_RADIUS)) {
            m->awake = 1;
            if (*msg_count < MAX_MESSAGES) {
                snprintf(messages_out[*msg_count], 80, "The %s notices you!", m->name);
                (*msg_count)++;
            }
        } else {
            return; /* still asleep/unaware, do nothing this turn */
        }
    }

    if (adjacent) {
        int roll = rand() % 20;
        int hit_chance = 12 + m->attack - player->base_defense - player->armor_bonus;
        if (hit_chance < 2) hit_chance = 2;
        if (hit_chance > 19) hit_chance = 19;

        if (roll < hit_chance) {
            int dmg = m->attack - (player->base_defense + player->armor_bonus) / 2;
            if (dmg < 1) dmg = 1;
            player->hp -= dmg;
            if (*msg_count < MAX_MESSAGES) {
                snprintf(messages_out[*msg_count], 80, "The %s hits you for %d damage.", m->name, dmg);
                (*msg_count)++;
            }
        } else {
            if (*msg_count < MAX_MESSAGES) {
                snprintf(messages_out[*msg_count], 80, "The %s attacks but misses.", m->name);
                (*msg_count)++;
            }
        }
        return;
    }

    step_toward(&m->x, &m->y, player->x, player->y, map);
}
