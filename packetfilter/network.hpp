#ifndef NETWORK_H
#define NETWORK_H

#include "stdafx.h"

class CONNECTION;

// NETWORK class
class NETWORK
{
public:
	NETWORK();
	~NETWORK();


	pthread_mutex_t mutex_count;

	// Send data to a client
	//void send(int id, char * data);

	// Amount of people connected
	int players_count;
	int conns_count;

	// Add a connection
	void add_connection(int _nc_sock);

	// Get account by char name
	char* get_account(char _char_name[]);

	// crash! haha
	bool crashed;

	// Auth sock
	//int a_sock;


	// Reload the admins
	void reload_admins();

	// Get the connection itself
	CONNECTION* get_connection(char* name);
	// Get ip
	char* get_ip(char* name);
	int get_ip_connections(char* ip);
	// Send notice
	void send_global_notice(char* msg);
	void send_gm_notice(char* msg);
	void send_private_notice(char* name, char* msg);
	// Send green text
	void send_global_green_text(char* msg);
	void send_gm_green_text(char* msg);
	void send_private_green_text(char* name, char* msg);
	// Kick
	void out(char* name);
	void out_all();
	// Auth
	int get_auth(char* name);
	// Get connections
	int get_connections();
	// Send gmlist
	void send_gms(CONNECTION* conn);

	// Network loop
	void net_loop();
	// Window loop
	void window_loop();


	// Notice stuff
	char n_notice[500];
	char n_gnotice[500];


	// ---------- Some statistic stuff
	int starttime;

	// ---------- Window stuff

	// Unser fenster
	HWND hWnd;

	// Users online
	int win_players_count;
	int win_connections_count;
	int win_admins_count;
	int win_bans_count;
	int win_cmds_count;

	// execute cmd
	void execute_cmd(char* cmd);

private:

	// Is a socket ready?
	bool is_sock_rdy(int m_sock, int time);

	// --------------------------

	// Main loop
	void loop();

	pthread_mutex_t mutex_addconn;
	pthread_mutex_t mutex_conns;

	//std::vector<CONNECTION*> connections;
	CONNECTION* connections[MAX_CONNS];

	// Some values for the server
	/*enum
	{
		NETWORK_MAXCONNECTIONS = 16,
		NETWORK_PORT           = 9998
	};*/

	// Own socket descriptor
	int m_sock;

	// Socket for new conns
	int nc_sock;

	// Own socket addr
	sockaddr_in m_addr;

	// -------- Funcs ---------

	// Check for new connections
	bool listen();

	// Wait till new one is here
	int accept();

	// --------- Auth part!

	// Connect to the auth server
	//bool connect_auth(char ip[], int port);

	//int last_auth;


	// --------- Game part!

	int last_notice;


	// --------- Window part!

	// Zeit des letzten updates
	int last_update;
	int last_clean;


	// ---------------

	// Config stuff
	void cmd_reload_cmds();
	void cmd_reload_cfg();
	void cmd_reload_admins();
	void cmd_reload_bans();
	void cmd_reload_ranks();

	// Admin
	void cmd_add_admin(char* name, int level);
	void cmd_remove_admin(char* name);

	// KICK! :D
	void cmd_outall();
	void cmd_out(char* name);

	// Notice
	void cmd_green_text(char* msg);
	void cmd_green_text_gm(char* msg);
	void cmd_notice(char* msg);
	void cmd_notice_gm(char* msg);

	// Banning
	void cmd_ban(char* name, unsigned int time);
	void cmd_ban_account(char* name, unsigned int time);
	void cmd_ban_ip(char* ip, unsigned int time);
	void cmd_unban(char* name);
	void cmd_unban_ip(char* ip);
};

#endif