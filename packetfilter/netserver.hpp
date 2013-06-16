#ifndef NETWORK_NETSERVER_H
#define NETWORK_NETSERVER_H

class GAME;
class PACKETMANAGER;

// Netserver class
class NETSERVER
{
public:
	NETSERVER();
	~NETSERVER();

	// Tick
	void tick();

	// Send data to a client
	void send(int id, char * data);

	PACKETMANAGER* packetmanager;

private:

	// Some values for the server
	enum
	{
		NETWORK_MAXCONNECTIONS = 16,
		NETWORK_PORT           = 9998
	};

	// Own socket descriptor
	int m_sock;
	// Own socket addr
	sockaddr_in m_addr;

	// Maximum sock number
	int max_sock;

	// Client socks
	int client_socks[NETWORK_MAXCONNECTIONS];

	// Bit fields
	fd_set all_socks;
	fd_set read_socks;

	// -------- Funcs ---------

	// Check for new data
	void check_socks();

	// Deal with new connection
	void connect();
	// Deal with connection disconnect
	void disconnect(int id);

	// Deal with data
	void newData(int id);
	// Handle new data
	void handleData(int id, char data[512]);

};

#endif