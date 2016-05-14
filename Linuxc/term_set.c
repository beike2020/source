/******************************************************************************
 * Function: 	Term set.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall term_set.c -I/usr/include/ncurses -lncurses -o term_set
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>

#define  PASSWORD_LEN 	 	8
#define  LOCAL_ESCAPE_KEY	27

static FILE *output_stream = (FILE *) 0;
static struct termios initial_settings, new_settings;

char *menu[] = {
	"a - add new record",
	"d - delete record",
	"q - quit",
	NULL,
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test the size of term\n");
	printf(" 2: test the style of term\n");
	printf(" 3: test the color of term\n");
	printf(" 4: test the choice menu use term\n");
	printf(" 5: test the echo function of term\n");
	printf(" 6: test the keyboard capture of term\n");
	printf(" 7: test the muti_windows of term\n");
	printf(" 8: test the sub_windows of term\n");
	printf(" 9: test the size is biger than a normal term\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int show_term_size()
{
	int nrows, ncolumns;

	setupterm(NULL, fileno(stdout), (int *)0);
	nrows = tigetnum("lines");
	ncolumns = tigetnum("cols");
	printf("Here %d columns and %d rows\n", ncolumns, nrows);

	return 0;
}

int show_term_color()
{
	int i;

	initscr();

	if (!has_colors()) {
		endwin();
		printf("No color support on this terminal\n");
		exit(EXIT_FAILURE);
	}

	if (start_color() != OK) {
		endwin();
		printf("Could not initialize colors\n");
		exit(EXIT_FAILURE);
	}

	clear();
	mvprintw(5, 5, "Here %d COLORS, and %d PAIRS", COLORS, COLOR_PAIRS);
	refresh();

	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_GREEN);
	init_pair(3, COLOR_GREEN, COLOR_RED);
	init_pair(4, COLOR_YELLOW, COLOR_BLUE);
	init_pair(5, COLOR_BLACK, COLOR_WHITE);
	init_pair(6, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(7, COLOR_CYAN, COLOR_WHITE);

	for (i = 1; i <= 7; i++) {
		attroff(A_BOLD);
		attrset(COLOR_PAIR(i));
		mvprintw(5 + i, 5, "Color pair %d", i);
		attrset(COLOR_PAIR(i) | A_BOLD);
		mvprintw(5 + i, 25, "Bold color pair %d", i);
		refresh();
		sleep(1);
	}

	endwin();

	return 0;
}

int show_term_style()
{
	const char *scan_ptr;
	const char witch_one[] = " First Witch  ";

	initscr();

	move(5, 15);
	attron(A_BOLD);
	printw("%s", "Macbeth");
	attroff(A_BOLD);
	refresh();
	sleep(1);

	move(7, 10);
	printw("%s", "When shall we three meet again");
	move(8, 23);
	printw("%s", "In thunder, lightning, or in rain ?");
	refresh();
	sleep(1);

	attron(A_DIM);
	scan_ptr = witch_one + strlen(witch_one) - 1;
	while (scan_ptr != witch_one) {
		move(7, 10);
		insch(*scan_ptr--);
	}

	attroff(A_DIM);
	refresh();
	sleep(1);

	move(LINES - 1, COLS - 1);
	refresh();
	sleep(1);
	endwin();

	return 0;
}

int char_to_term(int to_write)
{
	if (output_stream)
		putc(to_write, output_stream);

	return 0;
}

int get_choice(char *greet, char *choices[], FILE * in, FILE * out)
{
	int selected, chosen = 0;
	int screenrow, screencol = 10;
	char *cursor, *clear, **option;

	output_stream = out;
	setupterm(NULL, fileno(out), (int *)0);

	cursor = tigetstr("cup");
	clear = tigetstr("clear");

	screenrow = 4;
	tputs(clear, 1, char_to_term);
	tputs(tparm(cursor, screenrow, screencol), 1, char_to_term);
	fprintf(out, "Choice: %s", greet);
	screenrow += 2;
	option = choices;

	while (*option) {
		tputs(tparm(cursor, screenrow, screencol), 1, char_to_term);
		fprintf(out, "%s", *option);
		screenrow++;
		option++;
	}
	fprintf(out, "\n");

	do {
		fflush(out);
		selected = fgetc(in);
		option = choices;
		while (*option) {
			if (selected == *option[0]) {
				chosen = 1;
				break;
			}
			option++;
		}

		if (chosen == 0) {
			tputs(tparm(cursor, screenrow, screencol), 1,
			      char_to_term);
			fprintf(out, "Incorrect choice, select again\n");
		}
	} while (!chosen);

	tputs(clear, 1, char_to_term);

	return selected;
}

int read_term_choice()
{
	int choice = 0;
	FILE *input, *output;

	if (!isatty(fileno(stdout)))
		printf("You are not a terminal, OK.\n");

	input = fopen("/dev/tty", "r");
	output = fopen("/dev/tty", "w");
	if (input == NULL || output == NULL) {
		perror("/dev/tty\n");
		exit(EXIT_FAILURE);
	}

	tcgetattr(fileno(input), &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_lflag &= ~ISIG;

	if (tcsetattr(fileno(input), TCSANOW, &new_settings) != 0)
		fprintf(stderr, "could not set attributes\n");

	do {
		choice = get_choice("Select an action", menu, input, output);
		printf("You have chosen: %c\n", choice);
		sleep(1);
	} while (choice != 'q');

	tcsetattr(fileno(input), TCSANOW, &initial_settings);

	return 0;
}

int show_term_echo()
{
	char password[PASSWORD_LEN + 1];
	struct termios initialrsettings, newrsettings;

	tcgetattr(fileno(stdin), &initialrsettings);
	newrsettings = initialrsettings;
	newrsettings.c_lflag &= ~ECHO;

	printf("Enter password: ");
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &newrsettings) != 0) {
		printf("Could not set attributes\n");
	} else {
		fgets(password, PASSWORD_LEN, stdin);
		tcsetattr(fileno(stdin), TCSANOW, &initialrsettings);
		fprintf(stdout, "\nYou entered %s\n", password);
	}

	return 0;
}

int show_term_keyboard()
{
	int key;

	initscr();
	crmode();
	keypad(stdscr, TRUE);

	noecho();
	clear();
	mvprintw(5, 5, "Key pad demonstration. Press 'q' to quit");
	move(7, 5);
	refresh();

	key = getch();
	while (key != ERR && key != 'q') {
		move(7, 5);
		clrtoeol();

		if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
			printw("Key was %c", (char)key);
		} else {
			switch (key) {
			case LOCAL_ESCAPE_KEY:
				printw("%s", "Escape key");
				break;

			case KEY_END:
				printw("%s", "END key");
				break;

			case KEY_BEG:
				printw("%s", "BEGINNING key");
				break;

			case KEY_RIGHT:
				printw("%s", "RIGHT key");
				break;

			case KEY_LEFT:
				printw("%s", "LEFT key");
				break;

			case KEY_UP:
				printw("%s", "UP key");
				break;

			case KEY_DOWN:
				printw("%s", "DOWN key");
				break;

			default:
				printw("Unmatched - %d", key);
				break;
			}
		}

		refresh();
		key = getch();
	}

	endwin();

	return 0;
}

int show_muti_windows()
{
	int x_loop, y_loop;
	char a_letter = 'a';
	WINDOW *new_window_ptr, *popup_window_ptr;

	initscr();

	new_window_ptr = newwin(10, 20, 5, 5);
	mvwprintw(new_window_ptr, 2, 2, "%s", "Hello World");
	mvwprintw(new_window_ptr, 5, 2, "%s", "Notice how long lines wrap");
	wrefresh(new_window_ptr);
	sleep(2);

	a_letter = '0';
	for (x_loop = 0; x_loop < COLS - 1; x_loop++) {
		for (y_loop = 0; y_loop < LINES - 1; y_loop++) {
			mvwaddch(stdscr, y_loop, x_loop, a_letter);
			a_letter++;
			if (a_letter > '9')
				a_letter = '0';
		}
	}
	refresh();
	sleep(2);

	touchwin(new_window_ptr);
	wrefresh(new_window_ptr);
	sleep(2);

	popup_window_ptr = newwin(10, 20, 8, 8);
	box(popup_window_ptr, '|', '-');
	mvwprintw(popup_window_ptr, 5, 2, "%s", "Pop Up Window!");
	wrefresh(popup_window_ptr);
	sleep(2);

	touchwin(new_window_ptr);
	wrefresh(new_window_ptr);
	wclear(new_window_ptr);
	delwin(new_window_ptr);

	touchwin(popup_window_ptr);
	wrefresh(popup_window_ptr);
	delwin(popup_window_ptr);

	touchwin(stdscr);
	refresh();
	sleep(2);

	endwin();

	return 0;
}

int show_sub_windows()
{
	char a_letter = '1';
	WINDOW *sub_window_ptr;
	int x_loop, y_loop, counter;

	initscr();

	for (y_loop = 0; y_loop < LINES - 1; y_loop++) {
		for (x_loop = 0; x_loop < COLS - 1; x_loop++) {
			mvwaddch(stdscr, y_loop, x_loop, a_letter);
			a_letter++;
			if (a_letter > '9')
				a_letter = '1';
		}
	}

	sub_window_ptr = subwin(stdscr, 10, 20, 10, 10);
	scrollok(sub_window_ptr, 1);

	touchwin(stdscr);
	refresh();
	sleep(1);

	werase(sub_window_ptr);
	mvwprintw(sub_window_ptr, 2, 0, "%s", "This window will now scroll");
	wrefresh(sub_window_ptr);
	sleep(1);

	for (counter = 1; counter < 10; counter++) {
		wprintw(sub_window_ptr, "%s",
			"This text is both wrapping and scrolling.");
		wrefresh(sub_window_ptr);
		sleep(1);
	}

	delwin(sub_window_ptr);

	touchwin(stdscr);
	refresh();
	sleep(1);

	endwin();

	return 0;
}

int show_pad_model()
{
	char disp_char;
	WINDOW *pad_ptr;
	int x, y, pad_lines, pad_cols;

	initscr();

	pad_lines = LINES + 50;
	pad_cols = COLS + 50;
	pad_ptr = newpad(pad_lines, pad_cols);

	disp_char = 'a';
	for (x = 0; x < pad_lines; x++) {
		for (y = 0; y < pad_cols; y++) {
			mvwaddch(pad_ptr, x, y, disp_char);
			if (disp_char == 'z')
				disp_char = 'a';
			else
				disp_char++;
		}
	}

	prefresh(pad_ptr, 5, 7, 2, 2, 9, 9);
	sleep(1);

	prefresh(pad_ptr, LINES + 5, COLS + 7, 5, 5, 21, 19);
	sleep(1);

	delwin(pad_ptr);
	endwin();

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test the size of term
	case 1:
		show_term_size();
		break;

		//test the color pair of term show
	case 2:
		show_term_color();
		break;

		//test the style of term show 
	case 3:
		show_term_style();
		break;

		//test the choice menu use term
	case 4:
		read_term_choice();
		break;

		//test the echo function of term
	case 5:
		show_term_echo();
		break;

		//test the keyboard capture of term
	case 6:
		show_term_keyboard();
		break;

		//test the muti_windows of term
	case 7:
		show_muti_windows();
		break;

		//test the sub_windows of term
	case 8:
		show_sub_windows();
		break;

		//test the pad model of term
	case 9:
		show_pad_model();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
