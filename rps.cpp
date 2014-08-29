#include <stdio.h>
#include <unistd.h>
//#include <readline/readline.h>
#ifdef linux
#include <ncurses.h>
#endif

int main(int argc, char** argv)
{
#if defined linux
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
#elif defined WIN32
	printf("You probably should use linux.\n");
#else
	fprintf(stderr,"What is this platform even?\n");
#endif
	return 0;
}
