#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> // Include for usleep

#define SPACE_SHIP_HEIGHT 4

#define BULLET '^'
#define BULLET_SPEED 1
#define MAX_BULLETS 10000 // Maximum number of bullets

#define ROCKET_SPEED 1
#define MAX_Rocket 5

#define ROCKET_MOVE_DELAY 20 // Adjust this value to control rocket movement speed

typedef struct
{
    int x;
    int y;
    int active; // Flag to indicate if the bullet is active or not
} Bullet;

typedef struct
{
    int x;
    int y;
    int active; // Flag to indicate if the rocket is active or not
} Rocket;

Bullet bullets[MAX_BULLETS]; // Array to hold multiple bullets
Rocket rockets[MAX_Rocket];     // Array to hold multiple rocket

int rocketMoveCounter = 0; // Counter to control rocket movement speed

// Function to draw the boundaries
void drawBoundaries()
{
    for (int i = 0; i < COLS; i++)
    {
        mvprintw(0, i, "#");
        mvprintw(LINES - 1, i, "#");
    }

    for (int i = 0; i < LINES; i++)
    {
        mvprintw(i, 0, "#");
        mvprintw(i, COLS - 1, "#");
    }
}

void drawSpaceship(int x, int y)
{
    mvprintw(y, x, "   I  ");
    mvprintw(y + 1, x, "  /-\\");
    mvprintw(y + 2, x, " |   |");
    mvprintw(y + 3, x, "/o o o\\");
}

// Function to move a bullet
void moveBullet(int index)
{
    // Move and draw the bullet if active
    mvprintw(bullets[index].y, bullets[index].x, " ");          // Clear previous bullet position
    bullets[index].y -= BULLET_SPEED;                           // Move the bullet upwards
    mvprintw(bullets[index].y, bullets[index].x, "%c", BULLET); // Draw the bullet
}

// Function to draw the rocket
void drawRocket(int x, int y)
{
    mvprintw(y, x, "\\ || / ");
    mvprintw(y + 1, x, " |oo| ");
    mvprintw(y + 2, x, " |oo| ");
    mvprintw(y + 3, x, "  \\/ ");
    mvprintw(y + 4, x, "  ");
}

// Function to move the rocket
void moveRocket(int index)
{
    if (rocketMoveCounter % ROCKET_MOVE_DELAY == 0)
    {                                                      // Move the rocket every ROCKET_MOVE_DELAY frames
        mvprintw(rockets[index].y, rockets[index].x, "   "); // Clear previous rocket position
        rockets[index].y += ROCKET_SPEED;                    // Move the rocket downwards
    }
}

// Function to check for collision between bullet and rocket
void checkCollision()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_Rocket; j++)
            {
                if (rockets[j].active &&
                    bullets[i].x >= rockets[j].x && bullets[i].x < rockets[j].x + 7 &&
                    bullets[i].y >= rockets[j].y && bullets[i].y < rockets[j].y + 5)
                {
                    bullets[i].active = 0; // Deactivate bullet
                    rockets[j].active = 0; // Deactivate rocket
                    break;
                }
            }
        }
    }
}

void main(void)
{
    // Initialize the curses library
    initscr();

    // Don't display typed characters on the screen
    noecho();

    // Enable special keyboard input (e.g., arrow keys)
    keypad(stdscr, TRUE);

    // Hide the cursor
    curs_set(0);

    // Make getch a non-blocking call
    nodelay(stdscr, TRUE);

    // Print a message
    printw("Press arrow keys to move the spaceship. Press spacebar to fire bullets. Press q to quit.");

    for (int i = 0; i < MAX_Rocket; i++)
    {
        if (!rockets[i].active)
        {
            rockets[i].x = rand() % (COLS - 5) + 1;
            rockets[i].y = 1;
            rockets[i].active = 1;
        }
    }

    // Initialize the spaceship position
    int x = (COLS - 7) / 2;                // Starting position for spaceship
    int y = LINES - SPACE_SHIP_HEIGHT - 1; // Adjusted starting position for the spaceship

    while (1)
    {
        // Get keyboard input without waiting for Enter
        int ch = getch();

        // Clear the screen
        clear();

        // Handle arrow key presses to move the spaceship
        switch (ch)
        {
        case 'q':
            // Quit the game
            endwin();
            return;
            break;
        case KEY_LEFT:
            if (x - 1 > 0)
                x--;
            break;
        case KEY_RIGHT:
            if (x + 1 < COLS - 7)
                x++;
            break;
        case ' ':
            // Find an inactive bullet slot and activate it
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active)
                {
                    bullets[i].x = x + 3; // Adjust bullet position to appear from spaceship's center
                    bullets[i].y = y - 1;
                    bullets[i].active = 1;
                    break;
                }
            }
            break;
        default:
            break;
        }

        // Move all active bullets
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                moveBullet(i);
            }
        }

        // Draw and move the rocket
        for (int i = 0; i < MAX_Rocket; i++)
        {
            if (rockets[i].active)
            {
                drawRocket(rockets[i].x, rockets[i].y);
                moveRocket(i);
            }
        }

        // Check for collision between bullets and rockets
        checkCollision();

        // Increment the rocket move counter
        rocketMoveCounter++;

        // Reset the counter to prevent overflow
        if (rocketMoveCounter >= ROCKET_MOVE_DELAY)
            rocketMoveCounter = 0;

        // Draw the spaceship
        drawSpaceship(x, y);

        // Draw boundaries
        drawBoundaries();

        // Refresh the screen
        refresh();

        // Sleep for a short time to control the game loop speed
        usleep(50000); // 50 milliseconds (adjust as needed)
    }

    // End curses mode
    endwin();
}
