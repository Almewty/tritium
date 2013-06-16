#include "stdafx.h"
#include "engine.hpp"
#include "network.hpp"
#include "connection.hpp"
#include "ban.hpp"
#include "admin.hpp"
#include "cmd.hpp"
#include "cfg.hpp"
#include "ip.hpp"
#include "window.h"

#include "resource.h"


// Extern globals
extern CFGMANAGER*   cfg;
extern CMDMANAGER*   cmdmanager;
extern ADMINMANAGER* adminmanager;
extern BANMANAGER*   banmanager;
extern IPMANAGER*    ipmanager;

extern char tom94[7];

// Intern global
NETWORK* net;

NETWORK::NETWORK()
:
players_count(0),
conns_count(0),
crashed(false),

//last_auth(clock()),
last_notice(clock()),
last_update(clock()+1000),
last_clean(clock()),

win_players_count(0),
win_connections_count(0),
win_admins_count(0),
win_bans_count(0),
win_cmds_count(0),

starttime(clock())
{
	for(int i = 0; i < MAX_CONNS; i++)
		// Create the new connection
		connections[i] = new CONNECTION();

	// Set the net
	net = this;

	// Init the mutex
	pthread_mutex_init(&mutex_addconn, NULL);
	pthread_mutex_init(&mutex_count, NULL);

	// Winsock initialization
	{
		dbg_msg(3, "INFO", "Attempting to initialize winsock.");

		// Set up winsock.dll
		WSADATA wsaData;
		// Startup with version 2.0
		if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		{
			dbg_msg(4, "ERROR", " -> Couldn't initialize Winsock.");
			goto error;
		}

		// Console
		dbg_msg(2, "SUCCESS", " -> Winsock initialized.");
	}

	// Line
	dbg_msg(0, "", "----------------------------------------------");

	// Creating auth socket
	{
		//dbg_msg(3, "INFO", "Attempting to create the auth socket.");

		// Forget the auth server... -.-
		/*
		// Set m_sock
		a_sock = socket(AF_INET, SOCK_STREAM, 0);

		// Check if socket creating is successfull
		if(a_sock == 0)
		{
			dbg_msg(4, "ERROR", " -> Couldn't create the auth socket. (%d)", WSAGetLastError());
			goto error;
		}
		*/

		// Console
		//dbg_msg(2, "SUCCESS", " -> Created the auth socket.");
	}

	// Connect to the auth server
	{
		//dbg_msg(3, "INFO", "Attempting to connect to the auth server.");

		/*unsigned char ip[16] = "193.200.241.203";

		// Yay, conencted to auth server
		if(!connect_auth((char*)ip, 15300))
		{
			dbg_msg(4, "ERROR", " -> Couldn't connect to the auth server. (%d)", WSAGetLastError());
			//goto error;
		}
		else*/

		// Console
		//dbg_msg(2, "SUCCESS", " -> Connected to the auth server");
	}

	// Line
	//dbg_msg(0, "", "----------------------------------------------");

	// Load cfg
	{
		// Create adminmanager
		cfg = new CFGMANAGER();
	}

	// Load cmds
	{
		// Create adminmanager
		cmdmanager = new CMDMANAGER();
	}

	// Load bans
	{
		// Create banmanager
		banmanager = new BANMANAGER();
	}

	// Load admins
	{
		// Create adminmanager
		adminmanager = new ADMINMANAGER();
	}

	// Load ipmanager
	{
		// Create adminmanager
		ipmanager = new IPMANAGER();
	}

	// Line
	dbg_msg(0, "", "----------------------------------------------");

	// Creating conn socket
	{
		dbg_msg(3, "INFO", "Attempting to create the connection socket.");

		// Set m_sock
		m_sock = socket(AF_INET, SOCK_STREAM, 0);

		// Check if socket creating is successfull
		if(m_sock == 0)
		{
			dbg_msg(4, "ERROR", " -> Couldn't create connection socket. (%d)", WSAGetLastError());
			goto error;
		}

		// Console
		dbg_msg(2, "SUCCESS", " -> Created connection socket. Descriptor: %d", m_sock);
	}

	// Bind socket to port
	{
		dbg_msg(3, "INFO", "Attempting to bind the connection socket.");

		// Client port (5400 default)
		int port = cfg->client_port;

		// Init m_addr
		m_addr.sin_family      = AF_INET;
		m_addr.sin_addr.s_addr = INADDR_ANY;
		m_addr.sin_port        = htons(port);

		// Bind
		if(bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
		{
			dbg_msg(4, "ERROR", " -> Couldn't bind connection socket to port %d. (%d)", port, WSAGetLastError());
			goto error;
		}

		// Console
		dbg_msg(2, "SUCCESS", " -> Connection socket bound to port %d.", port);
	}

	// Start listening
	{
		dbg_msg(3, "INFO", "Attempting to start listening to new connections.");

		if(!listen())
		{
			dbg_msg(4, "ERROR", " -> Couldn't start listening to new connections.");
			goto error;
		}

		// SUCESS! :D
		dbg_msg(2, "SUCCESS", " -> Began listening to new connections.");
	}

	// Line
	dbg_msg(0, "", "----------------------------------------------");


	// Initialize main loop
	loop();

	return;

	// Error label
error:
	MessageBox(NULL, "A detailed description of the error can be found in the console.\nShutting down...", "Error while initializing.", MB_ICONEXCLAMATION|MB_OK);
}

NETWORK::~NETWORK()
{
	// Line
	dbg_msg(0, "", "----------------------------------------------");

	for(int i = 0; i < MAX_CONNS; i++)
		// Create the new connection
		delete connections[i];

	// Delete ipmanager
	if(ipmanager)
		delete ipmanager;

	// Delete banmanager
	if(banmanager)
		delete banmanager;

	// Delete adminmanager
	if(adminmanager)
		delete adminmanager;

	// Delete cmdmanager
	if(cmdmanager)
		delete cmdmanager;

	// Delete cmdmanager
	if(cfg)
		delete cfg;

	// No more connections
	pthread_mutex_lock(&mutex_addconn);

	// Close the own socket
	dbg_msg(3, "INFO", "Attempting to close the connection socket.");
	closesocket(m_sock);
	dbg_msg(2, "SUCCESS", " -> Closed connection socket.");

	dbg_msg(3, "INFO", "Attempting to close the auth socket.");
	//closesocket(a_sock);
	dbg_msg(2, "SUCCESS", " -> Closed auth socket.");

	// Cleanup WSA
	dbg_msg(3, "INFO", "Attempting to clean up winsock.");
	WSACleanup();
	dbg_msg(2, "SUCCESS", " -> Winsock cleaned up.");

	// Destroy mutex
	pthread_mutex_destroy(&mutex_addconn);
	pthread_mutex_destroy(&mutex_count);
}

// Proxy func
static void* new_connection(void* _sock)
{
	net->add_connection((int)_sock);
	pthread_exit(NULL);
	return NULL;
}

void NETWORK::add_connection(int _nc_sock)
{
	//dbg_msg(3, "INFO", "Attempting to create a connection.");

	int id = 0;

	for(; id < MAX_CONNS; id++)
	{
		if(connections[id]->active == false)
		{
			// Check if successful
			if(!connections[id]->activate(_nc_sock))
			{
				//dbg_msg(4, "ERROR", "Connection couldn't be created. (Sock: %d | ID: %d)", _nc_sock, id);
				closesocket(_nc_sock);
				return;
			}
			break;
		}
	}

	//dbg_msg(2, "SUCCESS", " -> Connection successfully created. (Sock: %d | ID: %d)", _nc_sock, id);

	conns_count++;

	// Start the connection loop
	connections[id]->loop();

	//dbg_msg(3, "INFO", "Attempting to terminate a connection. (Sock: %d | ID: %d)", _nc_sock, id);

	connections[id]->deactivate();

	conns_count--;

	//dbg_msg(2, "SUCCESS", " -> Connection successfully terminated. (Sock: %d | ID: %d)", _nc_sock, id);

	// return; // To the thread func
}

// Proxy func
static void* new_net_loop(void* _class)
{
	net->net_loop();
	pthread_exit(NULL);
	return NULL;
}

// Proxy window
static void* new_window_loop(void* _class)
{
	net->window_loop();
	pthread_exit(NULL);
	return NULL;
}


void NETWORK::loop()
{
	// Start network loop
	{
		dbg_msg(3, "INFO", "Attempting to create the net loop thread.");

		int result;

		// net_loop erstellen als thread
		pthread_t* thread = new pthread_t;
		result = pthread_create(thread, NULL, new_net_loop, (void*)this);
		delete thread;

		if(result)
		{
			dbg_msg(4, "ERROR", " -> Error while creating net loop thread. (%d)", result);
			return;
		}

		// SUCESS! :D
		dbg_msg(2, "SUCCESS", " -> Initialized the net loop.");
	}

	// Start window loop
	{
		dbg_msg(3, "INFO", "Attempting to create the window loop thread.");

		int result;

		// net_loop erstellen als thread
		pthread_t* thread = new pthread_t;
		result = pthread_create(thread, NULL, new_window_loop, (void*)this);
		delete thread;

		if(result)
		{
			dbg_msg(4, "ERROR", " -> Error while creating the window loop thread. (%d)", result);
			return;
		}

		// SUCESS! :D
		dbg_msg(2, "SUCCESS", " -> Initialized the window loop thread.");
	}

	// Some time the loops need to initialize
	Sleep(200);

	// SUCESS! :D
	dbg_msg(3, "INFO", "Attempting to initialize main loop.");
	dbg_msg(2, "SUCCESS", " -> Initialized the main loop.");

	// Line
	dbg_msg(0, "", "----------------------------------------------");

	dbg_msg(2, "SUCCESS", "Antihack initialized after %d milliseconds.", clock()-net->starttime-200);

	// Line
	dbg_msg(0, "", "----------------------------------------------");


	// For input.txt cmds
	char cmd[1024];
	char msg[1024];

	// Title
	char title[128] = "Antihack v"VERSION" | (C) by ";
	strcat_s(title, sizeof(title), tom94);

	while(true)
	{
		// Update some stuff each second
		if(clock() - last_update >= 1000)
		{
			// Update player count
			pthread_mutex_lock(&mutex_count);
			win_players_count = players_count;
			pthread_mutex_unlock(&mutex_count);

			// Update connections count
			win_connections_count = conns_count;

			// Update admins count
			pthread_mutex_lock(&adminmanager->mutex_admins);
			win_admins_count = adminmanager->admins.size();
			pthread_mutex_unlock(&adminmanager->mutex_admins);

			// Update bans count
			pthread_mutex_lock(&banmanager->mutex_bans);
			win_bans_count = banmanager->bans.size();
			pthread_mutex_unlock(&banmanager->mutex_bans);

			// Update cmds count
			pthread_mutex_lock(&cmdmanager->mutex_cmds);
			win_cmds_count = cmdmanager->cmds.size();
			pthread_mutex_unlock(&cmdmanager->mutex_cmds);

			// Redraw everything
			RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);


			last_update += 1000;
		}

		if(clock() - last_clean >= 5000)
		{
			// Clean IPS (garbage collection)
			ipmanager->clean_ips();

			last_clean += 5000;
		}
		

		// Loop msgs
		if(cfg->notice_loop_interval != 0 && (clock() - last_notice >= (cfg->notice_loop_interval * 1000)))
		{
			last_notice = clock();

			FILE* loopfile;

			if(fopen_s(&loopfile, cfg->notice_loop_message_file, "r") != 0)
				dbg_msg(6, "WARNING", "Couldn't open '%s'.", cfg->inputfile);
			else
			{
				dbg_msg(3, "INFO", "Sending notice loop.");

				// Read the cmd
				while(fgets(msg, 511, loopfile) != NULL)
				{
					for(unsigned int j = 0; j < 1024; j++)
					{
						if(msg[j] == '\n')
						{
							msg[j] = '\0';
							break;
						}
					}

					if(msg[0] == '\0')
						continue;

					send_global_green_text(msg);
					Sleep(200);
				}

				// Close file
				fclose(loopfile);
			}
		}

		// Close when crashed or window doesn't exist :o


		if(crashed || !FindWindow(NULL, title))
		{
			dbg_msg(2, "SUCCESS", "Main loop cancelled.");
			Sleep(1000);
			return;
		}

		// Input stuff
		{
			// File
			FILE* inputfile;

			if(fopen_s(&inputfile, cfg->inputfile, "r") != 0)
				dbg_msg(6, "WARNING", "Couldn't open '%s'.", cfg->inputfile);
			else
			{
				// Line counter
				int i = 0;

				// Read the cmd
				while(fgets(cmd, 511, inputfile) != NULL)
				{
					// Increase the line
					i++;

					for(unsigned int j = 0; j < 1024; j++)
					{
						if(cmd[j] == '\n')
						{
							cmd[j] = '\0';
							break;
						}
					}

					if(cmd[0] == '\0')
						continue;


					dbg_msg(3, "INFO", "Executing command '%s' in line '%d'.", cmd, i);

					// Execute the cmd
					execute_cmd(cmd);

				}

				if(i > 0)
				{
					// Clear the whole file!
					fclose(inputfile);

					while(fopen_s(&inputfile, cfg->inputfile, "w") != 0)
						Sleep(1);
				}

				// Close file
				fclose(inputfile);
			}
		}

		Sleep(20);
	}
}

void NETWORK::net_loop()
{
	int result;

	// Main Loop
	while(true)
	{
		if(crashed)
			break;

		if(is_sock_rdy(m_sock, 20000))
		{
			if(crashed)
				break;

			int _nc_sock = accept();

			if(nc_sock != _nc_sock)
			{
				if((nc_sock = _nc_sock) != INVALID_SOCKET)
				{
					// Verbindungsthread erstellen
					pthread_t* thread = new pthread_t;
					result = pthread_create(thread, NULL, new_connection, (void*)nc_sock);
					delete thread;

					if(result)
						dbg_msg(4, "ERROR", "Error while creating connection thread. (%d)", result);
					//else
					{
						//dbg_msg(2, "SUCCESS", "Created new connection thread. (Sock: %d)", nc_sock);
						//Sleep(1);
					}
				}
				//else
				//	dbg_msg(4, "ERROR", "Invalid socket error with new connection. (%d)", WSAGetLastError());
			}
			//else
			//	dbg_msg(4, "ERROR", "Doubled connection error.");
		}
	}

	dbg_msg(2, "SUCCESS", "Net loop cancelled.");
}

void NETWORK::window_loop()
{
    //HINSTANCE hInstance = GetModuleHandle(NULL);

	//DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)WndProc);


	// Loading our window
	{
		dbg_msg(3, "INFO", "Attempting to create the main window.");


		// Title
		char buf[128] = "Antihack v"VERSION" | (C) by ";
		strcat_s(buf, sizeof(buf), tom94);

		LPCSTR lpszAppName = buf;
		LPCSTR lpszTitle   = buf;

		WNDCLASSEX wc;
		HINSTANCE  hInstance = GetModuleHandle(NULL);

		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInstance;
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON));
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszClassName = lpszAppName;
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);
		wc.hIconSm       = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON), IMAGE_ICON, 16, 16, 0);


		if(RegisterClassEx(&wc) == 0)
		{
			dbg_msg(4, "ERROR", " -> Couldn't register the window class.");
			return;
		}

		hWnd = CreateWindowEx(NULL,
							  lpszAppName,
							  lpszTitle,
							  WS_SYSMENU|WS_MINIMIZEBOX,
							  5,
							  5,
							  550,
							  286,
							  NULL,
							  NULL,
							  hInstance,
							  NULL);


		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);

		// SUCESS! :D
		dbg_msg(2, "SUCCESS", " -> Created the main window.");
	}

	// Line
	dbg_msg(0, "", "----------------------------------------------");

	MSG msg;

	// MSG loop des fensters
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	dbg_msg(2, "SUCCESS", "Window loop cancelled.");
}

bool NETWORK::listen()
{
	// Start listening to new connections
	int result = ::listen(m_sock, 16);

	// Error
	if(result == -1)
		return false;

	// New connection
	return true;
}

int NETWORK::accept()
{
	// Size of addr_length var
	int addr_length = sizeof(m_addr);

	// Wait for new connection and fill in result
	int result = ::accept(m_sock, (sockaddr*)&m_addr, &addr_length);

	// New connection
	return result;
}

void NETWORK::reload_admins()
{
	ADMIN* admin;
	CONNECTION* conn;

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		admin = adminmanager->get_admin(connections[i]->account_name);
		conn = connections[i];

		// Check if admin
		if(admin != NULL && conn->level != admin->level)
			conn->auth(admin->level);
		else if(connections[i]->level && admin == NULL)
			conn->auth(0);
	}
}

CONNECTION* NETWORK::get_connection(char* name)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		if(!strcmp(connections[i]->account_name, name))
		{
			CONNECTION* conn = connections[i];
			return conn;
		}
	}

	return NULL;
}

char* NETWORK::get_account(char _char_name[])
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		if(!strcmp(connections[i]->char_name, _char_name))
		{
			char* account = connections[i]->account_name;
			return account;
		}
	}

	return NULL;
}

char* NETWORK::get_ip(char* name)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_name);
		if(!strcmp(connections[i]->char_name, name))
		{
			pthread_mutex_unlock(&connections[i]->mutex_name);
			char* ip = connections[i]->ip;
			return ip;
		}
		pthread_mutex_unlock(&connections[i]->mutex_name);
	}

	return NULL;
}

int NETWORK::get_auth(char* name)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_name);
		if(!strcmp(connections[i]->char_name, name))
		{
			pthread_mutex_unlock(&connections[i]->mutex_name);
			int level = connections[i]->level;
			return level;
		}
		pthread_mutex_unlock(&connections[i]->mutex_name);
	}

	return -1;
}

int NETWORK::get_ip_connections(char* ip)
{
	int count = 0;

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_ip);
		if(!strcmp(connections[i]->ip, ip))
			count++;
		pthread_mutex_unlock(&connections[i]->mutex_ip);
	}

	return count;
}

void NETWORK::send_global_green_text(char* msg)
{
	dbg_msg(3, "INFO", "Attempting to send a global green text. (%s)", msg);
	
	sprintf_s(n_gnotice, sizeof(n_gnotice), "%s", msg);

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		connections[i]->n_gnotice = true;
	}

	dbg_msg(2, "SUCCESS", " -> Sent a global green text. (%s)", msg);
}

void NETWORK::send_global_notice(char* msg)
{
	dbg_msg(3, "INFO", "Attempting to send a global notice. (%s)", msg);
	
	sprintf_s(n_notice, sizeof(n_notice), "%s", msg);

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		connections[i]->n_notice = true;
	}

	dbg_msg(2, "SUCCESS", " -> Sent a global notice. (%s)", msg);
}

void NETWORK::send_gm_notice(char* msg)
{
	dbg_msg(3, "INFO", "Attempting to send a GM notice. (%s)", msg);

	sprintf_s(n_notice, sizeof(n_notice), "%s", msg);

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in || connections[i]->level < cmdmanager->get_req_level("notice_gm"))
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		connections[i]->n_notice = true;
	}

	dbg_msg(2, "SUCCESS", " -> Sent a GM notice. (%s)", msg);
}

void NETWORK::send_gm_green_text(char* msg)
{
	dbg_msg(3, "INFO", "Attempting to send a GM green text. (%s)", msg);

	sprintf_s(n_gnotice, sizeof(n_gnotice), "%s", msg);

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in || connections[i]->level < cmdmanager->get_req_level("green_text_gm"))
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		connections[i]->n_gnotice = true;
	}

	dbg_msg(2, "SUCCESS", " -> Sent a GM green text. (%s)", msg);
}

void NETWORK::send_private_notice(char* name, char* msg)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_name);

		if(!strcmp(connections[i]->char_name, name))
		{
			pthread_mutex_unlock(&connections[i]->mutex_name);

			connections[i]->send_notice(msg);
			return;
		}

		pthread_mutex_unlock(&connections[i]->mutex_name);
	}
}

void NETWORK::send_private_green_text(char* name, char* msg)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_name);

		if(!strcmp(connections[i]->char_name, name))
		{
			pthread_mutex_unlock(&connections[i]->mutex_name);

			connections[i]->send_green_text(msg);
			return;
		}

		pthread_mutex_unlock(&connections[i]->mutex_name);
	}
}

void NETWORK::out(char* name)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		pthread_mutex_lock(&connections[i]->mutex_name);
		if(!strcmp(connections[i]->char_name, name))
		{
			pthread_mutex_unlock(&connections[i]->mutex_name);
			connections[i]->kicked = true;
			return;
		}
		pthread_mutex_unlock(&connections[i]->mutex_name);
	}
}

void NETWORK::out_all()
{
	dbg_msg(3, "INFO", "Attempting to kick all players.");

	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		connections[i]->kicked = true;
	}

	dbg_msg(2, "SUCCESS", " -> Kicked all players.");
}

void NETWORK::send_gms(CONNECTION* conn)
{
	for(unsigned int i = 0; i < MAX_CONNS; i++)
	{
		if(connections[i]->active == false)
			continue;

		pthread_mutex_lock(&connections[i]->mutex_logged_in);

		if(!connections[i]->logged_in)
		{
			pthread_mutex_unlock(&connections[i]->mutex_logged_in);
			continue;
		}

		pthread_mutex_unlock(&connections[i]->mutex_logged_in);

		ADMIN* admin = adminmanager->get_admin(connections[i]->account_name);

		if(admin != NULL)
		{
			if(admin->level < 100)
			{
				char msg[512];
				sprintf_s(msg, sizeof(msg), " * %s (%s) - Adminlevel %d",
										connections[i]->char_name,
										admin->status,
										admin->level);

				conn->send_green_text(msg);
			}
		}
	}
}

int NETWORK::get_connections()
{
	return conns_count;
}

bool NETWORK::is_sock_rdy(int m_sock, int time)
{
	fd_set all_socks;
	fd_set read_socks;

	// Init the reader
	{
		// Add the server sock to all socks
		FD_ZERO(&all_socks);
		FD_SET(m_sock, &all_socks);
	}

	// Set the socks to read to all socks
	read_socks = all_socks;

	// Timeval
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = time;

	int clients_ready;

	// Check for connections and if we got one then react on it
	if((clients_ready = select(m_sock + 1, &read_socks, NULL, NULL, &timeout)) != 0)
	{
		// Error?
		if(clients_ready == -1)
		{
			Sleep(20);
			return false;
		}

		// Read new data and manage
		if(FD_ISSET(m_sock, &read_socks))
			return true;
	}

	return false;
}



// --------------

void NETWORK::cmd_reload_cmds()
{
	cmdmanager->reload_cmds();
}

void NETWORK::cmd_reload_cfg()
{
	cfg->load_cfg();
}

void NETWORK::cmd_reload_admins()
{
	adminmanager->reload_admins();
	reload_admins();
}

void NETWORK::cmd_reload_bans()
{
	banmanager->reload_bans();
}

void NETWORK::cmd_reload_ranks()
{
}

void NETWORK::cmd_out(char* name)
{
	if(get_account(name) == NULL)
	{
		dbg_msg(6, "WARNING", " -> %s isn't online.", name);
		return;
	}

	out(name);
}

void NETWORK::cmd_outall()
{
	send_global_notice("All players got kicked. (Admin command)");

	out_all();
}

void NETWORK::cmd_add_admin(char* name, int level)
{
	ADMIN* admin;

	if((admin = adminmanager->get_admin(name)))
	{
		if(admin->level == level)
		{
			dbg_msg(6, "WARNING", " -> %s already is an admin with level %d.", name, level);
			return;
		}

		adminmanager->remove_admin(name);
		adminmanager->add_admin(name, level);
	}
	else
	{
		adminmanager->add_admin(name, level);
	}

	// Reload admins
	reload_admins();
}

void NETWORK::cmd_remove_admin(char* name)
{
	ADMIN* admin;

	if((admin = adminmanager->get_admin(name)))
	{
		adminmanager->remove_admin(name);

		// Reload admins
		reload_admins();
		return;
	}

	dbg_msg(6, "WARNING", " -> %s is not an admin.", name);
}

void NETWORK::cmd_notice_gm(char* msg)
{
	char _msg[1024];

	sprintf_s(_msg, sizeof(_msg), "[GM chat] %s", msg);

	send_gm_notice(_msg);
}

void NETWORK::cmd_green_text_gm(char* msg)
{
	char _msg[1024];

	sprintf_s(_msg, sizeof(_msg), "[GM chat] %s", msg);

	send_gm_green_text(_msg);
}

void NETWORK::cmd_notice(char* msg)
{
	send_global_notice(msg);
}


void NETWORK::cmd_ban(char* name, unsigned int time)
{
	char* account_name = get_account(name);
	char* ip           = get_ip(name);

	if(account_name == NULL)
	{
		dbg_msg(6, "WARNING", " -> %s doesn't exist or isn't online.", name);
		return;
	}

	if(ip == NULL)
	{
		dbg_msg(6, "WARNING", " -> IP couldn't be retrieved.");
		return;
	}

	if(strlen(ip) > 15)
	{
		dbg_msg(6, "WARNING", " -> IP is invalid.");
		return;
	}

	if(banmanager->is_banned(account_name))
	{
		dbg_msg(6, "WARNING", " -> Account is already banned.");
		return;
	}


	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban(account_name, time, ip);

	send_private_notice(name, cfg->banned_account);

	out(name);
}

void NETWORK::cmd_ban_account(char* name, unsigned int time)
{
	if(strlen(name) > 32)
	{
		dbg_msg(6, "WARNING", " -> Accountname is too long.");
		return;
	}

	if(banmanager->is_banned(name))
	{
		dbg_msg(6, "WARNING", " -> Account already is banned.");
		return;
	}


	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban(name, time, "-");
}

void NETWORK::cmd_ban_ip(char* ip, unsigned int time)
{
	if(strlen(ip) > 15)
	{
		dbg_msg(6, "WARNING", " -> IP isn't valid.");
		return;
	}

	if(banmanager->is_banned_ip(ip))
	{
		dbg_msg(6, "WARNING", " -> IP already is banned.");
		return;
	}

	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban("-", time, ip);
}

void NETWORK::cmd_unban(char* name)
{
	if(!banmanager->is_banned(name))
	{
		dbg_msg(6, "WARNING", " -> Account isn't banned.");
		return;
	}

	banmanager->remove_ban(name);
}

void NETWORK::cmd_unban_ip(char* ip)
{
	if(strlen(ip) > 15)
	{
		dbg_msg(6, "WARNING", " -> IP isn't valid.");
		return;
	}

	if(!banmanager->is_banned_ip(ip))
	{
		dbg_msg(6, "WARNING", " -> IP isn't banned.");
		return;
	}

	banmanager->remove_ban_ip(ip);
}

void NETWORK::cmd_green_text(char* msg)
{
	send_global_green_text(msg);
}

void NETWORK::execute_cmd(char* cmd)
{
	// Notice GM
	if(str_cmp((char*)cmd, "notice_gm ", 10))
	{
		char msg[256];

		if(sscanf_s((char*)cmd, "notice_gm %s",
								msg, sizeof(msg))
		   == 1)
			cmd_notice_gm((char*)&cmd[10]);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Notice
	else if(str_cmp((char*)cmd, "notice ", 7))
	{
		char msg[256];

		if(sscanf_s((char*)cmd, "notice %s",
								msg, sizeof(msg))
		   == 1)
			cmd_notice((char*)&cmd[7]);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// GText
	else if(str_cmp((char*)cmd, "green_text ", 11))
	{
		char msg[256];

		if(sscanf_s((char*)cmd, "green_text %s",
								msg, sizeof(msg))
		   == 1)
			cmd_green_text((char*)&cmd[11]);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// GText GM
	else if(str_cmp((char*)cmd, "green_text_gm ", 14))
	{
		char msg[256];

		if(sscanf_s((char*)cmd, "green_text_gm %s",
								msg, sizeof(msg))
		   == 1)
			cmd_green_text_gm((char*)&cmd[14]);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Reload admins
	else if(str_cmp((char*)cmd, "add_admin ", 10))
	{
		unsigned char name[33];
		int _level = 0;

		if(sscanf_s((char*)cmd, "add_admin %s %d",
								name, sizeof(name),
								&_level)
		   == 2)
			cmd_add_admin((char*)name, _level);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Reload admins
	else if(str_cmp((char*)cmd, "remove_admin ", 13))
	{
		char name[33];

		if(sscanf_s((char*)cmd, "remove_admin %s",
								name, sizeof(name))
		   == 1)
			cmd_remove_admin(name);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Ban
	else if(str_cmp((char*)cmd, "ban ", 4))
	{
		char name[33];
		unsigned int time = 0;

		if(sscanf_s((char*)cmd, "ban %s %d",
								name, sizeof(name),
								&time)
		   == 1)
			cmd_ban(name, time);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Ban account
	else if(str_cmp((char*)cmd, "ban_account ", 12))
	{
		char name[33];
		unsigned int time = 0;

		if(sscanf_s((char*)cmd, "ban_account %s %d",
								name, sizeof(name),
								&time)
		   == 1)
			cmd_ban_account(name, time);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Ban ip
	else if(str_cmp((char*)cmd, "ban_ip ", 7))
	{
		char name[33];
		unsigned int time = 0;

		if(sscanf_s((char*)cmd, "ban_ip %s",
								name, sizeof(name),
								&time)
		   == 1)
			cmd_ban_ip(name, time);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Unban
	else if(str_cmp((char*)cmd, "unban ", 6))
	{
		char name[33];

		if(sscanf_s((char*)cmd, "unban %s",
								name, sizeof(name))
		   == 1)
			cmd_unban(name);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Unban ip
	else if(str_cmp((char*)cmd, "unban_ip ", 9))
	{
		char name[33];

		if(sscanf_s((char*)cmd, "unban_ip %s",
								name, sizeof(name))
		   == 1)
			cmd_unban_ip(name);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Out
	else if(str_cmp((char*)cmd, "out ", 4))
	{
		char name[33];

		if(sscanf_s((char*)cmd, "out %s",
								name, sizeof(name))
		   == 1)
			cmd_out(name);
		else
			dbg_msg(6, "WARNING", " -> Invalid input argument.");
	}
	// Outall
	else if(str_cmp((char*)cmd, "outall", 6))
		cmd_outall();
	// Reload cmds
	else if(str_cmp((char*)cmd, "reload_cmds", 11))
		cmd_reload_cmds();
	// Reload cfg
	else if(str_cmp((char*)cmd, "reload_cfg", 10))
		cmd_reload_cfg();
	// Reload admins
	else if(str_cmp((char*)cmd, "reload_admins", 10))
		cmd_reload_admins();
	// Reload bans
	else if(str_cmp((char*)cmd, "reload_bans", 10))
		cmd_reload_bans();
	// Reload ranks
	else if(str_cmp((char*)cmd, "reload_ranks", 10))
		cmd_reload_ranks();
	// Invalid command
	else
		dbg_msg(6, "WARNING", " -> Invalid input command.");
}