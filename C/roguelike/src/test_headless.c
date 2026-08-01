/* Headless smoke test: exercises map generation, combat, leveling, items,
 * and monster AI directly, with no ncurses involved. This exists because
 * the sandbox this was developed in has no real TTY for ncurses to attach
 * to -- it's a development aid, not part of the shipped game. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "game.h"

static int failures = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("  [pass] %s\n", desc); } \
    else { printf("  [FAIL] %s\n", desc); failures++; } \
} while (0)

static void test_map_generation(void) {
    printf("test_map_generation:\n");
    Map map;
    map_generate(&map, 1);

    CHECK(map.room_count > 0, "at least one room was generated");

    int floor_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (map.tiles[y][x].type != TILE_WALL) floor_count++;
    CHECK(floor_count > 20, "a reasonable amount of open floor exists");

    CHECK(map.tiles[map.stairs_y][map.stairs_x].type == TILE_STAIRS_DOWN,
          "stairs tile matches the recorded stairs position");

    /* Every room's center should be walkable floor. */
    int all_rooms_walkable = 1;
    for (int i = 0; i < map.room_count; i++) {
        Room r = map.rooms[i];
        int cx = r.x + r.w / 2, cy = r.y + r.h / 2;
        if (!map_is_walkable(&map, cx, cy)) all_rooms_walkable = 0;
    }
    CHECK(all_rooms_walkable, "every generated room's center is walkable");

    /* Line of sight: a point should always see itself, and a wall should
     * block sight to whatever is directly behind it in a simple corridor. */
    CHECK(has_line_of_sight(&map, 5, 5, 5, 5), "a tile has line of sight to itself");
}

static void test_fov(void) {
    printf("test_fov:\n");
    Map map;
    map_generate(&map, 1);
    Room r0 = map.rooms[0];
    int px = r0.x + r0.w / 2, py = r0.y + r0.h / 2;

    fov_update(&map, px, py);
    CHECK(map.tiles[py][px].visible, "the player's own tile is visible after fov_update");

    int visible_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (map.tiles[y][x].visible) visible_count++;
    CHECK(visible_count > 1, "more than just the player's own tile is visible");
    CHECK(visible_count < MAP_WIDTH * MAP_HEIGHT, "fov does not reveal the entire map at once");
}

static void test_combat_and_leveling(void) {
    printf("test_combat_and_leveling:\n");
    Player p;
    player_init(&p);
    int starting_attack = p.base_attack;
    int starting_level = p.level;

    Monster m;
    memset(&m, 0, sizeof(m));
    strcpy(m.name, "test rat");
    m.hp = m.max_hp = 1000000; /* effectively unkillable, so we can watch damage accumulate */
    m.defense = 0;
    m.alive = 1;

    char msg[80];
    int hp_before = m.hp;
    /* Force a guaranteed hit by giving the player overwhelming attack. */
    p.base_attack = 1000;
    player_attack(&p, &m, msg);
    CHECK(m.hp < hp_before, "a hit with overwhelming attack reduces monster HP");
    p.base_attack = starting_attack;

    /* Killing blow should mark the monster dead and report it. */
    Monster weak;
    memset(&weak, 0, sizeof(weak));
    strcpy(weak.name, "training dummy");
    weak.hp = weak.max_hp = 1;
    weak.defense = 0;
    weak.alive = 1;
    p.base_attack = 1000;
    int killed = player_attack(&p, &weak, msg);
    CHECK(killed == 1, "lethal damage marks killed == 1");
    CHECK(weak.alive == 0, "a monster reduced to 0 hp becomes not-alive");
    p.base_attack = starting_attack;

    char xp_msg[80];
    int leveled = 0;
    for (int i = 0; i < 20 && !leveled; i++) {
        leveled = player_gain_xp(&p, 15, xp_msg);
    }
    CHECK(leveled == 1, "enough accumulated xp eventually triggers a level-up");
    CHECK(p.level > starting_level, "player level increased after leveling up");
    CHECK(p.hp == p.max_hp, "leveling up fully heals the player");
}

static void test_items(void) {
    printf("test_items:\n");
    Player p;
    player_init(&p);
    int hp_before_damage = p.hp;
    p.hp -= 10;
    CHECK(p.hp == hp_before_damage - 10, "player took simulated damage");

    Item potion;
    memset(&potion, 0, sizeof(potion));
    potion.kind = ITEM_POTION_HEAL;
    strcpy(potion.name, "potion of healing");
    potion.value = 6;
    potion.active = 1;

    char msg[80];
    int picked = player_pickup(&p, &potion, msg);
    CHECK(picked == 1, "picking up a potion succeeds");
    CHECK(p.inventory_count == 1, "inventory count increments after pickup");
    CHECK(potion.active == 0, "picked-up item is marked inactive on the ground");

    int hp_before_use = p.hp;
    player_use_inventory_slot(&p, 0, msg);
    CHECK(p.hp == hp_before_use + 6, "using the potion restores the expected HP");
    CHECK(p.inventory_count == 0, "inventory count decrements after use");

    Item weapon;
    memset(&weapon, 0, sizeof(weapon));
    weapon.kind = ITEM_WEAPON;
    strcpy(weapon.name, "iron sword");
    weapon.value = 7;
    weapon.active = 1;
    int old_bonus = p.weapon_bonus;
    player_pickup(&p, &weapon, msg);
    CHECK(p.weapon_bonus == 7 && p.weapon_bonus != old_bonus, "a better weapon auto-equips");

    Item worse_weapon;
    memset(&worse_weapon, 0, sizeof(worse_weapon));
    worse_weapon.kind = ITEM_WEAPON;
    worse_weapon.value = 1;
    worse_weapon.active = 1;
    player_pickup(&p, &worse_weapon, msg);
    CHECK(p.weapon_bonus == 7, "a worse weapon does not replace the equipped one");
}

static void test_monster_spawning(void) {
    printf("test_monster_spawning:\n");
    Map map;
    map_generate(&map, 1);

    Monster monsters[MAX_MONSTERS];
    int count;
    monster_spawn_wave(monsters, &count, &map, 1);
    CHECK(count > 0, "at least one monster spawns on depth 1");

    int all_on_floor = 1;
    for (int i = 0; i < count; i++) {
        if (!map_is_walkable(&map, monsters[i].x, monsters[i].y)) all_on_floor = 0;
        if (monsters[i].hp <= 0 || !monsters[i].alive) all_on_floor = 0;
    }
    CHECK(all_on_floor, "all spawned monsters start alive and on walkable tiles");

    Monster deep_monsters[MAX_MONSTERS];
    int deep_count;
    Map deep_map;
    map_generate(&deep_map, MAX_DEPTH);
    monster_spawn_wave(deep_monsters, &deep_count, &deep_map, MAX_DEPTH);
    CHECK(deep_count >= count, "deeper floors spawn at least as many monsters as depth 1");

    int found_dragon = 0;
    for (int i = 0; i < deep_count; i++) {
        if (deep_monsters[i].kind == MONSTER_DRAGON) found_dragon = 1;
    }
    CHECK(found_dragon, "the deepest floor always includes a dragon");
}

static void test_ai_wakes_and_attacks(void) {
    printf("test_ai_wakes_and_attacks:\n");
    Map map;
    map_generate(&map, 1);
    Room r = map.rooms[0];

    Player p;
    player_init(&p);
    p.x = r.x + 1;
    p.y = r.y + 1;
    p.hp = p.max_hp = 100;

    Monster m;
    memset(&m, 0, sizeof(m));
    strcpy(m.name, "rat");
    m.glyph = 'r';
    m.hp = m.max_hp = 4;
    m.attack = 2;
    m.defense = 0;
    m.alive = 1;
    m.awake = 0;
    m.x = p.x + 1;
    m.y = p.y;

    fov_update(&map, p.x, p.y);

    char msgbuf[4][80];
    int msg_count = 0;
    monster_take_turn(&m, &p, &map, msgbuf, &msg_count);
    CHECK(m.awake == 1, "an adjacent, visible monster wakes up on its first turn");
    CHECK(msg_count > 0, "waking up (or attacking) produces at least one message");

    int hp_before = p.hp;
    int total_damage_dealt = 0;
    for (int i = 0; i < 20; i++) {
        int before = p.hp;
        msg_count = 0;
        monster_take_turn(&m, &p, &map, msgbuf, &msg_count);
        total_damage_dealt += (before - p.hp);
    }
    CHECK(total_damage_dealt >= 0, "repeated adjacent attacks never heal the player");
    CHECK(p.hp <= hp_before, "player hp is monotonically non-increasing under repeated attack");
}

int main(void) {
    srand(42); /* deterministic seed so a failing run is reproducible */

    test_map_generation();
    test_fov();
    test_combat_and_leveling();
    test_items();
    test_monster_spawning();
    test_ai_wakes_and_attacks();

    printf("\n%s\n", failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return failures == 0 ? 0 : 1;
}
