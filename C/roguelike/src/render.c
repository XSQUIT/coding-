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

static int in_map(int x, int y)
{
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

static void draw_map(const Map *map, int cam_x, int cam_y,
                     int view_w, int view_h)
{
    for (int sy = 0; sy < view_h; sy++) {
        for (int sx = 0; sx < view_w; sx++) {
            int x = cam_x + sx;
            int y = cam_y + sy;

            if (!in_map(x, y))
                continue;

            const Tile *t = &map->tiles[y][x];
            chtype ch = ' ';
            int pair = COLOR_PAIR_FLOOR_DIM;

            if (t->discovered) {
                switch (t->type) {
                    case TILE_WALL:
                        ch = '#';
                        pair = COLOR_PAIR_WALL;
                        break;
                    case TILE_STAIRS_DOWN:
                        ch = '>';
                        pair = COLOR_PAIR_STAIRS;
                        break;
                    default:
                        ch = '.';
                        pair = t->visible
                            ? COLOR_PAIR_FLOOR_LIT
                            : COLOR_PAIR_FLOOR_DIM;
                        break;
                }

                if (!t->visible && t->type != TILE_STAIRS_DOWN)
                    pair = COLOR_PAIR_FLOOR_DIM;
            }

            move(sy, sx);
            attron(COLOR_PAIR(pair));
            addch(ch);
            attroff(COLOR_PAIR(pair));
        }
    }
}

static void draw_items(const GameState *gs, int cam_x, int cam_y,
                       int view_w, int view_h)
{
    for (int i = 0; i < gs->item_count; i++) {
        const Item *it = &gs->items[i];

        if (!it->active || !in_map(it->x, it->y))
            continue;
        if (!gs->map.tiles[it->y][it->x].visible)
            continue;

        int sx = it->x - cam_x;
        int sy = it->y - cam_y;
        if (sx < 0 || sx >= view_w || sy < 0 || sy >= view_h)
            continue;

        move(sy, sx);
        attron(COLOR_PAIR(COLOR_PAIR_ITEM));
        addch(it->glyph);
        attroff(COLOR_PAIR(COLOR_PAIR_ITEM));
    }
}

static void draw_monsters(const GameState *gs, int cam_x, int cam_y,
                          int view_w, int view_h)
{
    for (int i = 0; i < gs->monster_count; i++) {
        const Monster *m = &gs->monsters[i];

        if (!m->alive || !in_map(m->x, m->y))
            continue;
        if (!gs->map.tiles[m->y][m->x].visible)
            continue;

        int sx = m->x - cam_x;
        int sy = m->y - cam_y;
        if (sx < 0 || sx >= view_w || sy < 0 || sy >= view_h)
            continue;

        move(sy, sx);
        attron(COLOR_PAIR(COLOR_PAIR_MONSTER) | A_BOLD);
        addch(m->glyph);
        attroff(COLOR_PAIR(COLOR_PAIR_MONSTER) | A_BOLD);
    }
}

static void draw_player(const GameState *gs, int cam_x, int cam_y,
                        int view_w, int view_h)
{
    int sx = gs->player.x - cam_x;
    int sy = gs->player.y - cam_y;

    if (sx < 0 || sx >= view_w || sy < 0 || sy >= view_h)
        return;

    move(sy, sx);
    attron(COLOR_PAIR(COLOR_PAIR_PLAYER) | A_BOLD);
    addch('@');
    attroff(COLOR_PAIR(COLOR_PAIR_PLAYER) | A_BOLD);
}

static void draw_hud(const GameState *gs, const char *status_line,
                     int hud_row, int rows, int cols)
{
    attron(COLOR_PAIR(COLOR_PAIR_HUD));
    mvprintw(hud_row, 0,
             "HP %d/%d  Lv %d  XP %d/%d  Atk %d+%d  Def %d+%d  Gold %d  Depth %d/%d",
             gs->player.hp, gs->player.max_hp,
             gs->player.level, gs->player.xp, gs->player.xp_next,
             gs->player.base_attack, gs->player.weapon_bonus,
             gs->player.base_defense, gs->player.armor_bonus,
             gs->player.gold, gs->depth, MAX_DEPTH);
    attroff(COLOR_PAIR(COLOR_PAIR_HUD));

    if (hud_row + 1 < rows) {
        mvaddnstr(hud_row + 1, 0,
                  status_line ? status_line : "",
                  cols - 1);
    }

    for (int i = 0; i < gs->message_count; i++) {
        int row = hud_row + 2 + i;
        if (row >= rows)
            break;

        mvaddnstr(row, 0, gs->messages[i], cols - 1);
    }
}

void render_frame(const GameState *gs, const char *status_line)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();

    if (rows < 4 || cols < 10) {
        mvaddstr(0, 0, "Terminal too small");
        refresh();
        return;
    }

    /* Reserve rows for the HUD, status line, and visible message log. */
    int log_rows = gs->message_count;
    if (log_rows > rows - 3)
        log_rows = rows - 3;

    int hud_row = rows - (log_rows + 2);
    int view_h = hud_row;
    int view_w = cols - 1; /* Avoid writing into ncurses' bottom-right cell. */

    if (view_h < 1)
        view_h = 1;

    /* Center the camera on the player, clamping at map edges. */
    int cam_x = gs->player.x - view_w / 2;
    int cam_y = gs->player.y - view_h / 2;

    int max_cam_x = MAP_WIDTH > view_w ? MAP_WIDTH - view_w : 0;
    int max_cam_y = MAP_HEIGHT > view_h ? MAP_HEIGHT - view_h : 0;

    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > max_cam_x) cam_x = max_cam_x;
    if (cam_y > max_cam_y) cam_y = max_cam_y;

    draw_map(&gs->map, cam_x, cam_y, view_w, view_h);
    draw_items(gs, cam_x, cam_y, view_w, view_h);
    draw_monsters(gs, cam_x, cam_y, view_w, view_h);
    draw_player(gs, cam_x, cam_y, view_w, view_h);
    draw_hud(gs, status_line, hud_row, rows, cols);

    refresh();
}
s
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
