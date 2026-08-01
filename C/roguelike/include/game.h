#ifndef GAME_H
#define GAME_H

/* ============================================================
 * game.h
 *
 * Shared types and function declarations for the whole game.
 * Every subsystem (map generation, entities, combat, items,
 * field of view, rendering) includes this single header rather
 * than passing structs around via void pointers, which keeps
 * things simple at the cost of some coupling -- a reasonable
 * trade for a project this size.
 * ============================================================ */

#define MAP_WIDTH        60
#define MAP_HEIGHT       20
#define MAX_ROOMS        14
#define MIN_ROOM_SIZE     4
#define MAX_ROOM_SIZE     9

#define MAX_MONSTERS     30
#define MAX_ITEMS        20
#define MAX_INVENTORY    12
#define MAX_MESSAGES      5
#define MAX_DEPTH        10

#define FOV_RADIUS        8

/* Forward-declared here (and given its full definition further down) so
 * that monster_take_turn's prototype below can reference it by pointer
 * without accidentally creating a second, parameter-scoped "struct Player"
 * that the compiler would treat as a distinct, conflicting type. */
typedef struct Player Player;

/* ---------------- Map ---------------- */

typedef enum {
    TILE_WALL = 0,
    TILE_FLOOR,
    TILE_STAIRS_DOWN
} TileType;

typedef struct {
    TileType type;
    int visible;     /* lit this turn */
    int discovered;  /* ever seen (drawn dim when not currently visible) */
} Tile;

typedef struct {
    int x, y, w, h;
} Room;

typedef struct {
    Tile tiles[MAP_HEIGHT][MAP_WIDTH];
    Room rooms[MAX_ROOMS];
    int room_count;
    int stairs_x, stairs_y;
} Map;

void map_generate(Map *map, int depth);
int  map_is_walkable(const Map *map, int x, int y);
int  has_line_of_sight(const Map *map, int x0, int y0, int x1, int y1);
void fov_update(Map *map, int px, int py);

/* ---------------- Items ---------------- */

typedef enum {
    ITEM_POTION_HEAL = 0,
    ITEM_SCROLL_STRENGTH,
    ITEM_WEAPON,
    ITEM_ARMOR,
    ITEM_GOLD,
    ITEM_TYPE_COUNT
} ItemKind;

typedef struct {
    ItemKind kind;
    char name[32];
    char glyph;
    int value;       /* heal amount / attack bonus / defense bonus / gold amount */
    int x, y;
    int active;      /* still exists / still on the ground */
} Item;

/* ---------------- Monsters ---------------- */

typedef enum {
    MONSTER_RAT = 0,
    MONSTER_GOBLIN,
    MONSTER_ORC,
    MONSTER_TROLL,
    MONSTER_DRAGON,
    MONSTER_TYPE_COUNT
} MonsterKind;

typedef struct {
    MonsterKind kind;
    char name[16];
    char glyph;
    int x, y;
    int hp, max_hp;
    int attack, defense;
    int xp_reward;
    int alive;
    int awake;       /* has it noticed the player yet */
} Monster;

void monster_spawn_wave(Monster monsters[], int *count, const Map *map, int depth);
void monster_take_turn(Monster *m, struct Player *player, Map *map, char messages_out[][80], int *msg_count);

/* ---------------- Player ---------------- */

typedef struct Player {
    int x, y;
    int hp, max_hp;
    int base_attack, base_defense;
    int weapon_bonus, armor_bonus;
    int level, xp, xp_next;
    int gold;
    Item inventory[MAX_INVENTORY];
    int inventory_count;
} Player;

void player_init(Player *p);
int  player_attack(Player *p, Monster *m, char *msg_out);
int  player_gain_xp(Player *p, int amount, char *msg_out);
int  player_pickup(Player *p, Item *item, char *msg_out);
int  player_use_inventory_slot(Player *p, int slot, char *msg_out);

/* ---------------- Game state / messages ---------------- */

typedef struct {
    Map map;
    Player player;
    Monster monsters[MAX_MONSTERS];
    int monster_count;
    Item items[MAX_ITEMS];
    int item_count;
    int depth;
    int turn;
    int game_over;
    int won;
    char messages[MAX_MESSAGES][80];
    int message_count;
} GameState;

void game_state_init(GameState *gs);
void game_log(GameState *gs, const char *msg);
void game_descend(GameState *gs);
void item_spawn_wave(Item items[], int *count, const Map *map, int depth);

/* ---------------- Rendering (ncurses) ---------------- */

void render_frame(const GameState *gs, const char *status_line);
void render_inventory(const GameState *gs);
void render_game_over(const GameState *gs);
void render_init(void);
void render_shutdown(void);

#endif /* GAME_H */
