#include <stdio.h>
#include <unistd.h>
//#include <readline/readline.h>
#include <ncurses.h>

int main(int argc, char** argv)
{
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	printw("Rock Paper Scissors Game\n");
	refresh();
	getch();
	printw("Exiting ");
	refresh();
	for(int i=0; i<3; ++i)
	{
		sleep(1);
		printw(".");
		refresh();
	}
	sleep(1);
	printw("\n");
	refresh();
	endwin();
	return 0;
}
