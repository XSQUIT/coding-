#include <ncurses.h>
#include <string.h>
#include "game.h"

#define COLOR_PAIR_WALL      1
#define COLOR_PAIR_FLOOR_LIT 2
#define COLOR_PAIR_FLOOR_DIM 3
#define COLOR_PAIR_STAIRS    4
#define COLOR_PAIR_PLAYER    5
#define COLOR_PAIR_MONSTER   6
#define COLOR_PAIR_ITEM      7
#define COLOR_PAIR_HUD       8

void render_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(COLOR_PAIR_WALL,      COLOR_WHITE,  COLOR_BLACK);
        init_pair(COLOR_PAIR_FLOOR_LIT, COLOR_WHITE,  COLOR_BLACK);
        init_pair(COLOR_PAIR_FLOOR_DIM, COLOR_BLUE,   COLOR_BLACK);
        init_pair(COLOR_PAIR_STAIRS,    COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_PLAYER,    COLOR_CYAN,   COLOR_BLACK);
        init_pair(COLOR_PAIR_MONSTER,   COLOR_RED,    COLOR_BLACK);
        init_pair(COLOR_PAIR_ITEM,      COLOR_GREEN,  COLOR_BLACK);
        init_pair(COLOR_PAIR_HUD,       COLOR_YELLOW, COLOR_BLACK);
    }
}

void render_shutdown(void) {
    endwin();
}

static void draw_map(const Map *map) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            const Tile *t = &map->tiles[y][x];
            if (!t->discovered) {
                move(y, x);
                addch(' ');
                continue;
            }

            chtype ch;
            int pair;
            switch (t->type) {
                case TILE_WALL:        ch = '#'; pair = COLOR_PAIR_WALL; break;
                case TILE_STAIRS_DOWN: ch = '>'; pair = COLOR_PAIR_STAIRS; break;
                default:                ch = '.'; pair = t->visible ? COLOR_PAIR_FLOOR_LIT : COLOR_PAIR_FLOOR_DIM;
            }
            if (!t->visible && t->type != TILE_STAIRS_DOWN) pair = COLOR_PAIR_FLOOR_DIM;

            move(y, x);
            attron(COLOR_PAIR(pair));
            addch(ch);
            attroff(COLOR_PAIR(pair));
        }
    }
}

static void draw_items(const GameState *gs) {
    for (int i = 0; i < gs->item_count; i++) {
        const Item *it = &gs->items[i];
        if (!it->active) continue;
        if (!gs->map.tiles[it->y][it->x].visible) continue;

        move(it->y, it->x);
        attron(COLOR_PAIR(COLOR_PAIR_ITEM));
        addch(it->glyph);
        attroff(COLOR_PAIR(COLOR_PAIR_ITEM));
    }
}

static void draw_monsters(const GameState *gs) {
    for (int i = 0; i < gs->monster_count; i++) {
        const Monster *m = &gs->monsters[i];
        if (!m->alive) continue;
        if (!gs->map.tiles[m->y][m->x].visible) continue;

        move(m->y, m->x);
        attron(COLOR_PAIR(COLOR_PAIR_MONSTER) | A_BOLD);
        addch(m->glyph);
        attroff(COLOR_PAIR(COLOR_PAIR_MONSTER) | A_BOLD);
    }
}

static void draw_player(const GameState *gs) {
    move(gs->player.y, gs->player.x);
    attron(COLOR_PAIR(COLOR_PAIR_PLAYER) | A_BOLD);
    addch('@');
    attroff(COLOR_PAIR(COLOR_PAIR_PLAYER) | A_BOLD);
}

static void draw_hud(const GameState *gs, const char *status_line) {
    int hud_row = MAP_HEIGHT + 1;
    move(hud_row, 0);
    clrtoeol();

    attron(COLOR_PAIR(COLOR_PAIR_HUD));
    mvprintw(hud_row, 0,
        "HP %d/%d  Lv %d  XP %d/%d  Atk %d+%d  Def %d+%d  Gold %d  Depth %d/%d",
        gs->player.hp, gs->player.max_hp,
        gs->player.level, gs->player.xp, gs->player.xp_next,
        gs->player.base_attack, gs->player.weapon_bonus,
        gs->player.base_defense, gs->player.armor_bonus,
        gs->player.gold, gs->depth, MAX_DEPTH);
    attroff(COLOR_PAIR(COLOR_PAIR_HUD));

    move(hud_row + 1, 0);
    clrtoeol();
    mvprintw(hud_row + 1, 0, "%s", status_line ? status_line : "");

    int log_row = hud_row + 3;
    for (int i = 0; i < gs->message_count; i++) {
        move(log_row + i, 0);
        clrtoeol();
        mvprintw(log_row + i, 0, "%s", gs->messages[i]);
    }
}

void render_frame(const GameState *gs, const char *status_line) {
    erase();
    draw_map(&gs->map);
    draw_items(gs);
    draw_monsters(gs);
    draw_player(gs);
    draw_hud(gs, status_line);
    refresh();
}

void render_inventory(const GameState *gs) {
    erase();
    mvprintw(0, 0, "=== Inventory (press a letter to use, any other key to close) ===");
    if (gs->player.inventory_count == 0) {
        mvprintw(2, 0, "  (empty)");
    }
    for (int i = 0; i < gs->player.inventory_count; i++) {
        const Item *it = &gs->player.inventory[i];
        mvprintw(2 + i, 0, "  %c) %s", 'a' + i, it->name);
    }
    mvprintw(2 + gs->player.inventory_count + 2, 0, "Gold: %d", gs->player.gold);
    refresh();
}

void render_game_over(const GameState *gs) {
    erase();
    if (gs->won) {
        mvprintw(2, 2, "*** VICTORY ***");
        mvprintw(4, 2, "You slew the dragon at depth %d and escaped with %d gold.",
                 gs->depth, gs->player.gold);
    } else {
        mvprintw(2, 2, "*** YOU DIED ***");
        mvprintw(4, 2, "You fell at depth %d, level %d, with %d gold.",
                 gs->depth, gs->player.level, gs->player.gold);
    }
    mvprintw(6, 2, "Turns survived: %d", gs->turn);
    mvprintw(8, 2, "Press any key to exit.");
    refresh();
    nodelay(stdscr, FALSE);
    getch();
}
