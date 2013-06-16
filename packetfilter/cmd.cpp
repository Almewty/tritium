#include "stdafx.h"
#include "engine.hpp"
#include "cmd.hpp"
#include "cfg.hpp"

// Extern globals
extern CFGMANAGER* cfg;

// Intern global
CMDMANAGER* cmdmanager = 0;

CMD::CMD(char _name[])
{
	sprintf_s(name, sizeof(name), "%s", _name);
	level = GetPrivateProfileInt("cmds", name, 0, cfg->cmdfile);
	if(str_cmp(_name, "version", 7))
		level = 0;
}

CMD::~CMD()
{
}


// CMDMANAGER

CMDMANAGER::CMDMANAGER()
{
	pthread_mutex_init(&mutex_cmds, NULL);

	load_cmds();
}

CMDMANAGER::~CMDMANAGER()
{
	// Delete the cmd objects
	for(unsigned int i = 0; i < cmds.size(); i++)
		delete cmds[i];

	pthread_mutex_destroy(&mutex_cmds);
}

int CMDMANAGER::get_req_level(char* _cmd)
{
	if(_cmd == NULL)
		return -1;

	pthread_mutex_lock(&mutex_cmds);
	for(unsigned int i = 0; i < cmds.size(); i++)
	{
		if(strlen(cmds[i]->name) == strlen(_cmd) &&
		   str_cmp(cmds[i]->name, _cmd, strlen(_cmd)))
		{
			pthread_mutex_unlock(&mutex_cmds);
			return cmds[i]->level;
		}
	}

	pthread_mutex_unlock(&mutex_cmds);
	return GetPrivateProfileInt("flyff_cmds", _cmd, 0, cfg->cmdfile);
}

void CMDMANAGER::reload_cmds()
{
	unload_cmds();
	load_cmds();
}

void CMDMANAGER::load_cmds()
{
	// Info about loading
	dbg_msg(3, "INFO", "Attempting to load the CMDs.");

	int begin = clock();

	pthread_mutex_lock(&mutex_cmds);

	cmds.push_back(new CMD("commands"));
	cmds.push_back(new CMD("players"));
	cmds.push_back(new CMD("rates"));
	cmds.push_back(new CMD("version"));
	cmds.push_back(new CMD("time"));
	cmds.push_back(new CMD("isgm"));
	cmds.push_back(new CMD("gmlist"));

	cmds.push_back(new CMD("status"));

	cmds.push_back(new CMD("notice"));
	cmds.push_back(new CMD("notice_gm"));
	cmds.push_back(new CMD("notice_anonymous"));

	cmds.push_back(new CMD("green_text"));
	cmds.push_back(new CMD("green_text_gm"));

	cmds.push_back(new CMD("get_ip"));
	cmds.push_back(new CMD("get_account"));

	cmds.push_back(new CMD("banlist"));
	cmds.push_back(new CMD("adminlist"));

	cmds.push_back(new CMD("ban"));
	cmds.push_back(new CMD("ban_account"));
	cmds.push_back(new CMD("ban_ip"));
	cmds.push_back(new CMD("unban"));
	cmds.push_back(new CMD("unban_ip"));
	cmds.push_back(new CMD("outall"));
	cmds.push_back(new CMD("add_admin"));
	cmds.push_back(new CMD("remove_admin"));

	cmds.push_back(new CMD("reload_cmds"));
	cmds.push_back(new CMD("reload_cfg"));
	cmds.push_back(new CMD("reload_admins"));
	cmds.push_back(new CMD("reload_bans"));
	cmds.push_back(new CMD("reload_ranks"));

	cmds.push_back(new CMD("get_connections"));

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> %d CMDs loaded from '%s' in %d msec.", cmds.size(), cfg->cmdfile, clock() - begin);

	pthread_mutex_unlock(&mutex_cmds);
}

void CMDMANAGER::unload_cmds()
{
	pthread_mutex_lock(&mutex_cmds);

	for(unsigned int i = 0; i < cmds.size(); i++)
		delete cmds[i];

	cmds.clear();

	pthread_mutex_unlock(&mutex_cmds);
}