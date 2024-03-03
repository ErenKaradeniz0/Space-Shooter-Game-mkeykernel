/*
* Copyright (C) 2014  Arjun Sreedharan
* License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
*/
#include "keyboard_map.h"
// #include "clock.c"
/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C


int x = 1;
int y = 1;
int test_x=1;
int flag = 0;
char current_key = '1';

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
extern void sleep(int milliseconds);


/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

struct IDT_entry {
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
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = 0x07;
	}
}


void kprint_at(int x, int y, const char *str) {
    // Check if coordinates are within valid range
    if (x < 0 || x >= COLUMNS_IN_LINE || y < 0 || y >= LINES) {
        return;  // Handle invalid coordinates (optional)
    }

    // Calculate absolute index in video memory buffer
    int index = BYTES_FOR_EACH_ELEMENT * (y * COLUMNS_IN_LINE + x);

    // Ensure pointer access is safe (adjust base address if needed)
    char *ptr = (char *)0xb8000 + index;

    while (*str != '\0') {
        // Print character
        *ptr++ = *str++;

        // Print attribute byte (adjust if needed)
        *ptr++ = 0x07; // Assuming white text on black background

        // Handle potential wrapping at the end of a line
        if (x == COLUMNS_IN_LINE - 1) {
            // Move to the beginning of the next line
            x = 0;
            y++;

            // Check if y is within valid range
            if (y >= LINES) {
                // Handle bottom of screen reached (optional)
                break;
            }

            // Update index for the next line
            index = BYTES_FOR_EACH_ELEMENT * (y * COLUMNS_IN_LINE + x);
            ptr = (char *)0xb8000 + index;
        } else {
            // Increment x for next character within the same line
            x++;
        }
    }

    // Update global cursor position (optional)
    // ... (implementation details based on your kernel's cursor logic)
}


void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
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
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;
        current_key = (char) keyboard_map[(unsigned char) keycode];
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
    for (int i = 0; i < COLUMNS_IN_LINE; i++)
    {
        kprint_at(0, i, "#");
        kprint_at(COLUMNS_IN_LINE - 1, i, "#");
    }

    for (int i = 0; i < COLUMNS_IN_LINE; i++)
    {
        kprint_at(i, 0, "#");
        kprint_at(i, LINES - 1, "#");
    }
}


void kmain(void)
{
    int counter_A = 0;
    clear_screen();
    kprint_at(x, y, "welcome wasd to move x press q to quit");
    idt_init();
	kb_init();
    clear_screen();
    drawBoundaries();
    while(1){
        if(counter_A % 10000000 == 0)
        {
                kprint_at(test_x, y, " ");
                if(flag){
                    kprint_at(x, y, " ");
                    switch (current_key)
                    {
                        case 'w':
                        y--;
                        break;
                        case 'a':
                        x--;
                        break;
                        case 's':
                        y++;
                        break;
                        case 'd':
                        x++;
                        break;
                        case 'q':
                        break;
                    }
                flag = 0;
                }
                test_x++;
                drawBoundaries();
                kprint_at(test_x, y, "o");
                kprint_at(x, y, "X");
                counter_A = 1;
        }
        counter_A++;
        if(current_key == 'q'){
        clear_screen();
        kprint_at(x, y, "The End");
        break;
        }
        
    }

	while(1);
}
