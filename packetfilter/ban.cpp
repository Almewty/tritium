#include "stdafx.h"
#include "engine.hpp"
#include "ban.hpp"
#include "connection.hpp"
#include "cfg.hpp"

// Extern globals
extern CFGMANAGER*   cfg;

// Intern global
BANMANAGER* banmanager = 0;

// The BAN itself
BAN::BAN(char _account_name[], int _time, char _ip[])
{
	sprintf_s(account_name, sizeof(account_name), "%s", _account_name);
	time = _time;
	sprintf_s(ip, sizeof(ip), "%s", _ip);
}

BAN::~BAN()
{
}


// BANMANAGER
BANMANAGER::BANMANAGER()
{
	pthread_mutex_init(&mutex_bans, NULL);

	load_bans();
}

BANMANAGER::~BANMANAGER()
{
	pthread_mutex_lock(&mutex_bans);
	unload_bans();
	pthread_mutex_unlock(&mutex_bans);

	pthread_mutex_destroy(&mutex_bans);
}


void BANMANAGER::load_bans()
{
	// Info about loading
	dbg_msg(3, "INFO", "Attempting to load the bans.");

	int begin = clock();

	FILE* banlist;

	if(fopen_s(&banlist, cfg->banlist, "r") != 0)
	{
		dbg_msg(4, "ERROR", " -> Couldn't open '%s'.", cfg->banlist);
		return;
	}

	char _account_name[17];
	int _time;
	char _ip[16];
	while(fscanf_s(banlist, "%s %d %s\n",
							_account_name, sizeof(_account_name),
							&_time,
							_ip, sizeof(_ip))
							
							== 3)
	{
		bans.push_back(new BAN(_account_name, _time, _ip));
	}

	fclose(banlist);

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> %d bans loaded from '%s' in %d msec.", bans.size(), cfg->banlist, clock() - begin);
}

void BANMANAGER::unload_bans()
{
	// Delete the admin objects
	for(unsigned int i = 0; i < bans.size(); i++)
		delete bans[i];

	bans.clear();
}

void BANMANAGER::reload_bans()
{
	pthread_mutex_lock(&mutex_bans);
	unload_bans();
	load_bans();
	pthread_mutex_unlock(&mutex_bans);
}

bool BANMANAGER::is_banned(char _account_name[])
{
	if(_account_name == NULL)
		true;

	pthread_mutex_lock(&mutex_bans);
	for(unsigned int i = 0; i < bans.size(); i++)
	{
		if(strlen(bans[i]->account_name) == strlen(_account_name) &&
		   str_cmp(bans[i]->account_name, _account_name, strlen(_account_name)))
		{
			pthread_mutex_unlock(&mutex_bans);
			return true;
		}
	}
	pthread_mutex_unlock(&mutex_bans);

	return false;
}

bool BANMANAGER::is_banned_ip(char _ip[])
{
	if(_ip == NULL)
		return true;

	pthread_mutex_lock(&mutex_bans);
	for(unsigned int i = 0; i < bans.size(); i++)
	{
		if(str_cmp(bans[i]->ip, _ip, strlen(bans[i]->ip)))
		{
			pthread_mutex_unlock(&mutex_bans);
			return true;
		}
	}

	pthread_mutex_unlock(&mutex_bans);
	return false;
}

void BANMANAGER::add_ban(char _account_name[], int _time, char _ip[])
{
	dbg_msg(3, "INFO", "Attempting to ban account '%s' with IP '%s'.", _account_name, _ip);

	pthread_mutex_lock(&mutex_bans);
	bans.push_back(new BAN(_account_name, _time, _ip));
	pthread_mutex_unlock(&mutex_bans);

	FILE* banlist;

	while(fopen_s(&banlist, cfg->banlist, "a") != 0)
		Sleep(1);

	char buf[1024];
	sprintf_s(buf, sizeof(buf), "%s %d %s\n", _account_name, _time, _ip);
	fputs(buf, banlist);

	fclose(banlist);

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> Account '%s' with IP '%s' got banned.", _account_name, _ip);
}

void BANMANAGER::remove_ban(char name[])
{
	dbg_msg(3, "INFO", "Attempting to unban the account '%s'.", name);


	pthread_mutex_lock(&mutex_bans);
	// Erase from vector
	for(std::vector<BAN*>::iterator i = bans.begin();
		i != bans.end();
		i++)
	{
		if(strlen((*i)->account_name) == strlen(name) &&
		   str_cmp((*i)->account_name, name, strlen(name)))
		{
			delete (*i);
			bans.erase(i);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_bans);


	// Change banlist
	FILE* banlist;

	while(fopen_s(&banlist, cfg->banlist, "w") != 0)
		Sleep(1);

	char buf[1024];

	pthread_mutex_lock(&mutex_bans);
	for(unsigned int i = 0; i < bans.size(); i++)
	{
		sprintf_s(buf, sizeof(buf), "%s %d %s\n", bans[i]->account_name, bans[i]->time, bans[i]->ip);
		fputs(buf, banlist);
	}
	pthread_mutex_unlock(&mutex_bans);

	fclose(banlist);

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> Account '%s' got unbanned.", name);
}

void BANMANAGER::remove_ban_ip(char ip[])
{
	dbg_msg(3, "INFO", "Attempting to unban the IP '%s'.", ip);


	pthread_mutex_lock(&mutex_bans);

	// Erase from vector
	for(std::vector<BAN*>::iterator i = bans.begin();
		i != bans.end();
		i++)
	{
		if(strlen((*i)->ip) == strlen(ip) &&
		   str_cmp((*i)->ip, ip, strlen(ip)))
		{
			bans.erase(i);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_bans);


	// Change banlist
	FILE* banlist;

	while(fopen_s(&banlist, cfg->banlist, "w") != 0)
		Sleep(1);

	char buf[1024];

	pthread_mutex_lock(&mutex_bans);
	for(unsigned int i = 0; i < bans.size(); i++)
	{
		sprintf_s(buf, sizeof(buf), "%s %d %s\n", bans[i]->account_name, bans[i]->time, bans[i]->ip);
		fputs(buf, banlist);
	}
	pthread_mutex_unlock(&mutex_bans);

	fclose(banlist);


	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> IP '%s' got unbanned.", ip);
}