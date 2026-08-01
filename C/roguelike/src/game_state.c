#include <string.h>
#include <stdio.h>
#include "game.h"

void game_log(GameState *gs, const char *msg) {
    if (gs->message_count < MAX_MESSAGES) {
        strncpy(gs->messages[gs->message_count], msg, 79);
        gs->messages[gs->message_count][79] = '\0';
        gs->message_count++;
    } else {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            strcpy(gs->messages[i], gs->messages[i + 1]);
        }
        strncpy(gs->messages[MAX_MESSAGES - 1], msg, 79);
        gs->messages[MAX_MESSAGES - 1][79] = '\0';
    }
}

static void place_player_in_first_room(GameState *gs) {
    Room start = gs->map.rooms[0];
    gs->player.x = start.x + start.w / 2;
    gs->player.y = start.y + start.h / 2;
}

void game_state_init(GameState *gs) {
    gs->depth = 1;
    gs->turn = 0;
    gs->game_over = 0;
    gs->won = 0;
    gs->message_count = 0;

    player_init(&gs->player);
    map_generate(&gs->map, gs->depth);
    place_player_in_first_room(gs);
    fov_update(&gs->map, gs->player.x, gs->player.y);

    monster_spawn_wave(gs->monsters, &gs->monster_count, &gs->map, gs->depth);
    item_spawn_wave(gs->items, &gs->item_count, &gs->map, gs->depth);

    game_log(gs, "You descend into the dungeon.");
}

void game_descend(GameState *gs) {
    gs->depth++;
    map_generate(&gs->map, gs->depth);
    place_player_in_first_room(gs);
    fov_update(&gs->map, gs->player.x, gs->player.y);

    monster_spawn_wave(gs->monsters, &gs->monster_count, &gs->map, gs->depth);
    item_spawn_wave(gs->items, &gs->item_count, &gs->map, gs->depth);

    char buf[80];
    snprintf(buf, sizeof(buf), "You descend to depth %d.", gs->depth);
    game_log(gs, buf);

    if (gs->depth >= MAX_DEPTH) {
        game_log(gs, "The air grows hot. Something ancient stirs nearby...");
    }
}
