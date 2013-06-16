#include "stdafx.h"
#include "engine.hpp"
#include "ip.hpp"
#include "cfg.hpp"

// Extern globals
extern CFGMANAGER* cfg;

// Intern global
IPMANAGER* ipmanager = 0;

// The IP itself
IP::IP(int _times, int _clock, char _ip[])
{
	sprintf_s(ip, sizeof(ip), "%s", _ip);
	times = _times;
	clock = _clock;
}

IP::~IP()
{
}


// IPMANAGER
IPMANAGER::IPMANAGER()
{
	pthread_mutex_init(&mutex_ips, NULL);
}

IPMANAGER::~IPMANAGER()
{
	pthread_mutex_destroy(&mutex_ips);
}

void IPMANAGER::clean_ips()
{
	pthread_mutex_lock(&mutex_ips);

check:
	// Erase from vector
	for(std::vector<IP*>::iterator i = ips.begin();
		i != ips.end();
		i++)
	{
		(*i)->times -= (clock() - (*i)->clock) / (60000/cfg->ip_connections_per_minute);

		if((*i)->times <= 0)
		{
			delete (*i);
			ips.erase(i);
			goto check;
		}

		(*i)->clock = clock() - ((clock() - (*i)->clock) % (60000/cfg->ip_connections_per_minute));
	}

	pthread_mutex_unlock(&mutex_ips);
}

int IPMANAGER::times_connected(char _ip[])
{
	if(_ip == NULL)
		return 0;

	pthread_mutex_lock(&mutex_ips);

	for(std::vector<IP*>::iterator i = ips.begin();
		i != ips.end();
		i++)
	{
		if(strlen((*i)->ip) == strlen(_ip) &&
		   str_cmp((*i)->ip, _ip, strlen(_ip)))
		{
			int num;


			(*i)->times -= (clock() - (*i)->clock) / (60000/cfg->ip_connections_per_minute);

			if((*i)->times <= 0)
			{
				delete (*i);
				ips.erase(i);
				
				num = 0;
			}
			else
			{
				(*i)->clock = clock() - ((clock() - (*i)->clock) % (60000/cfg->ip_connections_per_minute));
				num = (*i)->times;
			}

			pthread_mutex_unlock(&mutex_ips);
			return num;
		}
	}

	pthread_mutex_unlock(&mutex_ips);


	return 0;
}

void IPMANAGER::add_ip(char _ip[])
{
	if(_ip == NULL)
		return;

	// Increase existing counter
	pthread_mutex_lock(&mutex_ips);
	for(unsigned int i = 0; i < ips.size(); i++)
	{
		if(strlen(ips[i]->ip) == strlen(_ip) &&
		   str_cmp(ips[i]->ip, _ip, strlen(_ip)))
		{
			ips[i]->times++;
			pthread_mutex_unlock(&mutex_ips);
			return;
		}
	}

	// Else add new one
	ips.push_back(new IP(1, clock(), _ip));
	pthread_mutex_unlock(&mutex_ips);
}
