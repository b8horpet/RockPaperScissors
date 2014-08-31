#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#if defined WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <curses.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream> // this dependency should be removed, only needed for program_options
#include <boost/program_options.hpp>

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

enum ConnectionMode
{
	Unknown = 0,
	Server = 1<<0,
	Client = 1<<1,
	Hybrid = Server+Client,
};

int g_Mode = Unknown;

int main(int argc, char** argv)
{
	bool ShouldTryConnection=false;
	std::string host;
	std::string port;

	boost::program_options::options_description desc("Allowed options");
	desc.add_options()
	("help,?", "produce help message")
	("port,p", boost::program_options::value<std::vector<std::string> >(), "set communication port")
	("host,h", boost::program_options::value<std::string>(&host), "set communication host")
	("server", "server mode")
	("client", "client mode")
	;

	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	boost::program_options::notify(vm);

	if(vm.count("help"))
	{
		std::cout << desc << "\n";
		return 0;
	}

	if(vm.count("server"))
	{
		g_Mode |= Server;
	}
	if(vm.count("client"))
	{
		g_Mode |= Client;
	}

	if(g_Mode == Unknown)
	{
		std::cerr << desc << "\n" << "Either --server or --client should be used.\n";
		return 1;
	}
	if(g_Mode == Hybrid)
	{
		std::cerr << desc << "\n" << "Only one of the following options should be used: --server --client\n" << "Hybrid mode is not supported yet.\n";
		return 1;
	}

	if(vm.count("port"))
	{
		/*std::cout << "ports:";
		std::vector<std::string> myvec=vm["port"].as<std::vector<std::string> >();
		for(std::vector<std::string>::iterator i=myvec.begin(); i!=myvec.end(); ++i)
		{
			std::cout << " " << *i;
		}
		std::cout << "\n";*/
	}

	if(vm.count("host"))
	{
		//std::cout << "host: " << vm["host"].as<std::string>() << "\n";
	}
	else
	{
		std::cerr << desc << "\n";
		return 1;
	}

/*
	// Backup
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
*/



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
	return 0;
}
