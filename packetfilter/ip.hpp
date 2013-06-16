#ifndef IP_H
#define IP_H

class IP
{
public:
	IP(int _times, int _clock, char _ip[]);
	~IP();

	int times;
	int clock;
	char ip[16];
};

class IPMANAGER
{
public:
	IPMANAGER();
	~IPMANAGER();

	// Check how many ip connections in the last minute
	int times_connected(char _ip[]);

	// Add an IP connection
	void add_ip(char _ip[]);

	void clean_ips();

	// Mutex
	pthread_mutex_t mutex_ips;
	
	// Vector of all bans
	std::vector<IP*> ips;
};

#endif