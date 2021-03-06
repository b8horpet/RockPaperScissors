#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
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

#include "MD5.hxx"

#ifdef WIN32
unsigned sleep(unsigned seconds)
{
	Sleep(seconds*1000);
	return 0;
}
#endif

using namespace rps::crypto::md5;

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
	const char * const message;
	explicit my_exception() : message("lofasz") {}
	explicit my_exception(const char * const m) : message(m) {}
	virtual ~my_exception() throw() {}
	virtual const char * what() const throw() { return message; }
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

bool gShouldTryConnection=false;
std::string gHost;
std::string gPort;

void ParseCommandLineOptions(int argc, char** argv)
{
	boost::program_options::options_description desc("Allowed options");
	desc.add_options()
	("help,?", "produce help message")
	("port,p", boost::program_options::value<std::vector<std::string> >(), "set communication port")
	("host,h", boost::program_options::value<std::string>(&gHost), "set communication host")
	("server", "server mode")
	("client", "client mode")
	("version,v", "print version info and exit")
	;

	boost::program_options::variables_map vm;
	try
	{
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	}
	catch(std::exception& e)
	{
		std::cout << "ERROR: " << e.what() << "!\n";
		exit(1);
	}
	boost::program_options::notify(vm);

	if(vm.count("help"))
	{
		std::cout << desc << "\n";
		exit(0);
	}

	if(vm.count("version"))
	{
		std::cout << "Rock Paper Scissors Game\n" << "Built with Boost v" << BOOST_VERSION/100000 << "." << std::setfill('0') << std::setw(2) << (BOOST_VERSION/100)%100 << "." << std::setfill('0') << std::setw(2) << BOOST_VERSION%100 << "\n";
		exit(0);
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
		exit(1);
	}
	if(g_Mode == Hybrid)
	{
		std::cerr << desc << "\n" << "Only one of the following options should be used: --server --client\n" << "Hybrid mode is not supported yet.\n";
		exit(1);
	}

	if(vm.count("port"))
	{
		gShouldTryConnection=true;
		std::vector<std::string> ports=vm["port"].as<std::vector<std::string> >();
		if(!ports.empty())
			gPort=ports.back();
	}

	if(!vm.count("host"))
	{
		std::cerr << desc << "\n";
		exit(1);
	}
}

void InitWindow()
{
/*	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	scrollok(stdscr,TRUE);
	noecho();*/
#define printw printf
	printw("Rock Paper Scissors Game\n");
	printw("Built with Boost v %d.%02d.%02d\n",BOOST_VERSION/100000,(BOOST_VERSION/100)%100,BOOST_VERSION%100);
	printw("\nStarting TCP stuff\n");
	refresh();
}

char valid_chars[3]={'r','p','s'};

int main(int argc, char** argv)
{
	ParseCommandLineOptions(argc,argv);
	InitWindow();
	srand(time(NULL));
	/// Do the thing
	char buf[4096]={0};

	try
	{
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::resolver resolver(io_service);
		printw("Querying host\n");
		refresh();
		boost::asio::ip::tcp::resolver::query query(gHost,gPort);
		printw("Resolving host\n");
		refresh();
		boost::asio::ip::tcp::socket socket(io_service);
		if(gShouldTryConnection)
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
				boost::asio::ip::tcp::acceptor acceptor(io_service,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),boost::lexical_cast<int>(gPort)));
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
			// temporary workaround
			// only serves testing purposes
			while(true)
			{
				char salt1=(rand()%255) +1;
				char gothash[4096]={0};
				if(g_Mode==Server)
				{
					socket.read_some(boost::asio::buffer(gothash,4096));
					printw("read: ");
					for(int hx=0; hx<16; ++hx)
						printw("%02hhx",gothash[hx]);
					printw("\n");
					refresh();
					memset(buf,0,4096);
					switch(rand()%3)
					{
					case 0:
						strcpy(buf+1,"r");
						break;
					case 1:
						strcpy(buf+1,"p");
						break;
					case 2:
						strcpy(buf+1,"s");
						break;
					default:
						throw my_exception("wtf?");
						break;
					}
					printw("send: %s\n",buf+1);
					refresh();
					buf[0]=buf[2]=salt1;
					digest d = generate(buf,3);
					std::copy(d.begin(),d.end(),buf);
					printw("send: ");
					for(int hx=0; hx<strlen(buf); ++hx)
						printw("%02hhx",buf[hx]);
					printw("\n");
					refresh();
					socket.write_some(boost::asio::buffer(buf,16));
				}
				else
				{
					memset(buf,0,4096);
					switch(rand()%3)
					{
					case 0:
						strcpy(buf+1,"r");
						break;
					case 1:
						strcpy(buf+1,"p");
						break;
					case 2:
						strcpy(buf+1,"s");
						break;
					default:
						throw my_exception("wtf?");
						break;
					}
					printw("send: %s\n",buf+1);
					refresh();
					buf[0]=buf[2]=salt1;
					digest d = generate(buf,3);
					std::copy(d.begin(),d.end(),buf);
					printw("send: ");
					for(int hx=0; hx<strlen(buf); ++hx)
						printw("%02hhx",buf[hx]);
					printw("\n");
					refresh();
					socket.write_some(boost::asio::buffer(buf,16));
					socket.read_some(boost::asio::buffer(gothash,4096));
					printw("read: ");
					for(int hx=0; hx<16; ++hx)
						printw("%02hhx",gothash[hx]);
					printw("\n");
					refresh();
				}
				char salt2[4096]={0};
				if(g_Mode==Server)
				{
					socket.read_some(boost::asio::buffer(salt2,4096));
					memset(buf,0,4096);
					buf[0]=salt1;
					socket.write_some(boost::asio::buffer(buf,1));
				}
				else
				{
					memset(buf,0,4096);
					buf[0]=salt1;
					socket.write_some(boost::asio::buffer(buf,1));
					socket.read_some(boost::asio::buffer(salt2,4096));
				}
				bool successfullyunhashed=false;
				for(int i=0; i<3; ++i)
				{
					char p=valid_chars[i];
					memset(buf,0,4096);
					memcpy(buf+strlen(buf),salt2,strlen(salt2));
					buf[strlen(buf)]=p;
					memcpy(buf+strlen(buf),salt2,strlen(salt2));
					digest d=generate(buf,strlen(buf));
					std::copy(d.begin(),d.end(),buf);
					if(strcmp(buf,gothash)==0)
					{
						printw("read: %c\n",p);
						refresh();
						successfullyunhashed=true;
						break;
					}
				}
				if(!successfullyunhashed)
				{
					printw("cannot unpack, terminating\n");
					refresh();
					break;
				}
				printw("--- *** ---\n");
				//usleep(1000);
			}
		}
		printw("Done. Press any key\n");
		refresh();
	}
	catch(std::exception& e)
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
	printw("Exiting\n");
	refresh();
//	endwin();
	return 0;
}
