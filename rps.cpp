#include <stdio.h>
#include <unistd.h>
#include <string>
//#include <readline/readline.h>
#if defined WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <curses.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/version.hpp>

#ifdef WIN32
unsigned sleep(unsigned seconds)
{
	Sleep(seconds*1000);
	return 0;
}
#endif

#if (BOOST_VERSION/100)%100 < 48
namespace boost
{
	namespace asio
	{
		template<
			typename Protocol,
			typename SocketService,
			typename Iterator>
		Iterator connect(
			basic_socket< Protocol, SocketService > & s,
			Iterator begin)
		{
			boost::system::error_code error = boost::asio::error::host_not_found;
			boost::asio::ip::tcp::resolver::iterator end;
			while (error && begin != end)
			{
				s.close();
				s.connect(*begin++, error);
			}
			if (error)
				throw boost::system::system_error(error);
			return begin;
		}
	}
}
#endif

int main(int argc, char** argv)
{
	bool ShouldTryConnection=false;
	if(argc < 2)
	{
		fprintf(stderr,"Usage %s <host> [port]\n",argv[0]);
		return 1;
	}
	std::string host(argv[1]);
	std::string port;
	if(argc>2)
	{
		ShouldTryConnection=true;
		port=argv[2];
	}
#if defined linux || defined WIN32
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	printw("Rock Paper Scissors Game\n");
	printw("Built with Boost v %d.%02d.%02d\n",BOOST_VERSION/100000,(BOOST_VERSION/100)%100,BOOST_VERSION%100);
	refresh();
	printw("Press any key to start");
	refresh();
	getch();
	printw("\nStarting TCP stuff\n");
	refresh();
	/// Do the thing

try{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver(io_service);
	printw("Querying host\n");
	refresh();
	boost::asio::ip::tcp::resolver::query query(host,port);
	printw("Resolving host\n");
	refresh();
	boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
	boost::asio::ip::tcp::socket socket(io_service);
	if(ShouldTryConnection)
	{
		printw("Connecting socket\n");
		refresh();
		boost::asio::connect(socket, it);
	}
	printw("Done. Press any key\n");
	refresh();
} catch(std::exception e)
{
	printw("ERRROR: %s!\n",e.what());
	printw("Press any key\n");
	refresh();
	getch();
	endwin();
	return 1;
}

	/// Thing done, exiting
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
