#include "stdafx.h"
#include "engine.hpp"
#include "admin.hpp"
#include "cfg.hpp"

// Extern globals
extern CFGMANAGER* cfg;

// Intern global
ADMINMANAGER* adminmanager = 0;

// Admin object
ADMIN::ADMIN(char* _name, int _level)
:
// level = _level
level(_level)
{
	// Admin name ftw
	sprintf_s(name,   sizeof(name),   "%s", _name);

	// Default status is 'Online'
	sprintf_s(status, sizeof(status), "%s", "Online");
}

ADMIN::~ADMIN()
{
}


// Admin manager
ADMINMANAGER::ADMINMANAGER()
{
	pthread_mutex_init(&mutex_admins, NULL);

	load_admins();
}

ADMINMANAGER::~ADMINMANAGER()
{
	pthread_mutex_lock(&mutex_admins);
	unload_admins();
	pthread_mutex_unlock(&mutex_admins);

	pthread_mutex_destroy(&mutex_admins);
}

void ADMINMANAGER::load_admins()
{
	// Info about loading
	dbg_msg(3, "INFO", "Attempting to load the admins.");

	int begin = clock();

	FILE* adminlist;

	if(fopen_s(&adminlist, cfg->adminlist, "r") != 0)
	{
		dbg_msg(4, "ERROR", " -> Couldn't open '%s'.", cfg->adminlist);
		return;
	}

	char _name[33];
	int _rights;

	while(fscanf_s(adminlist, "%s %d\n",
							  _name, sizeof(_name),
							  &_rights)
							
							  == 2)
	{
		admins.push_back(new ADMIN(_name, _rights));
	}

	fclose(adminlist);


	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> %d admins loaded from '%s' in %d msec.", admins.size(), cfg->adminlist, clock() - begin);
}

void ADMINMANAGER::unload_admins()
{
	// Delete the admin objects
	for(unsigned int i = 0; i < admins.size(); i++)
		delete admins[i];

	admins.clear();
}

void ADMINMANAGER::reload_admins()
{
	pthread_mutex_lock(&mutex_admins);
	unload_admins();
	load_admins();
	pthread_mutex_unlock(&mutex_admins);
}

void ADMINMANAGER::add_admin(char _account_name[], int _level)
{
	dbg_msg(3, "INFO", "Attempting to grant account '%s' the adminlevel %d.", _account_name, _level);


	pthread_mutex_lock(&mutex_admins);
	admins.push_back(new ADMIN(_account_name, _level));
	pthread_mutex_unlock(&mutex_admins);

	FILE* adminlist;

	while(fopen_s(&adminlist, cfg->adminlist, "a") != 0)
		Sleep(1);

	char buf[1024];
	sprintf_s(buf, sizeof(buf), "%s %d\n", _account_name, _level);
	fputs(buf, adminlist);

	fclose(adminlist);

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> Account '%s' became admin level %d.", _account_name, _level);
}

void ADMINMANAGER::remove_admin(char _name[])
{
	dbg_msg(3, "INFO", "Attempting to revoke account '%s' it's adminlevel.", _name);


	pthread_mutex_lock(&mutex_admins);
	// Erase from vector
	for(std::vector<ADMIN*>::iterator i = admins.begin();
		i != admins.end();
		i++)
	{
		if(strlen((*i)->name) == strlen(_name) &&
		   str_cmp((*i)->name, _name, strlen(_name)))
		{
			delete (*i);
			admins.erase(i);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_admins);


	// Change banlist
	FILE* adminlist;

	while(fopen_s(&adminlist, cfg->adminlist, "w") != 0)
		Sleep(1);

	char buf[1024];

	pthread_mutex_lock(&mutex_admins);
	for(unsigned int i = 0; i < admins.size(); i++)
	{
		sprintf_s(buf, sizeof(buf), "%s %d\n", admins[i]->name, admins[i]->level);
		fputs(buf, adminlist);
	}
	pthread_mutex_unlock(&mutex_admins);

	fclose(adminlist);

	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> Account '%s' hasn't got an adminlevel anymore.", _name);
}

ADMIN* ADMINMANAGER::get_admin(char* _name)
{
	pthread_mutex_lock(&mutex_admins);
	for(unsigned int i = 0; i < admins.size(); i++)
	{
		if(strlen(admins[i]->name) == strlen(_name) &&
		   str_cmp(admins[i]->name, _name, strlen(_name)))
		{
			pthread_mutex_unlock(&mutex_admins);
			return admins[i];
		}
	}
	pthread_mutex_unlock(&mutex_admins);

	return NULL;
}