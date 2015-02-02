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
#include <boost/lexical_cast.hpp>

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

		template<
			typename SyncWriteStream,
			typename ConstBufferSequence>
		std::size_t write(
			SyncWriteStream & s,
			const ConstBufferSequence & buffers,
			boost::system::error_code & ec)
		{
			return write(s,buffers,boost::asio::transfer_all(),ec);
		}
	}
}
#endif

class my_exception : public std::exception
{
public:
	const char* const message;
	explicit my_exception() : message("lofasz") {}
	explicit my_exception(const char * m) : message(m) {}
	virtual ~my_exception() throw() {}
	virtual const char* what() const throw() { return message; }
};

enum ConnectionMode
{
	Unknown = 0,
	Server = 1<<0,
	Client = 1<<1,
	Hybrid = Server+Client,
};

int g_Mode = Unknown;

enum RockPaperScissors
{
	None = 0,
	Rock,
	Paper,
	Scissors,
};

class Step
{
//TODO: should be threadsafe
// some kind of locking or atomic?
	RockPaperScissors Value;
public:
	Step():Value(None){}
	void Set(RockPaperScissors s)
	{
		//lock
		Value=s;
		//unlock
	}
	RockPaperScissors Get()
	{
		//lock
		RockPaperScissors retval=Value;
		//unlock
	}
	void Clear()
	{
		//lock
		Value=None;
		//release
	}
};

enum WhatNow
{
	Init=-1,
	PickStep=0,
	SendStep,
	GetKey,
	UnlockStep,
	MatchResults,
	NewRound,
};

class GameState
{
public:
	std::vector<Step> History;
	WhatNow StateMachine;
	long wins,loses,draws;
	bool WantMoreRounds;
	//TODO
};

class GameLogic
{
	GameState State;
	Step BestStep;
	std::string SentKey,ReceivedKey;
	std::string SentStep,ReceivedStep;
	//vmi lock
	//vmi Communicator
public:
	void NextState()
	{
		//lock
		//state changed event?
		switch(State.StateMachine)
		{
		case Init:
			{
				//...
				State.StateMachine=PickStep;
			}
			break;
		case PickStep:
			{
				BestStep.Clear();
				//start picking
				State.StateMachine=SendStep;
			}
			break;
		case SendStep:
			{
				SentStep=PickBestStep();
				//Communicator.Send(EncodeStep());
				State.StateMachine=GetKey;
			}
			break;
		case GetKey:
			{
				//ReceivedStep=Communicator.Get();
				//StoreKey(Communicator.Get());
				State.StateMachine=UnlockStep;
			}
			break;
		case UnlockStep:
			{
				ReceivedStep=DecodeStep();
				State.StateMachine=MatchResults;
			}
			break;
		case MatchResults:
			{
				DidIWin();
				if(State.WantMoreRounds)
					State.StateMachine=PickStep;
				else
					exit(0);
			}
			break;
		default:
			assert(false);
		}
		//unlock
	}
	void DidIWin()
	{
		RockPaperScissors mine=StringToStep(SentStep);
		RockPaperScissors other=StringToStep(ReceivedStep);
		assert(mine&&other);
		if(mine==other)
			++State.draws;
		if((mine-other+3)%3==1)
			++State.wins;
		else
			++State.loses;
		//GameState.SaveRound;
	}
	std::string PickBestStep()
	{
		return StepToString(BestStep.Get());
	}
	std::string DecodeStep()
	{
		//use the key;
		return ReceivedStep;
	}

	void PickKey()
	{
		SentKey="";
	}
	void StoreKey(std::string k)
	{
		assert(k.empty());
		ReceivedKey=k;
	}

	std::string EncodeStep()
	{
		//majd
		PickKey();
		return SentStep;
	}
	std::string StepToString(RockPaperScissors rps)
	{
		switch(rps)
		{
			case Rock:
				return "r";
			case Paper:
				return "p";
			case Scissors:
				return "s";
			default:
				return "";
		}
		return "";
	}
	RockPaperScissors StringToStep(std::string s)
	{
		if(s.empty())
			return None;
		if(s[1]!=0)
			return None;
		switch(s[0])
		{
		case 'r':
			return Rock;
		case 'p':
			return Paper;
		case 's':
			return Scissors;
		default:
			return None;
		}
		return None;
	}
} g_GameLogic;

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
	try{
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	}
	catch(std::exception& e)
	{
		std::cout << "ERROR: " << e.what() << "!\n";
		return 1;
	}
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
		ShouldTryConnection=true;
		std::vector<std::string> ports=vm["port"].as<std::vector<std::string> >();
		if(!ports.empty())
			port=ports.back();
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
	char buf[4096];
	memset(buf,0,4096);

try{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver(io_service);
	printw("Querying host\n");
	refresh();
	boost::asio::ip::tcp::resolver::query query(host,port);
	printw("Resolving host\n");
	refresh();
	boost::asio::ip::tcp::socket socket(io_service);
	if(ShouldTryConnection)
	{
		printw("Connecting socket\n");
		refresh();
		switch(g_Mode)
		{
		case Client:
			{
			boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
			boost::asio::connect(socket, it);
			}
			break;
		case Server:
			{
			boost::asio::ip::tcp::acceptor acceptor(io_service,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),boost::lexical_cast<int>(port)));
			acceptor.accept(socket);
			}
			break;
		case Hybrid:
			throw my_exception("not implemented yet");
			break;
		case Unknown:
		default:
			throw my_exception("unknown mode");
			break;
		}
		socket.read_some(boost::asio::buffer(buf,4096));
		printw("read: %s\n",buf);
		memset(buf,0,4096);
		strcpy(buf,"Allah Akbar!");
		printw("sending %s\n",buf);
		socket.write_some(boost::asio::buffer(buf,strlen(buf)));
	}
	printw("Done. Press any key\n");
	refresh();
} catch(std::exception& e)
{
	printw("ERROR: %s!\n",e.what());
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
