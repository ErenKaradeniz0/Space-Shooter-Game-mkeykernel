#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h> // Include for usleep

#define LINES 25
#define COLUMNS_IN_LINE 80

#define SIDE_BAR_WIDTH 20
#define ROCKET_WIDTH 4
#define ROCKET_HEIGHT 3

#define SPACE_SHIP_HEIGHT 4
#define SPACE_SHIP_WIDTH 6

#define BULLET_SPEED 1
#define MAX_BULLETS 30

#define ROCKET_SPEED 1
#define MAX_ROCKETS 4
#define ROCKET_MOVE_DELAY 48
#define BULLET_MOVE_DELAY 8

#define PRINT_ROCKETLEFT_X 10
#define PRINT_ROCKETLEFT_Y 15

int quit_flag = 0;

typedef struct
{
    int x;
    int y;
    int active;  // Flag to bullet is active or not
    int avaible; // Flag to bullet is avaible to shot
} Bullet;

typedef struct
{
    int x;
    int y;
    int active; // Flag to rocket is active or not
} Rocket;

Bullet bullets[MAX_BULLETS];
Rocket rockets[MAX_ROCKETS];

int rocketMoveCounter = 0; // Counter to control rocket movement speed
int bulletMoveCounter = 0; // Counter to control bullet movement speed

int pause_flag = 0;
int bullet_count = MAX_BULLETS;
int x;
int y;
int score = 0;
char score_str[3];   // maximum number of digits is 3
char bullets_str[2]; // maximum number of digits is 2

void drawBoundaries()
{
    for (int i = 0; i < COLUMNS_IN_LINE; i++)
    {
        mvprintw(i, 0, "#");              // Swap parameters
        mvprintw(i, SIDE_BAR_WIDTH, "#"); // Swap parameters
        mvprintw(i, COLUMNS_IN_LINE - 1, "#");
    }

    for (int i = 0; i < COLUMNS_IN_LINE; i++)
    {
        mvprintw(0, i, "#");
        mvprintw(LINES - 1, i, "#");
    }
}

// Function to convert an integer to its string representation
int int_to_string(int num, char *buffer)
{
    int i = 0;
    int digits = 0; // Variable to store the number of digits

    if (num == 0)
    {
        buffer[i++] = '0';
        digits = 1; // If the number is zero, it has one digit
    }
    else
    {
        // Calculate the number of digits
        int temp = num;
        while (temp != 0)
        {
            digits++;
            temp /= 10;
        }

        // Convert each digit to character and store in the buffer
        while (num != 0)
        {
            int digit = num % 10;
            buffer[i++] = '0' + digit;
            num /= 10;
        }
    }
    buffer[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    return digits; // Return the number of digits
}

void printScore(int x, int y)
{
    int num_digits = int_to_string(score, score_str);
    mvprintw(y, x, "%s", score_str);
}

void bullet_counter()
{
    bullet_count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].avaible)
        {
            bullet_count += 1;
        }
    }
}

void printBulletCount(int x, int y)
{

    int num_digits = int_to_string(bullet_count, bullets_str);
    mvprintw(y, x, "%s", bullets_str);
    if (bullet_count < 10)
        mvprintw(y, x + 1, " ");
}

void info()
{
    // Display the welcome message and instructions
    mvprintw(1, 2, "Welcome!");          // Swap parameters
    mvprintw(2, 2, "Save the World!");   // Swap parameters
    mvprintw(3, 2, "by Eren Karadeniz"); // Swap parameters
    mvprintw(4, 2, "200101070");         // Swap parameters

    mvprintw(6, 2, "Keys");               // Swap parameters
    mvprintw(7, 2, "A to move left");     // Swap parameters
    mvprintw(8, 2, "D to move right");    // Swap parameters
    mvprintw(9, 2, "Space to Shot");      // Swap parameters
    mvprintw(10, 2, "Q to quit game");    // Swap parameters
    mvprintw(11, 2, "R to restart game"); // Swap parameters
    mvprintw(12, 2, "P to pause game");
    mvprintw(14, 2, "Win after reach"); // Swap parameters
    mvprintw(15, 2, "25 Score");        // Swap parameters
}

void winGame()
{
    clear();
    drawBoundaries();
    info();
    mvprintw(12, 44, "You Win");
    mvprintw(13, 37, "Press R for Play Again");
}

void intro()
{
    // Clear the screen and draw boundaries
    clear();
    drawBoundaries();

    info();

    mvprintw(17, 2, "Bullets:");
    printBulletCount(11, 17);

    mvprintw(18, 2, "Score:");
    printScore(10, 18);
}

void drawSpaceship()
{
    mvprintw(y, x, "A  I  A");
    mvprintw(y + 1, x, "A /-\\ A");
    mvprintw(y + 2, x, "\\  U  /");
    mvprintw(y + 3, x, "/o o o\\");
}

void clearSpaceship()
{
    // Clear the old position of the spaceship
    mvprintw(y, x, "       ");
    mvprintw(y + 1, x, "       ");
    mvprintw(y + 2, x, "       ");
    mvprintw(y + 3, x, "       ");
}

void drawRocket(int x, int y)
{
    mvprintw(y, x, "\\||/");
    mvprintw(y + 1, x, "|oo|");
    mvprintw(y + 2, x, " \\/");
}

void clearRocket(int x, int y)
{
    mvprintw(y, x, "    ");
    mvprintw(y + 1, x, "    ");
    mvprintw(y + 2, x, "   ");
}

void moveRocket(int index)
{
    if (rocketMoveCounter % ROCKET_MOVE_DELAY == 0)
    {                                                    // Move the rocket every ROCKET_MOVE_DELAY frames
        clearRocket(rockets[index].x, rockets[index].y); // Clear previous rocket position
        rockets[index].y += ROCKET_SPEED;                // Move the rocket downwards
        drawRocket(rockets[index].x, rockets[index].y);
    }
}

void moveBullet(int index)
{
    if (bulletMoveCounter % BULLET_MOVE_DELAY == 0)
    {
        mvprintw(bullets[index].y, bullets[index].x, " "); // Clear previous bullet position
        bullets[index].y -= BULLET_SPEED;                  // Move the bullet upwards
        if (bullets[index].y > 0)
        {
            mvprintw(bullets[index].y, bullets[index].x, "^"); // Draw the bullet
        }
        else
            bullets[index].active = 0;
    }
}

unsigned int get_system_timer_value()
{
    unsigned int val;
    // Read the value of the system timer (assuming x86 architecture)
    asm volatile("rdtsc" : "=a"(val));
    return val;
}

// Define some global variables for the random number generator
static unsigned long next;
// A function to generate a pseudo-random integer
int rand(void)
{
    next = get_system_timer_value();
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % RAND_MAX;
}
// Function to busy-wait for a specified number of milliseconds

void initBullets()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].x = 1;
        bullets[i].y = 1;
        bullets[i].active = 0;
        bullets[i].avaible = 1;
    }
}

int randRocketAxis()
{
    int min_x = SIDE_BAR_WIDTH + 1;                 // 21
    int max_x = COLUMNS_IN_LINE - ROCKET_WIDTH - 1; // 73
    int x = rand();
    while (min_x > x || x > max_x)
    {
        x = rand();
    }
    return x;
}

void initRockets()
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        int newRocketX, newRocketY;
        int collisionDetected;

        do
        {
            // Generate random position for the new rocket
            newRocketX = randRocketAxis(); // Adjust range to prevent overflow
            newRocketY = 1;                // Adjust range as needed

            // Check for collision with existing rockets based on X position only
            collisionDetected = 0;
            for (int j = 0; j < i; j++)
            {
                if (rockets[j].active &&
                    (newRocketX >= rockets[j].x - ROCKET_WIDTH && newRocketX <= rockets[j].x + ROCKET_WIDTH)) // Check only X position
                {
                    collisionDetected = 1;
                    i = 0;
                    break;
                }
            }
        } while (collisionDetected);

        // Set the position of the new rocket
        rockets[i].x = newRocketX;
        rockets[i].y = newRocketY;
        rockets[i].active = 1;
    }
}

// Function to check for collision between bullet and rocket
int collisionBullet()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_ROCKETS; j++)
            {
                if (rockets[j].active &&
                    bullets[i].x >= rockets[j].x && bullets[i].x < rockets[j].x + 7 &&
                    bullets[i].y >= rockets[j].y && bullets[i].y < rockets[j].y + 5)
                {
                    score += 1;

                    printScore(10, 18);
                    bullets[i].active = 0; // Deactivate bullet
                    rockets[j].active = 0; // Deactivate rocket
                    mvprintw(bullets[i].y, bullets[i].x, " ");
                    clearRocket(rockets[j].x, rockets[j].y);
                    break;
                }
            }
        }
    }
}

void gameOver()
{
    clear();
    drawBoundaries();
    info();
    mvprintw(12, 35, "You lost, Press R for Play Again");
    mvprintw(13, 46, "Score: ");
    mvprintw(13, 54, "%s", score_str);
}

// Function to check for collision between rocket and spaceship
void collisionSpaceShip()
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        // Check if any of the edges of the rocket box lie outside the spaceship box

        if (x <= rockets[i].x + ROCKET_WIDTH - 1 && x + SPACE_SHIP_WIDTH - 1 >= rockets[i].x && rockets[i].y + ROCKET_HEIGHT >= y)
        {
            quit_flag = 1;
            gameOver();
            mvprintw(11, 36, "Spaceship destroyed by rocket");
        }
    }
}

void quitGame()
{
    clear();
    drawBoundaries();
    info();
    mvprintw(12, 35, "Press R for Play Again");
}

void shot_bullet(Bullet *bullet)
{
    bullet->active = 1;
    bullet->avaible = 0;
    bullet->x = x + 3; // Adjust bullet position to appear from spaceship's center
    bullet->y = y - 1;
}

void init()
{

    initBullets();
    initRockets();
    intro();

    x = (COLUMNS_IN_LINE - SPACE_SHIP_WIDTH + SIDE_BAR_WIDTH) / 2; // Starting position for spaceship
    y = LINES - SPACE_SHIP_HEIGHT - 2;                             // Adjusted starting position for the spaceship
}

void restartGame()
{

    init(); // Initialize the game
}

void handleUserInput(char current_key, Bullet bullets[MAX_BULLETS])
{
    if (!pause_flag)
    {

        switch (current_key)
        {
        case 'a':
            if (x - 1 > SIDE_BAR_WIDTH)
            {
                clearSpaceship(x, y);
                (x)--;
            }
            break;
        case 'd':
            if (x + 1 < COLUMNS_IN_LINE - 7)
            {
                clearSpaceship(x, y);
                (x)++;
            }
            break;
        case ' ':
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active && bullets[i].avaible)
                {
                    shot_bullet(&bullets[i]);
                    bullet_counter();
                    printBulletCount(11, 17);
                    break;
                }
            }
            break;
        case 'q':
            score = 0;
            quitGame();
            bullet_count = MAX_BULLETS;
            quit_flag = 1;
            break;
        case 'r':
            score = 0;
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(); // Restart the game
            break;
        case 'p':
            pause_flag = !pause_flag; // Toggle pause_flag
            if (pause_flag)
            {
                mvprintw(10, 35, "Paused, Press p to continue");
                usleep(50000);
            }
            break;
        }
    }
    else
    {
        if (current_key == 'p')
        {
            pause_flag = 0;
            mvprintw(10, 35, "                                 ");
        }
    }
}

void move_bullets()
{

    // Move all active bullets
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        if (!pause_flag)
        {
            if (bullets[index].active && !bullets[index].avaible)
            {
                mvprintw(bullets[index].y, bullets[index].x, "^");
                moveBullet(index);
            }
        }
    }
    // Increment the bullet move counter
    bulletMoveCounter++;
    // Reset the counter to prevent overflow
    if (bulletMoveCounter >= BULLET_MOVE_DELAY)
        bulletMoveCounter = 0;
}

// Function to generate a single rocket from passive rocket
void generateRocket(Rocket *rocket)
{
    int newRocketX, newRocketY;
    int collisionDetected;

    do
    {
        // Generate random position for the new rocket
        newRocketX = randRocketAxis(); // Adjust range to prevent overflow
        newRocketY = 1;                // Adjust range as needed

        // Check for collision with existing rockets based on X position only
        collisionDetected = 0;
        for (int j = 0; j < MAX_ROCKETS; j++)
        {
            if (rockets[j].active &&
                (newRocketX >= rockets[j].x - ROCKET_WIDTH && newRocketX <= rockets[j].x + ROCKET_WIDTH)) // Check only X position
            {
                collisionDetected = 1;
                break;
            }
        }
    } while (collisionDetected);

    // Set the position of the new rocket
    rocket->x = newRocketX;
    rocket->y = newRocketY;
    rocket->active = 1;
}

void generate_rockets()
{
    // Generate new rockets if there are inactive rockets
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (!rockets[i].active)
        {
            generateRocket(&rockets[i]);
        }
    }
}

void move_rockets()
{
    // Draw and move the rocket
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (!pause_flag)
        {
            if (rockets[i].active)
            {
                drawRocket(rockets[i].x, rockets[i].y);
                moveRocket(i);
            }
        }
    }

    // Increment the rocket move counter
    rocketMoveCounter++;
    // Reset the counter to prevent overflow
    if (rocketMoveCounter >= ROCKET_MOVE_DELAY)
        rocketMoveCounter = 0;

    // Get keyboard input without waiting for Enter
    char current_key = getch();
    // Make getch a non-blocking call
    nodelay(stdscr, TRUE);
    if (current_key != 'p')
    {
        generate_rockets();
    }
}

int continueGame()
{
    // Check if all rockets have reached the bottom of the screen

    int rocketReachedBottom = 0;
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (rockets[i].y >= LINES)
        {
            rocketReachedBottom = 1;
            if (rocketReachedBottom)
            {
                quit_flag = 1;
                gameOver();
                return 0;
            }
        }
    }

    if (score == 25)
    {
        quit_flag = 1;
        winGame();
        score = 0;
        return 0;
    }

    return 1;
}

void main(void)
{
    // Initialize the curses library
    initscr();

    resize_term(25, 80);

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

    init();

    // game loop
    while (1)
    {
        // Get keyboard input without waiting for Enter
        char current_key = getch();
        // Make getch a non-blocking call
        nodelay(stdscr, TRUE);
        while (quit_flag == 0 && continueGame())
        {
            // Get keyboard input without waiting for Enter
            current_key = getch();
            // Make getch a non-blocking call
            nodelay(stdscr, TRUE);

            handleUserInput(current_key, bullets);
            if (current_key == 'q')
            {
                break;
            }

            drawSpaceship(x, y);
            move_bullets();
            move_rockets();
            // Check for collision between bullets and rockets
            collisionBullet();
            collisionSpaceShip();
            refresh();
            napms(10); // Introduce a small delay
        }

        if (current_key == 'r')
        {
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(); // Restart the game
        }
    }

    // End curses mode
    endwin();
}