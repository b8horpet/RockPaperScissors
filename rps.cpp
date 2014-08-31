#include <stdio.h>
#include <unistd.h>
//#include <readline/readline.h>
#if defined linux
#include <ncurses.h>
#elif defined WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <curses.h>
#else
//what? O_o
#endif

#include <boost/asio.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/version.hpp>

#ifdef WIN32
unsigned sleep(unsigned seconds)
{
	Sleep(seconds*1000);
	return 0;
}
#endif

int main(int argc, char** argv)
{
#if defined linux || defined WIN32
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	printw("Rock Paper Scissors Game\n");
	printw("Built with Boost v %d.%02d.%02d\n",BOOST_VERSION/100000,(BOOST_VERSION/100)%100,BOOST_VERSION%100);
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
/*#elif defined WIN32
	printf("You should probably use linux.\n");*/
#else
	fprintf(stderr,"What is this platform even?\n");
#endif
	return 0;
}
