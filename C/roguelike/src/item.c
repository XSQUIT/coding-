#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "game.h"

static int random_floor_tile_for_item(const Map *map, int *out_x, int *out_y) {
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

void item_spawn_wave(Item items[], int *count, const Map *map, int depth) {
    *count = 0;
    int n = 4 + depth / 2;
    if (n > MAX_ITEMS) n = MAX_ITEMS;

    for (int i = 0; i < n; i++) {
        int x, y;
        if (!random_floor_tile_for_item(map, &x, &y)) continue;

        /* Weighted-ish selection: potions and gold are common, gear is rarer. */
        int roll = rand() % 100;
        ItemKind kind;
        if (roll < 35) kind = ITEM_POTION_HEAL;
        else if (roll < 55) kind = ITEM_GOLD;
        else if (roll < 70) kind = ITEM_SCROLL_STRENGTH;
        else if (roll < 85) kind = ITEM_WEAPON;
        else kind = ITEM_ARMOR;

        Item *it = &items[*count];
        it->kind = kind;
        it->x = x;
        it->y = y;
        it->active = 1;

        switch (kind) {
            case ITEM_POTION_HEAL:
                strcpy(it->name, "potion of healing");
                it->glyph = '!';
                it->value = 12 + depth;
                break;
            case ITEM_SCROLL_STRENGTH:
                strcpy(it->name, "scroll of strength");
                it->glyph = '?';
                it->value = 2;
                break;
            case ITEM_WEAPON:
                strcpy(it->name, "weapon");
                it->glyph = '/';
                it->value = 3 + depth;
                break;
            case ITEM_ARMOR:
                strcpy(it->name, "armor");
                it->glyph = '[';
                it->value = 2 + depth / 2;
                break;
            case ITEM_GOLD:
                strcpy(it->name, "gold");
                it->glyph = '$';
                it->value = 5 + rand() % (10 + depth * 3);
                break;
            default:
                break;
        }
        (*count)++;
    }
}

int player_pickup(Player *p, Item *item, char *msg_out) {
    if (!item->active) return 0;

    switch (item->kind) {
        case ITEM_GOLD:
            p->gold += item->value;
            snprintf(msg_out, 80, "You pick up %d gold.", item->value);
            item->active = 0;
            return 1;

        case ITEM_WEAPON:
            if (item->value > p->weapon_bonus) {
                p->weapon_bonus = item->value;
                snprintf(msg_out, 80, "You equip a better weapon (+%d attack).", item->value);
            } else {
                snprintf(msg_out, 80, "You find a weapon, but yours is already better.");
            }
            item->active = 0;
            return 1;

        case ITEM_ARMOR:
            if (item->value > p->armor_bonus) {
                p->armor_bonus = item->value;
                snprintf(msg_out, 80, "You equip better armor (+%d defense).", item->value);
            } else {
                snprintf(msg_out, 80, "You find armor, but yours is already better.");
            }
            item->active = 0;
            return 1;

        case ITEM_POTION_HEAL:
        case ITEM_SCROLL_STRENGTH:
            if (p->inventory_count >= MAX_INVENTORY) {
                snprintf(msg_out, 80, "Your inventory is full.");
                return 0;
            }
            p->inventory[p->inventory_count] = *item;
            p->inventory_count++;
            snprintf(msg_out, 80, "You pick up a %s.", item->name);
            item->active = 0;
            return 1;

        default:
            return 0;
    }
}

int player_use_inventory_slot(Player *p, int slot, char *msg_out) {
    if (slot < 0 || slot >= p->inventory_count) {
        snprintf(msg_out, 80, "Nothing in that slot.");
        return 0;
    }

    Item it = p->inventory[slot];

    if (it.kind == ITEM_POTION_HEAL) {
        int healed = it.value;
        p->hp += healed;
        if (p->hp > p->max_hp) p->hp = p->max_hp;
        snprintf(msg_out, 80, "You drink the potion and recover %d HP.", healed);
    } else if (it.kind == ITEM_SCROLL_STRENGTH) {
        p->base_attack += it.value;
        snprintf(msg_out, 80, "You read the scroll. Attack +%d permanently.", it.value);
    } else {
        snprintf(msg_out, 80, "You can't use that right now.");
        return 0;
    }

    /* Remove the used item by shifting the rest of the inventory down. */
    for (int i = slot; i < p->inventory_count - 1; i++) {
        p->inventory[i] = p->inventory[i + 1];
    }
    p->inventory_count--;
    return 1;
}
