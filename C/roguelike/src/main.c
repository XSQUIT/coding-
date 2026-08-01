#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "game.h"

static Monster *monster_at(GameState *gs, int x, int y) {
    for (int i = 0; i < gs->monster_count; i++) {
        if (gs->monsters[i].alive && gs->monsters[i].x == x && gs->monsters[i].y == y) {
            return &gs->monsters[i];
        }
    }
    return NULL;
}

static void try_pickup_here(GameState *gs, char *status_line) {
    for (int i = 0; i < gs->item_count; i++) {
        Item *it = &gs->items[i];
        if (it->active && it->x == gs->player.x && it->y == gs->player.y) {
            char msg[80];
            if (player_pickup(&gs->player, it, msg)) {
                game_log(gs, msg);
                strncpy(status_line, msg, 119);
            }
        }
    }
}

static void resolve_attack(GameState *gs, Monster *target, char *status_line) {
    char msg[80];
    int killed = player_attack(&gs->player, target, msg);
    game_log(gs, msg);
    strncpy(status_line, msg, 119);

    if (killed) {
        char xp_msg[80];
        int leveled = player_gain_xp(&gs->player, target->xp_reward, xp_msg);
        game_log(gs, xp_msg);
        (void)leveled;

        if (target->kind == MONSTER_DRAGON && gs->depth >= MAX_DEPTH) {
            gs->won = 1;
            gs->game_over = 1;
        }
    }
}

static void run_monster_turns(GameState *gs) {
    char msgbuf[4][80];
    for (int i = 0; i < gs->monster_count; i++) {
        if (!gs->monsters[i].alive) continue;
        int msg_count = 0;
        monster_take_turn(&gs->monsters[i], &gs->player, &gs->map, msgbuf, &msg_count);
        for (int j = 0; j < msg_count; j++) {
            game_log(gs, msgbuf[j]);
        }
        if (gs->player.hp <= 0) {
            gs->player.hp = 0;
            gs->game_over = 1;
            gs->won = 0;
            return;
        }
    }
}

static void inventory_loop(GameState *gs) {
    while (1) {
        render_inventory(gs);
        int ch = getch();
        if (ch >= 'a' && ch < 'a' + gs->player.inventory_count) {
            char msg[80];
            player_use_inventory_slot(&gs->player, ch - 'a', msg);
            game_log(gs, msg);
            return;
        }
        /* any other key closes the inventory without using anything */
        return;
    }
}

static void handle_movement(GameState *gs, int dx, int dy, char *status_line) {
    int nx = gs->player.x + dx;
    int ny = gs->player.y + dy;

    Monster *target = monster_at(gs, nx, ny);
    if (target) {
        resolve_attack(gs, target, status_line);
    } else if (map_is_walkable(&gs->map, nx, ny)) {
        gs->player.x = nx;
        gs->player.y = ny;
        fov_update(&gs->map, gs->player.x, gs->player.y);
        try_pickup_here(gs, status_line);
    } else {
        strncpy(status_line, "You bump into a wall.", 119);
    }

    if (!gs->game_over) {
        run_monster_turns(gs);
    }
    gs->turn++;
}

int main(void) {
    srand((unsigned int)time(NULL));

    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_state_init(&gs);

    render_init();

    char status_line[120] = "Arrows/WASD/HJKL to move, bump monsters to attack, 'i' inventory, '>' descend, 'Q' quit.";

    while (!gs.game_over) {
        render_frame(&gs, status_line);
        int ch = getch();
        status_line[0] = '\0';

        switch (ch) {
            case KEY_UP:    case 'w': case 'k': handle_movement(&gs, 0, -1, status_line); break;
            case KEY_DOWN:  case 's': case 'j': handle_movement(&gs, 0,  1, status_line); break;
            case KEY_LEFT:  case 'a': case 'h': handle_movement(&gs, -1, 0, status_line); break;
            case KEY_RIGHT: case 'd': case 'l': handle_movement(&gs,  1, 0, status_line); break;

            case 'i':
                inventory_loop(&gs);
                break;

            case '>':
                if (gs.map.tiles[gs.player.y][gs.player.x].type == TILE_STAIRS_DOWN) {
                    if (gs.depth < MAX_DEPTH) {
                        game_descend(&gs);
                        strncpy(status_line, "You descend deeper into the dungeon.", 119);
                    } else {
                        strncpy(status_line, "This is the deepest level. The dragon is near.", 119);
                    }
                } else {
                    strncpy(status_line, "There are no stairs here.", 119);
                }
                break;

            case 'Q':
                gs.game_over = 1;
                gs.won = -1; /* sentinel meaning "quit", handled below */
                break;

            default:
                break;
        }
    }

    if (gs.won != -1) {
        render_game_over(&gs);
    }
    render_shutdown();
    return 0;
}
