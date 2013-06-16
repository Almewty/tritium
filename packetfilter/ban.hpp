#ifndef BAN_H
#define BAN_H

#include "stdafx.h"

class BAN
{
public:
	BAN(char _account_name[], int _time, char _ip[]);
	~BAN();

	char account_name[17];
	int time;
	char ip[16];
};

class BANMANAGER
{
public:
	BANMANAGER();
	~BANMANAGER();

	// Check if someone is banned
	bool is_banned(char _account_name[]);
	bool is_banned_ip(char _ip[]);

	// Add a ban
	void add_ban(char _account_name[], int _time, char _ip[]);
	// Remove a ban
	void remove_ban(char name[]);
	void remove_ban_ip(char ip[]);

	void reload_bans();

	// Mutex
	pthread_mutex_t mutex_bans;
	
	// Vector of all bans
	std::vector<BAN*> bans;


private:



	// ---- Funcs

	void load_bans();
	void unload_bans();
};

#endif