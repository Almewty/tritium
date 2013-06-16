#ifndef CONNECTION_H
#define CONNECTION_H

#include "stdafx.h"

class NETWORK;
class BANMANAGER;
class ADMINMANAGER;
class ADMIN;
class CMDMANAGER;
class CFGMANAGER;

class CONNECTION
{
public:
	CONNECTION();
	~CONNECTION();

	bool activate(int _c_sock);
	void deactivate();

	// Active?!?
	bool active;

	// Charname
	bool logged_in;
	char char_name[17];
	char account_name[33];

	// Mutex
	pthread_mutex_t mutex_ip;
	pthread_mutex_t mutex_name;
	pthread_mutex_t mutex_logged_in;

	// Mutex
	//pthread_mutex_t mutex_id;

	// IP
	char* ip;

	// Kicked
	bool kicked;

	// Authed
	int level;

	// Sends
	void send_notice(char msg[]);
	void send_notice_as(std::string& msg);
	void send_green_text(char msg[]);


	// Command, needs to be public
	void auth(int _level);

	// LOOP!!!! :D
	void loop();


	bool n_notice;
	bool n_gnotice;

	// Client socket descriptor
	int c_sock;

private:

	// Admin object
	ADMIN* admin;
	//int s_sock;
	
	// Own socket descriptor
	int m_sock;
	// Own socket addr
	sockaddr_in m_addr;

	fd_set all_socks;
	fd_set read_socks;

	// Reason for a drop
	char reason[1024];
	bool drop;


	bool connect(char ip[], int port);

	bool handle_client_packet(unsigned char packet[], int bytes);
	bool handle_server_packet(unsigned char packet[], int bytes);

	// Packet stuff
	bool send_packet(char* packet, int bytes, int sock);


	// ---------- FlyFF STUFF -----------

	// Session id
	unsigned char session_id[5];
	bool session_id_got;

	// Timers

	 // start
	 int starttime;
	 // Welcome msg
	 bool welcome_sent;
	 // buffed
	 int last_buffed;
	 // Facechange
	 int last_facechange;
	 // Shout stuff
	 int shouted;
	 int last_shout;
	 // Chat stuff
	 int chatted;
	 int last_chat;
	 // Drop stuff
	 int dropped;
	 int last_drop;
	 // Item use
	 int item_used;
	 int last_item;
	 // Upgrading
	 int upgraded;
	 int last_upgrade;
	 // Timout stuff
	 int last_packet;


	// 24 attack block
	int attacks_24;


    // Some stats
	/*bool destroyed_item;
	bool dropped_item;
	bool sold_item;
	bool bagged_item;
	bool banked_item;
	bool guildbanked_item;*/

	// Deal with connection disconnect
	//void disconnect(int id);

	// Deal with data
	//void newData(int id);
	// Handle new data
	//void handleData(int id, char data[512]);

	// Cmds
	void cmd_commands();
	void cmd_players();
	void cmd_version();
	void cmd_rates();
	void cmd_time();

	// GM stuff
	void cmd_isgm(char* name);
	void cmd_gmlist();

	// Banning
	void cmd_banlist();
	void cmd_ban(char* name, unsigned int time);
	void cmd_ban_account(char* name, unsigned int time);
	void cmd_ban_ip(char* ip, unsigned int time);
	void cmd_unban(char* name);
	void cmd_unban_ip(char* ip);

	// Notice
	void cmd_green_text(char* msg);
	void cmd_green_text_gm(char* msg);
	void cmd_notice(char* msg);
	void cmd_notice_gm(char* msg);
	void cmd_notice_anonymous(char* msg);

	// Get stuff
	void cmd_get_ip(char* name);
	void cmd_get_account(char* name);
	void cmd_get_connections();

	// KICK! :D
	void cmd_outall();

	// Admin stuff
	void cmd_adminlist();
	void cmd_auth();
	void cmd_add_admin(char* name, int level);
	void cmd_remove_admin(char* name);

	// Config stuff
	void cmd_reload_cmds();
	void cmd_reload_cfg();
	void cmd_reload_admins();
	void cmd_reload_bans();
	void cmd_reload_ranks();

	// Misc
	void cmd_status(char* status);
	void cmd_schinken();
	void cmd_kyu();
	void cmd_tom_klingt_wie_till();

	//void send_unequip(unsigned char id);


	// Logging
	void log(const char* sys, const char* fmt, ...);
	void log_ip();
};

#endif
