#!/usr/bin/env python3
"""Terminal Snake — arrow keys to move, 'q' to quit."""
import curses
import random


def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.timeout(100)

    sh, sw = stdscr.getmaxyx()
    h, w = sh - 1, sw - 1

    snake = [(h // 2, w // 2 + i) for i in range(3)]
    direction = curses.KEY_LEFT

    def new_food():
        while True:
            f = (random.randint(1, h - 1), random.randint(1, w - 1))
            if f not in snake:
                return f

    food = new_food()
    score = 0

    while True:
        key = stdscr.getch()
        if key == ord('q'):
            break
        if key in (curses.KEY_UP, curses.KEY_DOWN, curses.KEY_LEFT, curses.KEY_RIGHT):
            # prevent reversing directly into yourself
            opposites = {
                curses.KEY_UP: curses.KEY_DOWN,
                curses.KEY_DOWN: curses.KEY_UP,
                curses.KEY_LEFT: curses.KEY_RIGHT,
                curses.KEY_RIGHT: curses.KEY_LEFT,
            }
            if key != opposites.get(direction):
                direction = key

        head_y, head_x = snake[0]
        if direction == curses.KEY_UP:
            new_head = (head_y - 1, head_x)
        elif direction == curses.KEY_DOWN:
            new_head = (head_y + 1, head_x)
        elif direction == curses.KEY_LEFT:
            new_head = (head_y, head_x - 1)
        else:
            new_head = (head_y, head_x + 1)

        # wall / self collision
        if (
            new_head[0] <= 0 or new_head[0] >= h
            or new_head[1] <= 0 or new_head[1] >= w
            or new_head in snake
        ):
            break

        snake.insert(0, new_head)
        if new_head == food:
            score += 1
            food = new_food()
        else:
            snake.pop()

        stdscr.clear()
        stdscr.border()
        stdscr.addstr(0, 2, f" Score: {score} ")
        stdscr.addch(food[0], food[1], '@')
        for y, x in snake:
            stdscr.addch(y, x, '#')
        stdscr.refresh()

    stdscr.nodelay(False)
    stdscr.addstr(h // 2, w // 2 - 5, f"Game Over! Score: {score}")
    stdscr.addstr(h // 2 + 1, w // 2 - 8, "Press any key to exit")
    stdscr.getch()


if __name__ == "__main__":
    curses.wrapper(main)