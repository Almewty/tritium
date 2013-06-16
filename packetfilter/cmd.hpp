#ifndef CMD_H
#define CMD_H

#include "stdafx.h"

class CMD
{
public:
	CMD(char _name[]);
	~CMD();

	char name[128];
	int level;
};

class CMDMANAGER
{
public:
	CMDMANAGER();
	~CMDMANAGER();

	// Get the required level
	int get_req_level(char* _cmd);

	// Reload cmds
	void reload_cmds();

	// Mutex
	pthread_mutex_t mutex_cmds;
	
	// Vector of all bans
	std::vector<CMD*> cmds;


private:



	// ---- Funcs

	void load_cmds();
	void unload_cmds();
};

#endif