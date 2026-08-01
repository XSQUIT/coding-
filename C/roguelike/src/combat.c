#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"

void player_init(Player *p) {
    p->x = 0;
    p->y = 0;
    p->hp = 30;
    p->max_hp = 30;
    p->base_attack = 5;
    p->base_defense = 2;
    p->weapon_bonus = 0;
    p->armor_bonus = 0;
    p->level = 1;
    p->xp = 0;
    p->xp_next = 20;
    p->gold = 0;
    p->inventory_count = 0;
    memset(p->inventory, 0, sizeof(p->inventory));
}

/* Returns 1 if the monster died from this attack, 0 otherwise.
 * Writes a human-readable summary into msg_out (must hold >=80 bytes). */
int player_attack(Player *p, Monster *m, char *msg_out) {
    int total_attack = p->base_attack + p->weapon_bonus;
    int roll = rand() % 20;
    int hit_chance = 13 + total_attack - m->defense;
    if (hit_chance < 2) hit_chance = 2;
    if (hit_chance > 19) hit_chance = 19;

    if (roll >= hit_chance) {
        snprintf(msg_out, 80, "You swing at the %s and miss.", m->name);
        return 0;
    }

    int dmg = total_attack - m->defense / 2;
    if (dmg < 1) dmg = 1;
    m->hp -= dmg;

    if (m->hp <= 0) {
        m->hp = 0;
        m->alive = 0;
        snprintf(msg_out, 80, "You slay the %s! (+%d dmg)", m->name, dmg);
        return 1;
    }

    snprintf(msg_out, 80, "You hit the %s for %d damage.", m->name, dmg);
    return 0;
}

/* Returns 1 if the player leveled up as a result. */
int player_gain_xp(Player *p, int amount, char *msg_out) {
    p->xp += amount;
    if (p->xp >= p->xp_next) {
        p->xp -= p->xp_next;
        p->level++;
        p->xp_next = p->xp_next * 3 / 2;
        p->max_hp += 8;
        p->hp = p->max_hp; /* full heal on level up, classic roguelike reward */
        p->base_attack += 2;
        p->base_defense += 1;
        snprintf(msg_out, 80, "You reached level %d! (+%d XP)", p->level, amount);
        return 1;
    }
    snprintf(msg_out, 80, "+%d XP", amount);
    return 0;
}
