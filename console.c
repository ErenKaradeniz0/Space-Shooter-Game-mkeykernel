#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> // Include for usleep

#define SPACE_SHIP_HEIGHT 4

#define BULLET '^'
#define BULLET_SPEED 1
#define MAX_BULLETS 10000 // Maximum number of bullets

#define ALIEN_SPEED 1
#define MAX_Alien 15

#define ALIEN_MOVE_DELAY 20 // Adjust this value to control alien movement speed

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
    int active; // Flag to indicate if the alien is active or not
} Alien;

Bullet bullets[MAX_BULLETS]; // Array to hold multiple bullets
Alien aliens[MAX_Alien];     // Array to hold multiple alien

int alienMoveCounter = 0; // Counter to control alien movement speed

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

// Function to draw the alien
void drawAlien(int x, int y)
{
    mvprintw(y, x, "\\ || / ");
    mvprintw(y + 1, x, " |oo| ");
    mvprintw(y + 2, x, " |oo| ");
    mvprintw(y + 3, x, "  \\/ ");
    mvprintw(y + 4, x, "  ");
}

// Function to move the alien
void moveAlien(int index)
{
    if (alienMoveCounter % ALIEN_MOVE_DELAY == 0)
    {                                                      // Move the alien every ALIEN_MOVE_DELAY frames
        mvprintw(aliens[index].y, aliens[index].x, "   "); // Clear previous alien position
        aliens[index].y += ALIEN_SPEED;                    // Move the alien downwards
    }
}

// Function to check for collision between bullet and alien
void checkCollision()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_Alien; j++)
            {
                if (aliens[j].active &&
                    bullets[i].x >= aliens[j].x && bullets[i].x < aliens[j].x + 7 &&
                    bullets[i].y >= aliens[j].y && bullets[i].y < aliens[j].y + 5)
                {
                    bullets[i].active = 0; // Deactivate bullet
                    aliens[j].active = 0; // Deactivate alien
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

    for (int i = 0; i < MAX_Alien; i++)
    {
        if (!aliens[i].active)
        {
            aliens[i].x = rand() % (COLS - 5) + 1;
            aliens[i].y = 1;
            aliens[i].active = 1;
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

        // Draw and move the alien
        for (int i = 0; i < MAX_Alien; i++)
        {
            if (aliens[i].active)
            {
                drawAlien(aliens[i].x, aliens[i].y);
                moveAlien(i);
            }
        }

        // Check for collision between bullets and aliens
        checkCollision();

        // Increment the alien move counter
        alienMoveCounter++;

        // Reset the counter to prevent overflow
        if (alienMoveCounter >= ALIEN_MOVE_DELAY)
            alienMoveCounter = 0;

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
