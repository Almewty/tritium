#include "stdafx.h"
#include "netserver.hpp"
#include "gamecontroller.hpp"
#include "game.hpp"
#include "player.hpp"
#include "config.hpp"

// Winsock
#include <winsock.h>

// Packet
#include "../Thilo_Shared/packet.hpp"
#include "packetmanager.hpp"


NETSERVER::NETSERVER()
{
	// Packet manager
	packetmanager = new PACKETMANAGER();

	// Winsock initialization
	{
		// Set up winsock.dll
		WSADATA wsaData;
		// Startup with version 2.0
		if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw(EXCEPT("Couldn't initialize winsock."));

		// Console
		dbg_msg("SOCKET", "Winsock initialized.");
	}

	// Creating socket
	{
		// Set m_sock
		m_sock = socket(AF_INET, SOCK_STREAM, 0);

		// Check if socket creating is successfull
		if(m_sock == 0)
			throw(EXCEPT("Couldn't create server socket. (%d)", WSAGetLastError()));

		// Console
		dbg_msg("SOCKET", "Server socket created. Descriptor: %d", m_sock);
	}

	// Bind socket to port
	{
		// Init m_addr
		m_addr.sin_family      = AF_INET;
		m_addr.sin_addr.s_addr = INADDR_ANY;
		m_addr.sin_port        = htons(config.port);

		// Bind
		if(bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
			throw(EXCEPT("Couldn't bind server socket to port %d. (%d)", config.port, WSAGetLastError()));

		// Console
		dbg_msg("SOCKET", "Server socket bound to port %d.", config.port);
	}

	// Listen to server socket
	{
		// Listen
		if(listen(m_sock, config.max_clients) == -1)
			throw(EXCEPT("Coultn't start listening to server socket. (%d)", WSAGetLastError()));

		// Console
		dbg_msg("SOCKET", "Began listening to server socket.");
	}

	// Init the reader
	{
		// Set the max sock
		max_sock = m_sock;

		// Set all client socks to -1 (off)
		for(unsigned int i = 0; i < config.max_clients; i++)
			client_socks[i] = -1;

		// Zero the memory of the main bit field
		FD_ZERO(&all_socks);

		// Add the server sock to all socks
		FD_SET(m_sock, &all_socks);
	}
}

NETSERVER::~NETSERVER()
{
	delete packetmanager;

	// Close the own socket
	closesocket(m_sock);
	dbg_msg("SOCKET", "Closed server socket.", m_sock);

	// Close the client sockets
	for(unsigned int i = 0; i < config.max_clients; i++)
		if(client_socks[i] != -1)
			closesocket(client_socks[i]);

	// Cleanup WSA
	WSACleanup();
	dbg_msg("SOCKET", "Winsock cleaned up.");
}

void NETSERVER::tick()
{
	// Check the socks
	check_socks();
}

void NETSERVER::check_socks()
{
	// Set the socks to read to all socks
	read_socks = all_socks;

	// Timeval
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int clients_ready;

	// Check for connections and if we got one then react on it
	if(clients_ready = select(max_sock + 1, &read_socks, NULL, NULL, &timeout) != 0)
	{
		// Error?
		if(clients_ready == -1)
			throw(EXCEPT("Couldn't check for new data with 'select()'. (%d)", WSAGetLastError()));

		// Check for new connection
		if(FD_ISSET(m_sock, &read_socks))
		{
			// We got a new connection
			connect();

			// Return if no clients which are ready are left
			if(--clients_ready == 0)
				return;
		}

		// Check for new data
		for(unsigned int i = 0; i < config.max_clients; i++)
			// Client exists?
			if(client_socks[i] != -1)
				// New data?
				if(FD_ISSET(client_socks[i], &read_socks))
				{
					// Read new data and manage
					newData(i);

					// Return if no more data is here
					if(--clients_ready == 0)
						return;
				}
	}
}

void NETSERVER::connect()
{
	// Is needed later on
	unsigned int i;

	// Set the client descriptor into client_socks
	for(i = 0; i < config.max_clients; i++)
		if(client_socks[i] == -1)
		{
			// Get the size and write into int
			int addr_size = sizeof(m_addr);

			// Set the client sock
			client_socks[i] = accept(m_sock, (sockaddr*)&m_addr, &addr_size);

			// Break out of the loop
			break;
		}

	// Error?
	if(client_socks[i] <= 0)
		throw(EXCEPT("Couldn't accept new connection. (%d)", WSAGetLastError()));

	// Add the new socket descriptor to all socket descriptors
	FD_SET(client_socks[i], &all_socks);

	// Actualize the new highest descriptor
	if(client_socks[i] > max_sock)
		max_sock = client_socks[i];

	// Console
	if(i == config.max_clients)
		dbg_msg("NETWORK", "New client wasn't accepted due to a full server.");
	else
		dbg_msg("NETWORK", "Client %d connected. (Socket %d)", i, client_socks[i]);

	// Connection
	game.onPlayerConnect(i);
}

void NETSERVER::disconnect(int id)
{
	// Close connection
	closesocket(client_socks[id]);

	// Delete from all socks
	FD_CLR(client_socks[id], &all_socks);

	// Set to -1
	client_socks[id] = -1;

	// Disconnection
	dbg_msg("NETWORK", "Client %d disconnected.", id);

	// Disconnection
	game.onPlayerDisconnect(id);
}

void NETSERVER::newData(int id)
{
	// Create data array
	char data[512];

	// Reset memory of data
	memset(data, 0, 512);

	// Recieve data
	int status = recv(client_socks[id], data, 511, 0);

	// Check for some cases
	if(status == -1)
	{
		if(WSAGetLastError() == 10054)
		{
			disconnect(id);
			return;
		}
		else
			// Error!
			throw(EXCEPT("Couldn't read incoming data from client %d. (%d)", id, WSAGetLastError()));
	}
	else if(status == 0)
		disconnect(id);
	
	// Handle the new data
	//handleData(id, data);

	// Packet object
	PACKET _data(data);

	// Command of the packet
	char cmd = _data.get_byte();
	vec2 pos = _data.get_pos();
	
	dbg_msg("CLIENT", "ID: %d POS: x: %d y: %d", id, pos.x, pos.y/*, data*/);

	// Switch the cmds
	switch(cmd)
	{
	case 0x00: // Shoot
		//game.players[id]->onShoot(pos);
		/*for(unsigned int i = 0; i < config.max_clients; i++)
			if(game.players[i])
				send(i, packetmanager->serverCreateExplosionPacket(id, pos));*/
		break;
	}
}

void NETSERVER::handleData(int id, char data[512])
{
	char real_data[512];

	memcpy(real_data, data, sizeof(data));

	// Packet object
	PACKET _data(real_data);

	// Command of the packet
	char cmd = _data.get_byte();

	// Switch the cmds
	switch(cmd)
	{
	case 0x00: // Shoot
		vec2 pos = _data.get_pos();
		dbg_msg("CLIENT", "ID: %d POS: x: %d y: %d, %s", id, pos.x, pos.y, data);
		//game.players[id]->onShoot(pos);
		for(unsigned int i = 0; i < config.max_clients; i++)
			if(game.players[i])
				send(i, packetmanager->serverCreateExplosionPacket(id, pos));
		break;
	}
}

void NETSERVER::send(int id, char * data)
{
	// Send.
	if(::send(client_socks[id], data, sizeof(data), 0) == -1)
		throw(EXCEPT("Couldn't send data to client %d. (%d)", id, WSAGetLastError()));
}