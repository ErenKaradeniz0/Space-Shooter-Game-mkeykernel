/*
 * Copyright (C) 2014  Arjun Sreedharan
 * License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
 */
#include "keyboard_map.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT *COLUMNS_IN_LINE *LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

#define SIDE_BAR_WIDTH 20
#define ROCKET_WIDTH 4
#define ROCKET_HEIGHT 3

#define SPACE_SHIP_HEIGHT 4
#define SPACE_SHIP_WIDTH 6

#define BULLET_SPEED 1
#define MAX_BULLETS 30

#define ROCKET_SPEED 1
#define MAX_ROCKETS 4
#define ROCKET_MOVE_DELAY 14
#define BULLET_MOVE_DELAY 2

#define RAND_MAX 80

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
extern void sleep(int milliseconds);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char *)0xb8000;

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

int flag = 0; // flag 1 when key pressed
int pause_flag = 0;
char current_key = '1';
int bullet_count = MAX_BULLETS;
int x;
int y;
int score = 0;
char score_str[3];   // maximum number of digits is 3
char bullets_str[2]; // maximum number of digits is 2

struct IDT_entry
{
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void idt_init(void)
{
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    // Cursor Control Registers for VGA text mode
    unsigned char cursor_start_register = 0x0A;
    unsigned char cursor_end_register = 0x0B;

    // Write appropriate values to disable cursor blinking
    write_port(0x3D4, cursor_start_register); // Select start register
    write_port(0x3D5, 0x20);                  // Disable cursor blinking by setting bit 5

    write_port(0x3D4, cursor_end_register); // Select end register
    write_port(0x3D5, 0x00);                // Setting end register to 0 disables cursor

    /* populate IDT entry of keyboard's interrupt */
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /*     Ports
     *	 PIC1	PIC2
     *Command 0x20	0xA0
     *Data	 0x21	0xA1
     */

    /* ICW1 - begin initialization */
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);

    /* ICW2 - remap offset address of IDT */
    /*
     * In x86 protected mode, we have to remap the PICs beyond 0x20 because
     * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
     */
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);

    /* ICW3 - setup cascading */
    write_port(0x21, 0x00);
    write_port(0xA1, 0x00);

    /* ICW4 - environment info */
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);
    /* Initialization finished */

    /* mask interrupts */
    write_port(0x21, 0xff);
    write_port(0xA1, 0xff);

    /* fill the IDT descriptor */
    idt_address = (unsigned long)IDT;
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16;

    load_idt(idt_ptr);
}

void kb_init(void)
{
    /* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
    write_port(0x21, 0xFD);
}

// void kprint(const char *str)
// {
//     unsigned int i = 0;
//     while (str[i] != '\0')
//     {
//         vidptr[current_loc++] = str[i++];
//         vidptr[current_loc++] = 0x07;
//     }
// }

// void kprint_newline(void)
// {
//     unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
//     current_loc = current_loc + (line_size - current_loc % (line_size));
// }

void kprint_at(int x, int y, const char *str)
{
    // Check if coordinates are within valid range
    if (x < 0 || x >= COLUMNS_IN_LINE || y < 0 || y >= LINES)
    {
        return; // Handle invalid coordinates (optional)
    }

    // Calculate absolute index in video memory buffer
    int index = BYTES_FOR_EACH_ELEMENT * (y * COLUMNS_IN_LINE + x);

    // Ensure pointer access is safe (adjust base address if needed)
    char *ptr = (char *)0xb8000 + index;

    while (*str != '\0')
    {
        // Print character
        *ptr++ = *str++;

        // Print attribute byte (adjust if needed)
        *ptr++ = 0x07; // Assuming white text on black background

        // Handle potential wrapping at the end of a line
        if (x == COLUMNS_IN_LINE - 1)
        {
            // Move to the beginning of the next line
            x = 0;
            y++;

            // Check if y is within valid range
            if (y >= LINES)
            {
                // Handle bottom of screen reached (optional)
                break;
            }

            // Update index for the next line
            index = BYTES_FOR_EACH_ELEMENT * (y * COLUMNS_IN_LINE + x);
            ptr = (char *)0xb8000 + index;
        }
        else
        {
            // Increment x for next character within the same line
            x++;
        }
    }

    // Update global cursor position (optional)
    // ... (implementation details based on your kernel's cursor logic)
}

void clear_screen(void)
{
    unsigned int i = 0;
    while (i < SCREENSIZE)
    {
        vidptr[i++] = ' ';
        vidptr[i++] = 0x07;
    }
}

void keyboard_handler_main(void)
{
    unsigned char status;
    char keycode;

    /* write EOI */
    write_port(0x20, 0x20);

    status = read_port(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01)
    {
        keycode = read_port(KEYBOARD_DATA_PORT);
        if (keycode < 0)
            return;
        current_key = (char)keyboard_map[(unsigned char)keycode];
        flag = 1;
        // if(keycode == ENTER_KEY_CODE) {
        // 	kprint_newline();
        // 	return;
        // }

        // vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
        // vidptr[current_loc++] = 0x07;
    }
}

void drawBoundaries()
{
    for (int i = 0; i < LINES; i++)
    {
        kprint_at(0, i, "#");
        kprint_at(SIDE_BAR_WIDTH, i, "#");
        kprint_at(COLUMNS_IN_LINE - 1, i, "#");
    }

    for (int i = 0; i < COLUMNS_IN_LINE; i++)
    {
        kprint_at(i, 0, "#");
        kprint_at(i, LINES - 1, "#");
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
    kprint_at(x, y, score_str);
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
    kprint_at(x, y, bullets_str);
    if (bullet_count < 10)
        kprint_at(x + 1, y, " ");
}

void info()
{
    // Display the welcome message and instructions
    kprint_at(2, 1, "Welcome!");
    kprint_at(2, 2, "Save the World!");
    kprint_at(2, 3, "by Eren Karadeniz");
    kprint_at(2, 4, "200101070");

    kprint_at(2, 6, "Keys");
    kprint_at(2, 7, "A to move left");
    kprint_at(2, 8, "D to move right");
    kprint_at(2, 9, "Space to Shot");
    kprint_at(2, 10, "Q to quit game");
    kprint_at(2, 11, "R to restart game");
    kprint_at(2, 12, "P to pause game");
    kprint_at(2, 14, "Win after reach");
    kprint_at(2, 15, "25 Score");
}

void winGame()
{
    clear_screen();
    drawBoundaries();
    info();
    kprint_at(44, 12, "You Win");
    kprint_at(37, 13, "Press R for Play Again");
}

void intro()
{
    // Clear the screen and draw boundaries
    clear_screen();
    drawBoundaries();

    info();

    kprint_at(2, 17, "Bullets:");
    printBulletCount(11, 17);

    kprint_at(2, 18, "Score:");
    printScore(10, 18);
}

void drawSpaceship()
{
    kprint_at(x, y, "A  I  A");
    kprint_at(x, y + 1, "A /-\\ A");
    kprint_at(x, y + 2, "\\  U  /");
    kprint_at(x, y + 3, "/o o o\\");
}

void clearSpaceship()
{
    // Clear the old position of the spaceship
    kprint_at(x, y, "       ");
    kprint_at(x, y + 1, "       ");
    kprint_at(x, y + 2, "       ");
    kprint_at(x, y + 3, "       ");
}

void drawRocket(int x, int y)
{
    kprint_at(x, y, "\\||/");
    kprint_at(x, y + 1, "|oo|");
    kprint_at(x, y + 2, " \\/");
}

void clearRocket(int x, int y)
{
    kprint_at(x, y, "    ");
    kprint_at(x, y + 1, "    ");
    kprint_at(x, y + 2, "   ");
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
        kprint_at(bullets[index].x, bullets[index].y, " "); // Clear previous bullet position
        bullets[index].y -= BULLET_SPEED;                   // Move the bullet upwards
        if (bullets[index].y > 0)
        {
            kprint_at(bullets[index].x, bullets[index].y, "^"); // Draw the bullet
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

// Define a function to wait for a specified number of milliseconds
void busy_wait(unsigned int milliseconds)
{
    // Calculate the number of iterations needed for the desired milliseconds
    unsigned int iterations = milliseconds * 10000; // Adjust this value as needed based on your system's clock speed

    // Execute an empty loop for the specified number of iterations
    for (unsigned int i = 0; i < iterations; ++i)
    {
        // Do nothing, just wait
    }
}

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
                    kprint_at(bullets[i].x, bullets[i].y, " ");
                    clearRocket(rockets[j].x, rockets[j].y);
                    break;
                }
            }
        }
    }
}

void gameOver()
{
    clear_screen();
    drawBoundaries();
    info();
    kprint_at(35, 12, "You lost, Press R for Play Again");
    kprint_at(46, 13, "Score: ");
    kprint_at(54, 13, score_str);
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
            kprint_at(36, 11, "Spaceship destroyed by rocket");
        }
    }
}

void quitGame()
{
    clear_screen();
    drawBoundaries();
    info();
    kprint_at(35, 12, "Press R for Play Again");
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
    drawBoundaries();

    x = (COLUMNS_IN_LINE - SPACE_SHIP_WIDTH + SIDE_BAR_WIDTH) / 2; // Starting position for spaceship
    y = LINES - SPACE_SHIP_HEIGHT - 1;                             // Adjusted starting position for the spaceship
}

void restartGame()
{
    clear_screen(); // Clear the screen
    init();         // Initialize the game
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
                kprint_at(35, 10, "Paused, Press p to continue");
            }
            break;
        }
        flag = 0;
    }
    else
    {
        if (current_key == 'p')
        {
            pause_flag = 0;
            kprint_at(35, 10, "                                 ");
            flag = 0;
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
                kprint_at(bullets[index].x, bullets[index].y, "^");
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

void kmain(void)
{
    idt_init();
    kb_init();
    init();

    // game loop
    while (1)
    {
        while (quit_flag == 0 && continueGame())
        {
            if (flag)
            {
                handleUserInput(current_key, bullets);
                if (current_key == 'q')
                {
                    break;
                }
            }
            drawSpaceship(x, y);
            move_bullets();
            move_rockets();
            // Check for collision between bullets and rockets
            collisionBullet();
            collisionSpaceShip();

            busy_wait(1000); // Wait for 50 milliseconds using busy wait
        }

        if (current_key == 'r')
        {
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(); // Restart the game
        }
    }

    while (1)
        ;
}