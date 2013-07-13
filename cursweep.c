// Compile with '-lpanel -lncurses -lm'

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <panel.h>

#define FGgreen 1
#define FGyellow 2
#define FGred 7

#define BGred 10
#define BGhud 11
#define BGblue 12
#define BGnormal 13

#define UL 0x80
#define UU 0x40
#define UR 0x20
#define LL 0x10
#define RR 0x08
#define BL 0x04
#define BB 0x02
#define BR 0x01

#define HELP_WIDTH 52

/* Displays the help (in a created window and panel, for going back to the normal field after) */
void Help (int y, int x)
{
	WINDOW *help;
	PANEL *up;

	help = newwin (8, HELP_WIDTH, 1, 0);
	up = new_panel (help);
	update_panels ();
	doupdate ();

	box (help, 0, 0);
	wbkgd (help, COLOR_PAIR (BGblue));
	wrefresh (help);
	mvwaddstr (help, 0, HELP_WIDTH/2 - 2, "HELP");
	wattron (help, A_BOLD);
	mvwaddstr (help, 1, 1, "Arrow Keys:");
	mvwaddstr (help, 2, 1, "W,A,S,D:");
	mvwaddstr (help, 3, 1, "Space or Mouse B1 (one click):");
	mvwaddstr (help, 4, 1, "'m' or Mouse B2 (one click):");
	mvwaddstr (help, 5, 1, "'n' or Mouse B1 (double click):");
	mvwaddstr (help, 6, 1, "'q':");
	wattroff (help, A_BOLD);
	mvwaddstr (help, 1, 13, "move 1 space");
	mvwaddstr (help, 2, 10, "move 4 spaces");
	mvwaddstr (help, 3, 32, "step space");
	mvwaddstr (help, 4, 33, "mark flag");
	mvwaddstr (help, 5, 33, "check surroundings");
	mvwaddstr (help, 6, 6, "quit");

// writes the help window, wait for some key to be pressed and delete the help window
	wrefresh (help);
	curs_set (0);
	getch ();
	curs_set (1);

	wbkgd (help, COLOR_PAIR (BGnormal));
	werase (help);
	wrefresh (help);
	del_panel (up);
	delwin (help);

	move (y, x);
}


/* Initialize the tab, with lin lines and col columns, and put zeros everywhere */
char** InitTab (int lin, int col)
{
	int i, j;
	char **tab;

// alloc the tab, with lin lines and col columns
	tab = (char**) malloc (lin * sizeof (char*));
	if (tab == NULL)
		exit (1);
	for (i = 0; i < lin; i++) {
		tab[i] = (char*) malloc (col * sizeof (char));
		if (tab[i] == NULL)
			exit (1);
	}

// puts '0' all over tab
	for (i = 0; i < lin; i++)
		for (j = 0; j < col; j++)
			tab[i][j] = '0';

	return tab;
}


/* We need random bombs, right? */
void InitBombs (char **tab, int lin, int col, int bombs)
{
	int i, j, bomb;

	srand (time (NULL));

// random bombs
	for (bomb = 0; bomb < bombs; bomb++) {
// can't put bomb where there already is one
		do {
			i = rand () % lin;
			j = rand () % col;
		} while (tab[i][j] == 'B');
		tab[i][j] = 'B';
	}
}


/* Get the number of bombs int the side of every spot */
void InitNumbers (char **tab, int lin, int col)
{
	int i, j;

// goes through the tab matrix
	for (i = 0; i < lin; i++) {
		for (j = 0; j < col; j++) {
// if there's a bomb, say to everyone around "hey, there's a bomb in your side ;]"
			if (tab[i][j] == 'B') {
				if (i != 0 && j != 0 && tab[i-1][j-1] != 'B')	// UL
					tab[i-1][j-1]++;
				if (i != 0 && tab[i-1][j] != 'B')	// U
					tab[i-1][j]++;
				if (i != 0 && j != col - 1 && tab[i-1][j+1] != 'B')	// UR
					tab[i-1][j+1]++;
				if (j != 0 && tab[i][j-1] != 'B')	// L
					tab[i][j-1]++;
				if (j != col - 1 && tab[i][j+1] != 'B')	// R
					tab[i][j+1]++;
				if (i != lin - 1 && j != 0 && tab[i+1][j-1] != 'B')	// BL
					tab[i+1][j-1]++;
				if (i != lin - 1 && tab[i+1][j] != 'B')	// B
					tab[i+1][j]++;
				if (i != lin - 1 && j != col - 1 && tab[i+1][j+1] != 'B')	// BR
					tab[i+1][j+1]++;
			}
		}
	}
}


/* Create the field, with the squares */
void FieldCreate (int lin, int col)
{
	int i, j;

	for (i = 0; i < lin; i++) {
		for (j = 0; j < col; j++) {
			mvaddch (i + 1, j, '*');
		}
	}
}


/* Mark the tab as a bomb */
void Mark (int y, int x)
{
	char c;

	c = mvinch (y, x);
	if (c == 'B') {
		attron (COLOR_PAIR (BGnormal));
		mvaddch (y, x, '*');
	}
	else if (c == '*') {
		attron (COLOR_PAIR (FGred));
		mvaddch (y, x, 'B');
		attron (COLOR_PAIR (BGnormal));
	}

	refresh ();
	move (y, x);
}


/* You lost, sucka! Red vortex and KABOOM! */
void Loser (WINDOW *hud, int y, int x, int lin, int col)
{
	int i, j, dx = 0, dy = 1, ex = 0, ey = 0;

	wbkgd (hud, COLOR_PAIR (BGred));
	mvwaddstr (hud, 0, COLS/2 - 8, "YOU LOST, SUCKA!");
	wrefresh (hud);

	attron (COLOR_PAIR (FGyellow) | A_BOLD);
	mvaddstr (LINES/2 - 1, COLS/2 - 5, "\\/\\/\\/\\/\\/");
	mvaddstr (LINES/2, COLS/2 - 5, "< KABOOM >");
	mvaddstr (LINES/2 + 1, COLS/2 - 5, "/\\/\\/\\/\\/\\");
	refresh ();

	attron (COLOR_PAIR (BGred));
// center square
	mvaddstr (y, x, " ");

// spiral animation for losing
	for (i = 0; i < abs (LINES - y) + 1 || i < y + 1 || i < abs (COLS - x) + 1 || i < x + 1; i++) {
		dx = x + i;
		ex = x - i;
		dy = y;
		ey = y;
// d:↑ e:↓
		for (j = 0; j < i; j++) {
			mvaddstr (dy, dx, " ");
			mvaddstr (ey, ex, " ");
			refresh ();
			usleep (5e5/pow(i,2));
			dy--;
			ey++;
		}
// d:← e:→
		for (j = 0; j < i * 2; j++) {
			mvaddstr (dy, dx, " ");
			mvaddstr (ey, ex, " ");
			refresh ();
			usleep (5e5/pow(i,2));
			dx--;
			ex++;
		}
// d:↓ e:↑
		for (j = 0; j < i; j++) {
			mvaddstr (dy, dx, " ");
			mvaddstr (ey, ex, " ");
			refresh ();
			usleep (5e5/pow(i,2));
			dy++;
			ey--;
		}
	}
}


/* stepped on a bomb! Let's see what happens: 1 if you stepped right, 0 when tried to step a marked, -1 if stepped a bomb! */
int Step (int y, int x, char **tab, int *stepped)
{
	char c;
	int color;

	c = mvinch (y, x);
// you can only step a square if it has a '*' on it [unstepped and unmarked]
	if (c != '*')
		return 0;
	else if (tab[y - 1][x] == 'B')
		return -1;
	else {
		color = tab[y - 1][x] - '0';
		attron (COLOR_PAIR (color) | A_BOLD);
		if (tab[y - 1][x] != '0')
			mvaddch (y, x, tab[y - 1][x]);
		else
			mvaddch (y, x, ' ');
		refresh ();
		(*stepped)++;
// if you stepped a ' ' (0), all around it will open aswell (recursively)
		if (tab[y - 1][x] == '0') {
			Step (y - 1, x - 1, tab, stepped);
			Step (y - 1, x, tab, stepped);
			Step (y - 1, x + 1, tab, stepped);
			Step (y, x - 1, tab, stepped);
			Step (y, x + 1, tab, stepped);
			Step (y + 1, x - 1, tab, stepped);
			Step (y + 1, x, tab, stepped);
			Step (y + 1, x + 1, tab, stepped);
		}
	}

	move (y, x);

	return 1;
}


/* Check surroundings: if there's no 'B'omb, step spaces around */
void Check (int y, int x, char **tab, int lin, int col)
{
// bitwise boolean, for knowing if the spaces exists (3x3 (less the center), from up to down, from left to right)
	char exists = 0x00, c;

	c = mvinch (y, x);
// check surroundings only if already stepped
	if (c != '*' && c != 'B') {

// verify which sides exist
		if (y > 1 && x > 0) {
			exists |= UL;
			c = mvinch (y - 1, x - 1);
			if (tab[y - 2][x - 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (y > 1) {
			exists |= UU;
			c = mvinch (y - 1, x);
			if (tab[y - 2][x] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (y > 1 && x < col - 1) {
			exists |= UR;
			c = mvinch (y - 1, x + 1);
			if (tab[y - 2][x + 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (x > 0) {
			exists |= LL;
			c = mvinch (y, x - 1);
			if (tab[y - 1][x - 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (x < col - 1) {
			exists |= RR;
			c = mvinch (y, x + 1);
			if (tab[y - 1][x + 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (y < lin && x > 0) {
			exists |= BL;
			c = mvinch (y + 1, x - 1);
			if (tab[y][x - 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (y < lin) {
			exists |= BB;
			c = mvinch (y + 1, x);
			if (tab[y][x] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}
		if (y < lin && x < col - 1) {
			exists |= BR;
			c = mvinch (y + 1, y + 1);
			if (tab[y][x + 1] == 'B' && c != 'B') {
				move (y, x);
				return;
			}
		}

// none is 'B'omb, so print the '*'s in green
		attron (COLOR_PAIR (FGgreen));

		if (exists & UL) {
			c = mvinch (y - 1, x - 1);
			if (c == '*')
				mvaddch (y - 1, x - 1, '*');
		}
		if (exists & UU) {
			c = mvinch (y - 1, x);
			if (c == '*')
				mvaddch (y - 1, x, '*');
		}
		if (exists & UR) {
			c = mvinch (y - 1, x + 1);
			if (c == '*')
				mvaddch (y - 1, x + 1, '*');
		}
		if (exists & LL) {
			c = mvinch (y, x - 1);
			if (c == '*')
				mvaddch (y, x - 1, '*');
		}
		if (exists & RR) {
			c = mvinch (y, x + 1);
			if (c == '*')
				mvaddch (y, x + 1, '*');
		}
		if (exists & BL) {
			c = mvinch (y + 1, x - 1);
			if (c == '*')
				mvaddch (y + 1, x - 1, '*');
		}
		if (exists & BB) {
			c = mvinch (y + 1, x);
			if (c == '*')
				mvaddch (y + 1, x, '*');
		}
		if (exists & BR) {
			c = mvinch (y + 1, x + 1);
			if (c == '*')
				mvaddch (y + 1, x + 1, '*');
		}

		refresh ();

		attron (COLOR_PAIR (BGnormal));
		move (y, x);
	}
}




int main ()
{
	char **tab;	// o que aparece pro usuário: 'B' pra bomba, números pra quantidade de bombas ao redor
	int c;	// for getch ()
	int cont = 0, stepped, lin, col, bombs, y = 1, x = 0;
	WINDOW *hud;
	MEVENT event;

	initscr ();
	keypad (stdscr, true);
	start_color ();
	assume_default_colors (-1, -1);	// -1 gives the terminal default colors

	init_pair (BGnormal, -1, -1);	// default colors

	init_pair (1, COLOR_GREEN, -1);		// colors	[and for check]
	init_pair (2, COLOR_YELLOW, -1);	// for		[and for KABOOM]
	init_pair (3, COLOR_WHITE, -1);		// numbers
	init_pair (4, COLOR_CYAN, -1);
	init_pair (5, COLOR_BLUE, -1);
	init_pair (6, COLOR_MAGENTA, -1);
	init_pair (7, COLOR_RED, -1);		//			[and for Mark]
	init_pair (8, COLOR_BLACK, -1);

	init_pair (BGred, COLOR_BLACK, COLOR_RED);	// color for steppin on a bomb [u died, sucka xP]
	init_pair (BGhud, COLOR_WHITE, COLOR_GREEN);	// HUD color
	init_pair (BGblue, COLOR_WHITE, COLOR_BLUE);	// color for the help window

	mousemask (BUTTON1_CLICKED | BUTTON3_CLICKED | BUTTON1_DOUBLE_CLICKED, NULL);

	hud = subwin (stdscr, 1, COLS, 0, 0);
	wbkgd (hud, COLOR_PAIR (BGhud) | A_BOLD);
	mvwaddstr (hud, 0, COLS/2 - 4, "CURSWEEP");
	wrefresh (hud);

	mvprintw (6, 0, "Number of lines [10~%d] > ", LINES - 1);
	do {
		mvscanw (6, 26, "%d", &lin);
	} while (lin < 10 || lin > LINES - 1);

	mvprintw (9, 0, "Number of columns [10~%d] > ", COLS);
	do {
		mvscanw (9, 28, "%d", &col);
	} while (col < 10 || col > COLS);

	mvprintw (12, 0, "Number of bombs [%2.d~%3.d] > ", lin * col/10, lin * col/3);
	do {
		mvscanw (12, 27, "%d", &bombs);
	} while (bombs < lin * col/10 || bombs > lin * col/3);

	mvwaddstr (hud, 0, 0, "'?': Help");

	move (6, 0);
	clrtobot ();
	refresh ();
	noecho ();
	attron (A_BOLD);

	tab = InitTab (lin, col);
	InitBombs (tab, lin, col, bombs);
	InitNumbers (tab, lin, col);
	FieldCreate (lin, col);
	mvwprintw (hud, 0, COLS - 9, "Last: %3.d", lin * col - bombs);
	wrefresh (hud);
	move (1, 0);

// main loop
	while (cont < lin * col - bombs) {
		stepped = 0;
		c = getch ();

		switch (c) {
// asked for help
			case '?':
				Help (y, x);
				break;

// if mouse was clicked, get it's position
			case KEY_MOUSE:
				getmouse (&event);

				if (event.x > col || event.y < 1 || event.y > lin)
					break;
// get position
				x = event.x;
				y = event.y;
// B2: mark
				if (event.bstate & BUTTON3_CLICKED)
					goto MARK;
// B1 double: check
				if (event.bstate & BUTTON1_DOUBLE_CLICKED)
					goto CHECK;
// B1: step
				if (!(event.bstate & BUTTON1_CLICKED))
					break;

// step
			case ' ':
				switch (Step (y, x, tab, &stepped)) {
					case 1:
						cont += stepped;
						mvwprintw (hud, 0, COLS - 9, "Last: %3.d", lin * col - bombs - cont);
						wrefresh (hud);
						break;
					case -1:
						Loser (hud, y, x, lin, col);
						goto END;
				}
				break;

			case 'm':
MARK:
				Mark (y, x);
				break;

			case 'n':
CHECK:
				Check (y, x, tab, lin, col);
				break;

// move right: right for 1 space, 'd' for 4 spaces
			case KEY_RIGHT:
				if (x < col - 1) {
					x++;
					move (y, x);
				}
				break;
			case 'd':
				if (x < col - 1) {
					if (x > col - 5)
						x = col - 1;
					else
						x += 4;
					move (y, x);
				}
				break;
// move left: left for 1 space, 'a' for 4 spaces
			case KEY_LEFT:
				if (x > 0) {
					x--;
					move (y, x);
				}
				break;
			case 'a':
				if (x > 0) {
					if (x < 4)
						x = 0;
					else
						x -= 4;
					move (y, x);
				}
				break;
// move up: up for 1 space, 'w' for 4 spaces
			case KEY_UP:
				if (y > 1) {
					y--;
					move (y, x);
				}
				break;
			case 'w':
				if (y > 1) {
					if (y < 5)
						y = 1;
					else
						y -= 4;
					move (y, x);
				}
				break;
// move down: down blablabla, 's' blablabla
			case KEY_DOWN:
				if (y < lin) {
					y++;
					move (y, x);
				}
				break;
			case 's':
				if (y < lin) {
					if (y > lin - 4)
						y = lin;
					else
						y += 4;
					move (y, x);
				}
				break;

			case 'q':
				goto END;
		}
	}

// if you won
	attron (COLOR_PAIR (FGyellow) | A_BOLD);
	mvaddstr (LINES/2 - 1, COLS/2 - 12, "Congratulations, you won!");
	mvaddstr (LINES/2, COLS/2 - 5, "FUCK YEAH!");

	getch ();

END:
	endwin ();
	return 0;
}
