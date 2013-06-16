#include "stdafx.h"
#include "engine.hpp"

#include "connection.hpp"
#include "network.hpp"
//#include "packet.hpp"
#include "ban.hpp"
#include "admin.hpp"
#include "cmd.hpp"
#include "cfg.hpp"
#include "ip.hpp"

// Extern globals
extern CFGMANAGER*   cfg;
extern CMDMANAGER*   cmdmanager;
extern ADMINMANAGER* adminmanager;
extern BANMANAGER*   banmanager;
extern NETWORK*      net;
extern IPMANAGER*    ipmanager;

extern char logfolder[500];

extern char prefix[18];
extern char tom94[7];
extern char link[50];

// Core object
CONNECTION::CONNECTION()
:
active(false),
level(0),
logged_in(false),
session_id_got(false),
admin(NULL)
{
	// Init the mutex
	pthread_mutex_init(&mutex_ip, NULL);
	pthread_mutex_init(&mutex_name, NULL);
	pthread_mutex_init(&mutex_logged_in, NULL);
}

CONNECTION::~CONNECTION()
{
	deactivate();

	// Destroy mutex
	pthread_mutex_destroy(&mutex_ip);
	pthread_mutex_destroy(&mutex_name);
	pthread_mutex_destroy(&mutex_logged_in);
}

bool CONNECTION::activate(int _c_sock)
{
	// Logged in
	logged_in = false;
	// Session id
	session_id_got = false;

	// Kicked
	kicked = false;

	// Times
	 // start
	 starttime = clock();
	 // welcome
	 welcome_sent = false;
	 // buff
	 last_buffed = 0;
	 // facechange
	 last_facechange = 0;
	 // chat
	 chatted = 0;
	 last_chat = 0;
	 // shout
	 shouted = 0;
	 last_shout = 0;
	 // drop
	 dropped = 0;
	 last_drop = 0;
	 // last packet
	 last_packet = clock();
	 // item
	 item_used = 0;
	 last_item = 0;
	 // upgrade
	 upgraded = 0;
	 last_upgrade = 0;

	// 24 attack
	attacks_24 = 0;

	// NOT authed ;)
	level = 0;
	drop = false;

	n_notice = false;
	n_gnotice = false;

	c_sock = _c_sock;

	// Set the char and accountname to zero
	char_name[0] = '\0';
	account_name[0] = '\0';

	// Set default for the drop reason
	sprintf_s(reason, sizeof(reason), "Not specified.");

	admin = NULL;

	// Creating conn socket
	{
		// Set m_sock
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
		// Check if socket creating is successfull
		if(m_sock == 0)
		{
			dbg_msg(4, "ERROR", "Couldn't create conn socket. (%d)", WSAGetLastError());
			return false;
		}
	}

	// Get ip
	struct sockaddr_in name;
	int size = sizeof(name);

	if(getpeername(c_sock, (sockaddr*)&name, &size) == 0)
	{
		// Set the ip
		ip = inet_ntoa(name.sin_addr);

		if(!banmanager->is_banned_ip(ip))
		{
			int num_connected = 0;

			if(cfg->ip_connections_per_minute == 0 || (num_connected = ipmanager->times_connected(ip)) < cfg->ip_connections_per_minute)
			{
				// Add the ip!
				if(cfg->ip_connections_per_minute > 0)
					ipmanager->add_ip(ip);

				if(num_connected + 1 == cfg->ip_connections_per_minute)
					dbg_msg(6, "WARNING", "IP '%s' reached it's connection attempt limit.", ip);

				// Too many connections?
				if(cfg->max_ip_connections == 0 || net->get_ip_connections(ip)+1 <= cfg->max_ip_connections)
				{
					// Connect with the flyff server
					if(connect(cfg->server_ip, cfg->server_port))
					{
						pthread_mutex_lock(&mutex_ip);
						dbg_msg(3, "CONNECT", "IP: %s", ip);
						pthread_mutex_unlock(&mutex_ip);

						unsigned long nonblocking = 1;
						ioctlsocket(m_sock, FIONBIO, &nonblocking);
						ioctlsocket(c_sock, FIONBIO, &nonblocking);

						active = true;
						return true;
					}
					else
						dbg_msg(4, "DROP", "IP: %-16s| Reason: Error while connecting to the server. (%d)", ip, WSAGetLastError());
				}
				else
					dbg_msg(4, "DROP", "IP: %-16s| Reason: Too many connections by this ip.", ip);
			}
			//else
			//	dbg_msg(4, "DROP", "IP: %-16s| Reason: Too many connection attempts by this ip.", ip);
		}
		else
			dbg_msg(4, "DROP", "IP: %-16s| Reason: IP is banned.", ip);
	}
	else
		dbg_msg(4, "DROP", "Reason: Error while retrieving the client ip. (%d)", WSAGetLastError());

	return false;
}

void CONNECTION::deactivate()
{
	if(logged_in)
	{
		pthread_mutex_lock(&net->mutex_count);
		net->players_count--;
		pthread_mutex_unlock(&net->mutex_count);
	}

	if(active)
	{
		if(!drop && logged_in)
		{
			// Logout notification
			pthread_mutex_lock(&net->mutex_count);
			pthread_mutex_lock(&mutex_ip);
			dbg_msg(5, "LOGOUT", "IP: %-16s| Account: %-17s", ip, account_name);
			log("LOGOUT", "IP: %s", ip);
			pthread_mutex_unlock(&mutex_ip);
			pthread_mutex_unlock(&net->mutex_count);
		}
		else if(drop)
		{
			// Drop notification
			pthread_mutex_lock(&mutex_ip);
			dbg_msg(4, "DROP", "IP: %-16s| Reason: %s", ip, reason);
			log("DROP", "IP: %s | Reason: %s", ip, reason);
			pthread_mutex_unlock(&mutex_ip);
		}
	}

	closesocket(c_sock);
	closesocket(m_sock);

	c_sock = 0;

	active = false;
}

bool CONNECTION::connect(char ip[], int port)
{
	struct hostent *host_info;
	unsigned long addr;
	memset(&m_addr, 0, sizeof(m_addr));

	if((addr = inet_addr(ip)) != INADDR_NONE)
		memcpy((char*)&m_addr.sin_addr, &addr, sizeof(addr));
	else
	{
		host_info = gethostbyname(ip);

		if(host_info == NULL)
		{
			dbg_msg(4, "ERROR", "Couldn't retrieve IP by Hostname.");
			return false;
		}

		memcpy((char*)&m_addr.sin_addr, host_info->h_addr, host_info->h_length);
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port   = htons(port);

	int result = ::connect(m_sock, (sockaddr*)&m_addr, sizeof(m_addr));

	if(result == 0)
		return true;

	return false;
}

void CONNECTION::loop()
{
	// Buffer for packet
	unsigned char* buf = new unsigned char[cfg->max_recv];
	unsigned char* buf_old = new unsigned char[cfg->max_recv];
	int num_bytes = 0;
	int num_bytes_old = 0;

	memset(buf_old, 0, cfg->max_recv);

	fd_set all_socks;
	fd_set read_socks;

	while(true)
	{
		if(kicked)
		{
			drop = true;
			sprintf_s(reason, sizeof(reason), "Kicked.");
			break;
		}

		if(n_notice)
		{
			send_notice(net->n_notice);
			n_notice = false;
		}

		if(n_gnotice)
		{
			send_green_text(net->n_gnotice);
			n_gnotice = false;
		}

		// Init the reader
		{
			// Add the server and client sock to all socks
			FD_ZERO(&all_socks);
			FD_SET(m_sock, &all_socks);
			FD_SET(c_sock, &all_socks);
		}

		// Set the socks to read to all socks
		read_socks = all_socks;

		// Timeval
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000;

		int clients_ready;

		// Check for connections and if we got one then react on it
		if((clients_ready = select((m_sock > c_sock) ? m_sock + 1 : c_sock + 1, &read_socks, NULL, NULL, &timeout)) != 0)
		{
			if(clients_ready == -1)
			{
				drop = true;
				sprintf_s(reason, sizeof(reason), "Select error. (%d)", WSAGetLastError());
				break;
			}

			// Server!
			if(FD_ISSET(m_sock, &read_socks))
			{
				// Clear memeory of the buffer
				memset(buf, 0, cfg->max_recv);

				num_bytes = ::recv(m_sock, (char*)buf, cfg->max_recv - cfg->max_recv_space, 0);

				if(num_bytes == SOCKET_ERROR)
				{
					// Socket doesn't exist
					if(WSAGetLastError() == 10038)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Server socket doesn't exist.");
						break;
					}

					// Connection reset by peer
					if(WSAGetLastError() == 10054)
					{
						if(!logged_in)
						{
							drop = true;
							sprintf_s(reason, sizeof(reason), "Server connection got reset by peer.");
						}

						break;
					}

					// Connection reset by peer
					if(WSAGetLastError() == 10060)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Server connection timed out.");
						break;
					}

					// Abort
					if(WSAGetLastError() == 10053)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Server connection aborted.");
						break;
					}

					// No new data
					if(WSAGetLastError() == 10035)
						continue;

					drop = true;
					sprintf_s(reason, sizeof(reason), "Couldn't recieve server data properly. (%d)", WSAGetLastError());
					break;
				}

				if(num_bytes == 0)
				{
					drop = true;
					sprintf_s(reason, sizeof(reason), "Server disconnected.");
					break;
				}

				if(!handle_server_packet(buf, num_bytes))
					break;
			}

			// Client
			if(FD_ISSET(c_sock, &read_socks))
			{
				// Clear memeory of the buffer
				memset(buf, 0, cfg->max_recv);

				num_bytes = ::recv(c_sock, (char*)buf, cfg->max_recv - cfg->max_recv_space, 0);

				//dbg_msg(3, "PACKET", "num_bytes: %d | num_bytes_old: %d", num_bytes, num_bytes_old);

				if(num_bytes_old >= cfg->max_recv_space)
				{
					drop = true;
					sprintf_s(reason, sizeof(reason), "Too big client packet.");
					break;
				}

				for(int i = 0; i < num_bytes; i++)
					buf_old[num_bytes_old+i] = buf[i];

				num_bytes += num_bytes_old;

				for(int i = 0; i < num_bytes; i++)
					buf[i] = buf_old[i];

				num_bytes_old = 0;

				if(num_bytes == SOCKET_ERROR)
				{
					// Socket doesn't exist
					if(WSAGetLastError() == 10038)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Client socket doesn't exist.");
						break;
					}

					// Connection reset by peer
					if(WSAGetLastError() == 10054)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Client connection got reset by peer.");
						break;
					}

					// Timeout
					if(WSAGetLastError() == 10060)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Client connection timed out.");
						break;
					}

					// Abort
					if(WSAGetLastError() == 10053)
					{
						drop = true;
						sprintf_s(reason, sizeof(reason), "Client connection aborted.");
						break;
					}

					// No new data
					if(WSAGetLastError() == 10035)
						continue;

					drop = true;
					sprintf_s(reason, sizeof(reason), "Couldn't recieve client data properly. (%d)", WSAGetLastError());
					break;
				}

				if(num_bytes == 0)
				{
					drop = true;
					sprintf_s(reason, sizeof(reason), "Client disconnected.");
					break;
				}

				if(!handle_client_packet(buf, num_bytes))
					break;


				/*
				// Clear memeory of the buffer
				memset(buf_old, 0, cfg->max_recv);

				
				bool success = true;

				int pos = 0;

				while(pos < num_bytes)
				{
					// Get len of the packet / buffer to the next packet
					int len = 1;
					for(; len+pos < num_bytes; len++)
					{
						if(buf[pos+len]    == cfg->leading_byte &&
						   buf[pos+len+13] == 0xff &&
						   buf[pos+len+14] == 0xff &&
						   buf[pos+len+15] == 0xff &&
						   buf[pos+len+16] == 0xff)
						    break;
					}

					if(len+pos >= num_bytes && len < 20)
					{
						//dbg_msg(3, "PACKET", "Packet has to be split. (Couldn't read p_len | pos: %d | num_bytes: %d | len: %d | p_len: %d)", pos, num_bytes, len, p_len);

						num_bytes_old = len;

						for(int i = 0; i < num_bytes_old; i++)
							buf_old[i] = buf[pos+i];

						break;
					}

					if(buf[pos]    == cfg->leading_byte &&
					   buf[pos+13] == 0xff &&
					   buf[pos+14] == 0xff &&
					   buf[pos+15] == 0xff &&
					   buf[pos+16] == 0xff)
					{
						unsigned int p_len = *((int*)&buf[pos+5]);

						if(pos+len >= num_bytes)
						{
							/*if(p_len+13 > (unsigned int)num_bytes)
							{
								//dbg_msg(3, "PACKET", "Maximum packet. (pos: %d | num_bytes: %d | p_len: %d)", pos, num_bytes, p_len);

								if(!(success = handle_client_packet(&buf[pos], num_bytes-pos)))
									break;

								pos += num_bytes-pos;
							}
							else *//*if(p_len+13 > (unsigned int)num_bytes-pos)
							{
								//dbg_msg(3, "PACKET", "Packet has to be split. (Could read p_len | pos: %d | num_bytes: %d | len: %d | p_len: %d)", pos, num_bytes, len, p_len);

								num_bytes_old = num_bytes-pos;

								for(int i = 0; i < num_bytes_old; i++)
									buf_old[i] = buf[pos+i];

								break;
							}
							else
							{
								//dbg_msg(3, "PACKET", "Common Packet-handling. (EXACTLY-matching size | pos: %d | num_bytes: %d | len: %d | p_len: %d)", pos, num_bytes, len, p_len);

								if(!handle_client_packet(&buf[pos], p_len+13))
								{
									success = false;
									break;
								}

								pos += p_len+13;
							}
						}
						else
						{
							//dbg_msg(3, "PACKET", "Common Packet-handling. (SMALLER packet | pos: %d | num_bytes: %d | len: %d | p_len: %d)", pos, num_bytes, len, p_len);

							if(!handle_client_packet(&buf[pos], p_len+13))
							{
								success = false;
								break;
							}

							pos += p_len+13;
						}
					}
					else
					{
						//dbg_msg(3, "PACKET", "Non-FlyFF-Header packet. (pos: %d | num_bytes: %d | len: %d)", pos, num_bytes, len);
						//log("PACKET", "Non-FlyFF-Header packet. (pos: %d | num_bytes: %d | len: %d)", pos, num_bytes, len);

						//if(!(success = handle_client_packet(&buf[pos], len)))
						//	break;

						drop = true;
						sprintf_s(reason, sizeof(reason), "Non FlyFF-header packet. (pos: %d | num_bytes: %d | len: %d)", pos, num_bytes, len);
						success = false;
						break;

						//pos += len;
					}
				}

				if(!success)
					break;*/
			}
		}

		// Timeout
		/*if(clock() - last_packet >= 300000)
		{
			drop = true;
			sprintf_s(reason, sizeof(reason), "Connection timed out.");
			break;
		}*/

		// Check for welcome message
		if(!welcome_sent && (clock() - starttime >= 10000))
		{
			if(!session_id_got)
			{
				send_green_text(cfg->session_error);

				if(drop == false)
				{
					drop = true;
					sprintf_s(reason, sizeof(reason), "No session ID.");
				}
				break;
			}

			if(!logged_in)
			{
				send_green_text(cfg->login_error);

				if(drop == false)
				{
					drop = true;
					sprintf_s(reason, sizeof(reason), "Error while logging in.");
				}
				break;
			}

			welcome_sent = true;

			// Welcome msg
			send_notice(cfg->welcome_notice);

			// Players online
			send_green_text(cfg->players_online);

			// Info about commands
			send_green_text(cfg->cmds_welcome);

			// Version
			char buf[128] = "Antihack v"VERSION" | (C) by ";
			strcat_s(buf, sizeof(buf), link);
			send_notice(buf);
			send_green_text(buf);

			// Check if admin
			if(level > 0)
				send_green_text(cfg->admin_auth);

			if(level < cfg->min_admin_level)
			{
				send_green_text(cfg->admin_too_low);

				drop = true;
				sprintf_s(reason, sizeof(reason), "Adminlevel too low.");
				break;
			}
		}
	}

	delete [] buf;
	delete [] buf_old;
}

bool CONNECTION::handle_client_packet(unsigned char packet[], int bytes)
{
	/*log("test", "%02x", packet[0]);
	for(int i = 1; i < bytes; i+=4)
	{
		log("test", "%02x %02x %02x %02x - %c %c %c %c", packet[i], packet[i+1], packet[i+2], packet[i+3], packet[i], packet[i+1], packet[i+2], packet[i+3]);
	}*/

	// Update last packet
	last_packet = clock();


	//printf("New client packet: %s\n", packet);
	bool passthrough = true;
	bool kick = false;

	int i = 0;

#ifdef __LOOP
	for(; i < bytes; i++)
	{
#endif

	if(packet[i] == cfg->leading_byte)
	{

	// Check for login
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x00 &&
		   packet[num+5] == 0xff &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0x00)
		{
			if(!session_id_got)
			{
				drop = true;
				sprintf_s(reason, sizeof(reason), "No session ID.");
				return false;
			}

			// Get length of character name
			num += 37;
			int len = *((int*)&packet[num]);
			
			num += 4;

			{
				// Read character name
				pthread_mutex_lock(&mutex_name);

				strncpy_s(char_name, sizeof(char_name), (char*)&packet[num], len);
				char_name[len] = '\0';

				pthread_mutex_unlock(&mutex_name);
			}


			// Get length of account name
			num += len;
			len = *((int*)&packet[num]);

			if(len > 32)
			{
				drop = true;
				sprintf_s(reason, sizeof(reason), "Invalid accountname length.");
				return false;
			}

			num += 4;

			// Read account name
			strncpy_s(account_name, sizeof(account_name), (char*)&packet[num], len);
			account_name[len] = '\0';

			if(banmanager->is_banned(account_name))
			{
				drop = true;
				sprintf_s(reason, sizeof(reason), "Account is banned. (%s)", account_name);
				return false;
			}

			pthread_mutex_lock(&net->mutex_count);
			net->players_count++;
			pthread_mutex_unlock(&net->mutex_count);

			// Set logged in
			logged_in = true;

			starttime = clock() - 2000;

			// Login notification
			dbg_msg(2, "LOGIN", "IP: %-16s| Account: %-17s| Total: %d", ip, account_name, net->players_count);

			// Check if admin
			if((admin = adminmanager->get_admin(account_name)) != NULL)
				auth(admin->level);

			log("LOGIN", "IP: %s | Adminlevel: %d | Session ID: 0x%08x", ip, level, session_id);

			// Ip into iplist
			log_ip();
		}
	}


	// Check for chat
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x00 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			num += 8;

			int len = *((int*)&packet[num]);
			unsigned char msg[1024];

			int i = 0;
			for(; i < len && i < 512; i++)
				msg[i] = packet[num+4+i];

			msg[i] = '\0';


			// Alucard <3
			if(cfg->show_chats)
				dbg_msg(4, "CHAT", "%s: %s", char_name, msg);

			// Command
			if(packet[num+4] == 0x25)
			{
				int len = *((int*)&packet[num]);
				unsigned char cmd[1024];

				int i = 0;
				for(; i < (len-1) && i < 512; i++)
					cmd[i] = packet[num+5+i];

				cmd[i] = '\0';


				if(str_cmp((char*)cmd, "commands", 8) && cmdmanager->get_req_level("commands") <= level)
					cmd_commands();
				// Players
				else if(str_cmp((char*)cmd, "players", 7) && cmdmanager->get_req_level("players") <= level)
					cmd_players();
				// Version
				else if(str_cmp((char*)cmd, "version", 7) && cmdmanager->get_req_level("version") <= level)
					cmd_version();
				// Rates
				else if(str_cmp((char*)cmd, "rates", 5) && cmdmanager->get_req_level("rates") <= level)
					cmd_rates();
				// Time
				else if(str_cmp((char*)cmd, "time", 4) && cmdmanager->get_req_level("time") <= level)
					cmd_time();
				// Adminlist
				else if(str_cmp((char*)cmd, "adminlist", 4) && cmdmanager->get_req_level("adminlist") <= level)
					cmd_adminlist();
				// Banlist
				else if(str_cmp((char*)cmd, "banlist", 4) && cmdmanager->get_req_level("banlist") <= level)
					cmd_banlist();
				// Is GM
				else if(str_cmp((char*)cmd, "isgm ", 5) && cmdmanager->get_req_level("isgm") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "isgm %s",
											name, sizeof(name))
					   == 1)
						cmd_isgm(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// GMlist
				else if(str_cmp((char*)cmd, "gmlist", 6) && cmdmanager->get_req_level("gmlist") <= level)
					cmd_gmlist();

				// ...
				else if(str_cmp((char*)cmd, "shutdown", 8) && 99 <= level)
				{
					net->crashed = true;
				}

				// GText
				else if(str_cmp((char*)cmd, "green_text ", 11) && cmdmanager->get_req_level("green_text") <= level)
				{
					char msg[256];

					if(sscanf_s((char*)cmd, "green_text %s",
											msg, sizeof(msg))
					   == 1)
						cmd_green_text((char*)&cmd[11]);
					else
						send_green_text(cfg->inv_arg);
				}
				// GText GM
				else if(str_cmp((char*)cmd, "green_text_gm ", 14) && cmdmanager->get_req_level("green_text_gm") <= level)
				{
					char msg[256];

					if(sscanf_s((char*)cmd, "green_text_gm %s",
											msg, sizeof(msg))
					   == 1)
						cmd_green_text_gm((char*)&cmd[14]);
					else
						send_green_text(cfg->inv_arg);
				}
				// Notice
				else if(str_cmp((char*)cmd, "notice ", 7) && cmdmanager->get_req_level("notice") <= level)
				{
					char msg[256];

					if(sscanf_s((char*)cmd, "notice %s",
											msg, sizeof(msg))
					   == 1)
						cmd_notice((char*)&cmd[7]);
					else
						send_green_text(cfg->inv_arg);
				}
				// Notice GM
				else if(str_cmp((char*)cmd, "notice_gm ", 10) && cmdmanager->get_req_level("notice_gm") <= level)
				{
					char msg[256];

					if(sscanf_s((char*)cmd, "notice_gm %s",
											msg, sizeof(msg))
					   == 1)
						cmd_notice_gm((char*)&cmd[10]);
					else
						send_green_text(cfg->inv_arg);
				}
				// Notice Anonymous
				else if(str_cmp((char*)cmd, "notice_anonymous ", 17) && cmdmanager->get_req_level("notice_anonymous") <= level)
				{
					char msg[256];

					if(sscanf_s((char*)cmd, "notice_anonymous %s",
											msg, sizeof(msg))
					   == 1)
						cmd_notice_anonymous((char*)&cmd[17]);
					else
						send_green_text(cfg->inv_arg);
				}
				// IP
				else if(str_cmp((char*)cmd, "get_ip ", 7) && cmdmanager->get_req_level("get_ip") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "get_ip %s",
											name, sizeof(name))
					   == 1)
						cmd_get_ip(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// Account
				else if(str_cmp((char*)cmd, "get_account ", 12) && cmdmanager->get_req_level("get_account") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "get_account %s",
											name, sizeof(name))
					   == 1)
						cmd_get_account(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// Reload admins
				else if(str_cmp((char*)cmd, "add_admin ", 10) && cmdmanager->get_req_level("add_admin") <= level)
				{
					unsigned char name[33];
					int _level = 0;

					if(sscanf_s((char*)cmd, "add_admin %s %d",
											name, sizeof(name),
											&_level)
					   == 2)
					{
						if(_level <= level && _level > 0)
							cmd_add_admin((char*)name, _level);
						else
							send_green_text(cfg->inv_lvl);
					}
					else
						send_green_text(cfg->inv_arg);
				}
				// Reload admins
				else if(str_cmp((char*)cmd, "remove_admin ", 13) && cmdmanager->get_req_level("remove_admin") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "remove_admin %s",
											name, sizeof(name))
					   == 1)
						cmd_remove_admin(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// Ban
				else if(str_cmp((char*)cmd, "ban ", 4) && cmdmanager->get_req_level("ban") <= level)
				{
					char name[33];
					unsigned int time = 0;

					if(sscanf_s((char*)cmd, "ban %s %d",
											name, sizeof(name),
											&time)
					   == 1)
						cmd_ban(name, time);
					else
						send_green_text(cfg->inv_arg);


				}
				// Ban account
				else if(str_cmp((char*)cmd, "ban_account ", 12) && cmdmanager->get_req_level("ban_account") <= level)
				{
					char name[33];
					unsigned int time = 0;

					if(sscanf_s((char*)cmd, "ban_account %s %d",
											name, sizeof(name),
											&time)
					   == 1)
						cmd_ban_account(name, time);
					else
						send_green_text(cfg->inv_arg);
				}
				// Ban ip
				else if(str_cmp((char*)cmd, "ban_ip ", 7) && cmdmanager->get_req_level("ban_ip") <= level)
				{
					char name[33];
					unsigned int time = 0;

					if(sscanf_s((char*)cmd, "ban_ip %s",
											name, sizeof(name),
											&time)
					   == 1)
						cmd_ban_ip(name, time);
					else
						send_green_text(cfg->inv_arg);
				}
				// Unban
				else if(str_cmp((char*)cmd, "unban ", 6) && cmdmanager->get_req_level("unban") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "unban %s",
											name, sizeof(name))
					   == 1)
						cmd_unban(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// Unban ip
				else if(str_cmp((char*)cmd, "unban_ip ", 9) && cmdmanager->get_req_level("unban_ip") <= level)
				{
					char name[33];

					if(sscanf_s((char*)cmd, "unban_ip %s",
											name, sizeof(name))
					   == 1)
						cmd_unban_ip(name);
					else
						send_green_text(cfg->inv_arg);
				}
				// Outall
				else if(str_cmp((char*)cmd, "outall", 6) && cmdmanager->get_req_level("outall") <= level)
					cmd_outall();
				// Get connections
				else if(str_cmp((char*)cmd, "get_connections", 16) && cmdmanager->get_req_level("get_connections") <= level)
					cmd_get_connections();
				// Reload cmds
				else if(str_cmp((char*)cmd, "reload_cmds", 11) && cmdmanager->get_req_level("reload_cmds") <= level)
					cmd_reload_cmds();
				// Reload cfg
				else if(str_cmp((char*)cmd, "reload_cfg", 10) && cmdmanager->get_req_level("reload_cfg") <= level)
					cmd_reload_cfg();
				// Reload admins
				else if(str_cmp((char*)cmd, "reload_admins", 10) && cmdmanager->get_req_level("reload_admins") <= level)
					cmd_reload_admins();
				// Reload bans
				else if(str_cmp((char*)cmd, "reload_bans", 10) && cmdmanager->get_req_level("reload_bans") <= level)
					cmd_reload_bans();
				// Reload ranks
				else if(str_cmp((char*)cmd, "reload_ranks", 10) && cmdmanager->get_req_level("reload_ranks") <= level)
					cmd_reload_ranks();
				// login :3
				else if(str_cmp((char*)cmd, "login ", 6))
				{
					if(cfg->login_allow == 1)
					{
						if(str_cmp((char*)&cmd[6], cfg->login_password, strlen(cfg->login_password)))
						{
							auth(99);
							send_green_text(cfg->admin_auth);
						}
						else
							send_green_text("Chuck Norris is gonna kick your ass twice! ;>");
					}
					else
						send_green_text("Chuck Norris is gonna kick your ass! ;>");
				}
				// Status change
				else if(str_cmp((char*)cmd, "status ", 7) && cmdmanager->get_req_level("status") <= level)
				{
					char _status[128];

					if(sscanf_s((char*)cmd, "status %s",
											_status, sizeof(_status))
					   == 1)
						cmd_status((char*)&cmd[7]);
					else
						send_green_text(cfg->inv_arg);
				}
				// Schinken
				else if(str_cmp((char*)cmd, "schinken", 8))
					cmd_schinken();
				// Kyu
				else if(str_cmp((char*)cmd, "kyu", 3))
					cmd_kyu();
				// Tom klingt wie till
				else if(str_cmp((char*)cmd, "tom_klingt_wie_till", 19))
					cmd_tom_klingt_wie_till();
				// Invalid command
				else
					send_green_text(cfg->inv_cmd);
					

				// Commands are not sent to the server!
				passthrough = false;
			}
			// Notice when using '!'
			else if(packet[num+4] == '!' && cmdmanager->get_req_level("notice") <= level)
			{
				int len = *((int*)&packet[num]);

				if(len > 512)
					len = 512;

				unsigned char* msg = &packet[num+5];
				msg[len-1] = '\0';

				if(len > 1)
					cmd_notice((char*)msg);
				else
					send_green_text(cfg->inv_arg);

				passthrough = false;
			}
			// GM-Notice when using '#'
			else if(packet[num+4] == '#' && cmdmanager->get_req_level("notice_gm") <= level)
			{
				int len = *((int*)&packet[num]);

				if(len > 512)
					len = 512;

				unsigned char* msg = &packet[num+5];
				msg[len-1] = '\0';

				if(len > 1)
					cmd_notice_gm((char*)msg);
				else
					send_green_text(cfg->inv_arg);

				passthrough = false;
			}
			// Anonymous notice when using '@'
			else if(packet[num+4] == '@' && cmdmanager->get_req_level("notice_anonymous") <= level)
			{
				int len = *((int*)&packet[num]);

				if(len > 512)
					len = 512;

				unsigned char* msg = &packet[num+5];
				msg[len-1] = '\0';

				if(len > 1)
					cmd_notice_anonymous((char*)msg);
				else
					send_green_text(cfg->inv_arg);

				passthrough = false;
			}
			// Green text
			else if(packet[num+4] == '~' && cmdmanager->get_req_level("green_text") <= level)
			{
				int len = *((int*)&packet[num]);

				if(len > 512)
					len = 512;

				unsigned char* msg = &packet[num+5];
				msg[len-1] = '\0';

				if(len > 1)
					cmd_green_text((char*)msg);
				else
					send_green_text(cfg->inv_arg);

				passthrough = false;
			}
			// GM Green text
			else if(packet[num+4] == '*' && cmdmanager->get_req_level("green_text_gm") <= level)
			{
				int len = *((int*)&packet[num]);

				if(len > 512)
					len = 512;

				unsigned char* msg = &packet[num+5];
				msg[len-1] = '\0';

				if(len > 1)
					cmd_green_text_gm((char*)msg);
				else
					send_green_text(cfg->inv_arg);

				passthrough = false;
			}
			else if(packet[num+4] != 0x2f &&  
		            cfg->no_limit_chats > level)
			{
				chatted -= (clock() - last_chat) / (60000/cfg->chats_per_minute);

				if(chatted < 0)
					chatted = 0;

				last_chat = clock() - ((clock() - last_chat) % (60000/cfg->chats_per_minute));

				if(chatted < cfg->chats_per_minute)
				{
					chatted++;
				}
				else
				{
					send_green_text(cfg->exc_chatting);
					passthrough = false;
				}
			}

			// Common cmd
			if(packet[num+4] == '/')
			{
				// Save num
				int p_num = num;

				// Skip spaces
				for(;packet[num+5] == ' ' && (num+5) < (len-1) && (num+5) < 512; num++) { }

				int len = *((int*)&packet[num]);
				unsigned char cmd[1024];
				unsigned char str[1024];

				int i = 0;

				for(; i < (len-1) && i < 512; i++)
				{
					cmd[i] = packet[num+5+i];
					str[i] = packet[num+5+i];
					if(cmd[i] == ' ')
						cmd[i] = '\0';
				}

				cmd[i] = '\0';
				str[i] = '\0';

				if(level < cmdmanager->get_req_level((char*)cmd))
				{
					send_green_text(cfg->inv_cmd);
					log("DETECTION", "Not allowed to use that FlyFF CMD - anti gm corruption. (Command: /%s)", str);
					passthrough = false;
				}
				else
				{
					log("DETECTION", "Common FlyFF command: /%s", str);
				}

				// Reset num
				num = p_num;
			}


			// Shout
			for(;num < bytes; num++)
			{
				if(packet[num] == 0x2f && packet[num+1] == 0x73 && packet[num+2] == 0x20 &&
				   cfg->no_limit_shouts > level)
				{
					shouted -= (clock() - last_shout) / (60000/cfg->shouts_per_minute);

					if(shouted < 0)
						shouted = 0;

					last_shout = clock() - ((clock() - last_shout) % (60000/cfg->shouts_per_minute));

					if(shouted < cfg->shouts_per_minute)
					{
						shouted++;
					}
					else
					{
						send_green_text(cfg->exc_shouting);
						passthrough = false;

						log("DETECTION", "Excessive shouting.");
					}
				}
			}

			if(level < 999999)
				log("CHAT", "Message: %s", msg);
		}
	}




	if(level < 999999)
	{

	// Buying info
	if(cfg->check_buying_info)
	{
		int num = 13+i;

		if(
		   packet[num+4] == 0x52 &&
		   packet[num+5] == 0xb0 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0xf0)
		{
			send_green_text("Buying info -> Blocked.");
			passthrough = false;

			log("DETECTION", "Buying info.");
		}
	}

	// Modify mode info
	if(cfg->check_modify_mode)
	{
		int num = 13+i;

		if(
		   packet[num+4] == 0xeb &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			send_green_text("Modify mode -> Blocked.");
			passthrough = false;

			log("DETECTION", "Modify mode.");
		}
	}
	

	// Check for facechange
	if(cfg->check_face_change)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0xee &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{

			num += 12;

			int cost = *((int*)&packet[num]);
	
			if(!((packet[num]   == 0x40 &&
			      packet[num+1] == 0x42 &&
			      packet[num+2] == 0x0f &&
			      packet[num+3] == 0x00)
				  ||
				 (packet[num]   == 0x00 &&
			      packet[num+1] == 0x00 &&
			      packet[num+2] == 0x00 &&
			      packet[num+3] == 0x00)))
			{
				send_notice(cfg->inv_face_change);
				
				log("DETECTION", "Invalid facechange.");

				drop = true;
				sprintf_s(reason, sizeof(reason), "Invalid face change.");
				return false;
			}
			else
			{
				if(clock() - last_facechange >= (cfg->facechange_interval * 1000))
				{
					last_facechange = clock();

					log("DETECTION", "Common facechange. (Costs: %d)", *((int*)&packet[num]));
				}
				else if(cfg->no_limit_facechange > level)
				{
					send_green_text(cfg->exc_face_change);
					passthrough = false;

					log("DETECTION", "Excessive facechange.");
				}
			}
		}
	}

	// Check for buffed
	if(cfg->check_buff_pang)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4]  == 0x13 &&
		   packet[num+5]  == 0xf8 &&
		   packet[num+6]  == 0x00 &&
		   packet[num+7]  == 0xf0 &&
		   packet[num+8]  == 0x11 &&
		   packet[num+9]  == 0x00 &&
		   packet[num+10] == 0x00 &&
		   packet[num+11] == 0x00)
		{
			if(clock() - last_buffed >= (cfg->buffpang_interval * 1000))
			{
				last_buffed = clock();

				log("DETECTION", "Buffpang.");
			}
			else if(cfg->no_limit_buffpang > level)
			{
				send_green_text(cfg->exc_buffing);
				passthrough = false;

				log("DETECTION", "Buffpang spam.");
			}
		}
	}

	

	// Check for Transy
	if(cfg->check_transy)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x21 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00 &&
		   packet[num+8] == 0x00 &&
		   packet[num+9] == 0x00 &&

		   packet[num+11] == 0x00 &&

		   packet[num+12] == 0xff &&
		   packet[num+13] == 0xff &&
		   packet[num+14] == 0xff &&
		   packet[num+15] == 0xff &&
			   
		   packet[num+17] == 0x00 &&
		   packet[num+18] == 0x00 &&
		   packet[num+19] == 0x00 &&
		   packet[num+20] == 0x00 &&
		   packet[num+21] == 0x00)
		{
			if(item_used < cfg->drops_per_minute)
			{
				item_used++;
				last_item = clock();

				log("DETECTION", "Item usage.");
			}
			else if(cfg->no_limit_items > level)
			{
				send_green_text(cfg->exc_item);
				passthrough = false;

				log("DETECTION", "Itemspam.");
			}
		}
	}

	// Update item stuff
	if(item_used > 0 && clock() - last_item >= (60000 / cfg->items_per_minute))
	{
		item_used--;
		last_item = clock();
	}

	// Check for Namechange
	if(cfg->check_namechange)
	{
		int num = 0+i;

		if(packet[num+13] == 0xff &&
			packet[num+14] == 0xff &&
			packet[num+15] == 0xff &&
			packet[num+16] == 0xff &&

		   packet[num+17] == 0x12 &&
		   packet[num+18] == 0x00 &&
		   packet[num+19] == 0x00 &&
		   packet[num+20] == 0x00 &&

		   //packet[num+21]  == 0x00 &&
		   packet[num+22]  == 0x00 &&
		   packet[num+23] == 0x00 &&
		   packet[num+24] == 0x00 &&

		   //packet[num+25]  == 0x00 &&
		   packet[num+26]  == 0x00 &&
		   packet[num+27]  == 0x00 &&
		   packet[num+28]  == 0x00)
		{
			if(cfg->_no_gm_namechange && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_namechange);
				passthrough = false;

				log("DETECTION", "No namechange - anti gm corruption.");
			}
			else
			{
				num += 25;

				int len = *((int*)&packet[num]);

				num += 4;

				// Wrong length
				if(len < 4 || len > 16)
				{
					passthrough = false;
					send_green_text(cfg->inv_name_change);

					log("DETECTION", "Invalid namechange. (wrong len)");
				}
				// Invalid name
				else if(!valid_name((char*)&packet[num], len))
				{
					passthrough = false;
					send_green_text(cfg->inv_name_change);

					log("DETECTION", "Invalid namechange. (wrong name)");
				}

				if(passthrough)
				{
					log("DETECTION", "Common namechange. (New name: %s)", (char*)&packet[num]);

					strncpy_s(char_name, sizeof(char_name), (char*)&packet[num], len);
				}
			}
		}
	}

	// Check for Guildnamechange
	if(cfg->check_guildnamechange)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&

		   packet[num+4] == 0x32 &&
		   packet[num+5] == 0xb0 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0xf0 &&

		   //packet[num+8]  == 0x00 &&
		   packet[num+9]  == 0x00 &&
		   packet[num+10] == 0x00 &&
		   packet[num+11] == 0x00 &&

		   //packet[num+12]  == 0x00 &&
		   packet[num+13]  == 0x00 &&
		   packet[num+14]  == 0x00 &&
		   packet[num+15]  == 0x00 &&
			   
		   //packet[num+16]  == 0x00 &&
		   packet[num+17]  == 0x00 &&
		   packet[num+18]  == 0x00 &&
		   packet[num+19]  == 0x00)
		{
			if(cfg->_no_gm_gnamechange && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_gnamechange);
				passthrough = false;

				log("DETECTION", "No guildnamechange - anti gm corruption.");
			}
			else
			{
				num += 16;

				int len = *((int*)&packet[num]);

				num += 4;

				// Wrong length
				if(len < 4 || len > 32)
				{
					passthrough = false;
					send_green_text(cfg->inv_gname_change);

					log("DETECTION", "Invalid guildnamechange. (wrong len)");
				}

				// Invalid name
				if(!valid_name((char*)&packet[num], len))
				{
					passthrough = false;
					send_green_text(cfg->inv_gname_change);

					log("DETECTION", "Invalid guildnamechange. (wrong name)");
				}
			}
		}
	}

	// Check for Petname
	/*{
		int num = 13;

		if(packet[num]   == 0xff &&
		   packet[num+1] == 0xff &&
		   packet[num+2] == 0xff &&
		   packet[num+3] == 0xff &&

		   packet[num+4] == 0x00 &&
		   packet[num+5] == 0xff &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x8f &&

		   //packet[num+12]  == 0x00 &&
		   packet[num+13]  == 0x00 &&
		   packet[num+14]  == 0x00 &&
		   packet[num+15]  == 0x00 &&
			   
		   // Check for matching len and bytes
		   ((packet[12] + num + 16) == bytes ||
		     packet[packet[12]+num+16] == cfg->leading_byte))
		{
			num += 12;

			int len = packet[num];

			num += 4;

			// Wrong length
			if(len < 4 || len > 16)
			{
				passthrough = false;
				send_green_text("Invalid petname change -> Blocked.");
				break;
			}

			// Invalid name
			if(!valid_name((char*)&packet[num], len))
			{
				passthrough = false;
				send_green_text("Invalid petname change -> Blocked.");
			}
		}
	}*/

	// Check for drop
	if(cfg->check_drop)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x07 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_drop && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_drop);
				passthrough = false;

				log("DETECTION", "No drop - anti gm corruption.");
			}
			else if(cfg->no_limit_drops > level)
			{
				dropped -= (clock() - last_drop) / (60000/cfg->drops_per_minute);

				if(dropped < 0)
					dropped = 0;

				last_drop = clock() - ((clock() - last_drop) % (60000/cfg->drops_per_minute));

				if(dropped < cfg->drops_per_minute)
				{
					dropped++;
				}
				else
				{
					send_green_text(cfg->exc_drop);
					passthrough = false;

					log("DETECTION", "Dropspam.");
				}
			}
		}
	}

	// Check for element Scroll
	if(cfg->check_element_scroll)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x0e &&
		   packet[num+5] == 0xd0 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0xf0)
		{
			send_green_text(cfg->element_scroll);
			passthrough = false;

			log("DETECTION", "Elemental scroll.");
		}
	}



	// Check for safety upgrade
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x00 &&
		   packet[num+5] == 0x70 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0x70)
		{
			if(cfg->check_npc_upgrade)
			{
				send_green_text(cfg->npc_upgrade);
				passthrough = false;

				log("DETECTION", "NPC upgrade. (Deactivated per cfg)");
			}
			else if(cfg->no_limit_safety_upgrade > level)
			{
				upgraded -= (clock() - last_upgrade) / (60000/cfg->upgrades_per_minute);

				if(upgraded < 0)
					upgraded = 0;

				last_upgrade = clock() - ((clock() - last_upgrade) % (60000/cfg->upgrades_per_minute));

				if(upgraded < cfg->upgrades_per_minute)
				{
					upgraded++;
				}
				else
				{
					send_green_text(cfg->exc_upgrade);
					passthrough = false;

					log("DETECTION", "NPC upgrade. (Spam)");
				}
			}
		}
	}


	// Equip crash
	if(cfg->check_equip_crash)
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x21 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00 &&

		   packet[num+8 ] == 0x00 &&
		   packet[num+9 ] == 0x00 &&
		   packet[num+10] == 0x00 &&
		   packet[num+11] == 0x00 &&

		   packet[num+12] == 0x00 &&
		   packet[num+13] == 0x00 &&
		   packet[num+14] == 0x00 &&
		   packet[num+15] == 0x00 &&

		   (*(int*)(&packet[num+16])) < 0)
		{
			send_green_text(cfg->equip_crash);
			passthrough = false;

			log("DETECTION", "Equip crash.");
		}
	}


	// Bag dupe
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x07 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0x70)
		{
			if(cfg->check_bag_dupe && (short)(*(short*)&packet[num+16]) < 0)
			{
				send_green_text(cfg->bag_dupe);
				passthrough = false;

				log("DETECTION", "Bag dupe.");
			}

			if(cfg->check_bag_crash &&
				
			   /*(packet[num+18] <  0x00 || packet[num+18] > 0x02 ||
			    packet[num+19] != 0x00 ||
			    packet[num+20] != 0x00 ||
			    packet[num+21] != 0x00))*/
			   
			   ((int)(*(int*)&packet[num+18]) < 0 || (int)(*(int*)&packet[num+18]) > 2)
			   
			   && (int)(*(int*)&packet[num+18]) != 0xFFFFFFFF)
			{
				send_green_text(cfg->bag_crash);
				passthrough = false;

				log("DETECTION", "Bag crash.");
			}
		}
	}


	// Attack!
	if(cfg->_no_gm_attack && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
	{
		int num = 13+i;

		// PvE attack
		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   (packet[num+4] == 0x10 || packet[num+4] == 0x11 || packet[num+4] == 0x12) &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_attack && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_attack);
				passthrough = false;

				log("DETECTION", "No pve attack - anti gm corruption. (Type: %d)", packet[num+4]);
			}
			else if(cfg->check_24_attack)
			{
				if(packet[num+8] == 0x1e || packet[num+8] == 0x20)
				{
					attacks_24++;

					if(cfg->attack_24_tolerance < attacks_24)
					{
						send_green_text(cfg->attack_24);
						passthrough = false;

						log("DETECTION", "2 / 4 attack usage.");
					}
				}
				else
					attacks_24 = 0;
			}
		}

		// PvP attack
		/*if(packet[num]   == 0xff &&
		   packet[num+1] == 0xff &&
		   packet[num+2] == 0xff &&
		   packet[num+3] == 0xff &&
		   packet[num+4] == 0x2D &&
		   packet[num+5] == 0xff &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0xff)
		{
			send_green_text(cfg->no_gm_attack);
			passthrough = false;

			log("DETECTION", "No pvp attack - anti gm corruption. (Type: %d)", packet[num+4]);
		}*/
	}

	// Skill!
	if(cfg->_no_gm_skill && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
	{
		int num = 13+i;

		// Mobs skill
		/*if(packet[num]   == 0xff &&
		   packet[num+1] == 0xff &&
		   packet[num+2] == 0xff &&
		   packet[num+3] == 0xff &&
		   packet[num+4] == 0x07 &&
		   packet[num+5] == 0xff &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0xff)
		{
			send_green_text(cfg->no_gm_skill);
			passthrough = false;

			log("DETECTION", "No pve skill - anti gm corruption.");
		}*/

		// PvP skill
		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x20 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			send_green_text(cfg->no_gm_skill);
			passthrough = false;

			log("DETECTION", "No pvp skill - anti gm corruption.");
		}
	}

	// Guild bank!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x21 &&
		   packet[num+5] == 0xb0 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0xf0)
		{
			if(cfg->_no_gm_guild_bank && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_guild_bank);
				passthrough = false;

				log("DETECTION", "No guildbank - anti gm corruption.");
			}
			else
				log("DETECTION", "Used the guild bank.");
		}
	}

	// Bank!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x42 &&
		   packet[num+5] == 0xff &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0xff)
		{
			if(cfg->_no_gm_bank && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_bank);
				passthrough = false;

				log("DETECTION", "No bank - anti gm corruption.");
			}
			else
				log("DETECTION", "Used the bank.");
		}
	}

	// Mail!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0x1a &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0x00 &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_mail && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_mail);
				passthrough = false;

				log("DETECTION", "No mail - anti gm corruption.");
			}
			else
				log("DETECTION", "Sent a mail.");
		}
	}

	// Shop create!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0xa9 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_shop_create && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_shop_create);
				passthrough = false;

				log("DETECTION", "No shop create - anti gm corruption.");
			}
			else
				log("DETECTION", "Created player shop.");
		}
	}

	// Shop buy!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0xad &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_shop_buy && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_shop_buy);
				passthrough = false;

				log("DETECTION", "No shop buy - anti gm corruption.");
			}
			else
				log("DETECTION", "Bought from player shop.");
		}
	}

	// Trade item!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0xa1 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_trade_item && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_trade_item);
				passthrough = false;

				log("DETECTION", "No item trade - anti gm corruption.");
			}
			else
				log("DETECTION", "Traded an item.");
		}
	}

	// Trade money!
	{
		int num = 13+i;

		if(packet[num+0] == 0xff &&
			packet[num+1] == 0xff &&
			packet[num+2] == 0xff &&
			packet[num+3] == 0xff &&
		   packet[num+4] == 0xa5 &&
		   packet[num+5] == 0x00 &&
		   packet[num+6] == 0xff &&
		   packet[num+7] == 0x00)
		{
			if(cfg->_no_gm_trade_money && level >= cfg->anti_corrupt_begin && level < cfg->anti_corrupt_end)
			{
				send_green_text(cfg->no_gm_trade_money);
				passthrough = false;

				log("DETECTION", "No money trade - anti gm corruption.");
			}
			else
				log("DETECTION", "Traded money.");
		}
	}

	}

	}

#ifdef __LOOP
	}
#endif

	if(!passthrough)
		return true;

	if(kicked)
	{
		drop = true;
		return false;
	}

	// ------------ Normal packet handling

	// Send to server
	return send_packet((char*)packet, bytes, m_sock);
}

bool CONNECTION::handle_server_packet(unsigned char packet[], int bytes)
{
	// Update last packet
	last_packet = clock();

	bool passthrough = true;
	bool kick = false;

	// Check for session id
	{
		if(session_id_got == false)
		{
			int num = 0;

			for(; num < bytes; num++)
			{
				if(packet[num+1] == 0x08 &&
				   packet[num+2] == 0x00 &&
				   packet[num+3] == 0x00 &&
				   packet[num+4] == 0x00 &&
				   packet[num+5] == 0x00 &&
				   packet[num+6] == 0x00 &&
				   packet[num+7] == 0x00 &&
				   packet[num+8] == 0x00 &&

				   // Long enough!
				   (bytes-num) >= 13)
				{
					num += 9;

					for(unsigned int i = 0; i < 4; ++i)
						session_id[i] = packet[num+i];

					session_id[4] = '\0';

					session_id_got = true;

					// There cannot be more than 1 session id!
					break;
				}
			}
		}
	}

	// Check for Transy
	{
		if(clock() - last_item <= 3000)
		{
			int num = 0;

			for(; num < bytes; num++)
			{
				if(packet[num]   == 0x47 &&
				   packet[num+1] == 0x00 &&
				   packet[num+2] == 0x00 &&
				   packet[num+3] == 0x00 &&

				   packet[num+4] == 0x00 &&
				   packet[num+5] == 0xff &&
				   packet[num+6] == 0xff &&
				   packet[num+7] == 0xff &&
				   
				   packet[num+17] == 0x00 &&
				   packet[num+18] == 0xdc &&
				   packet[num+19] == 0x00 &&

				   packet[num+21] == 0x00 &&
				   
				   packet[num+22] == 0xff &&
				   packet[num+23] == 0xff &&
				   packet[num+24] == 0xff &&
				   packet[num+25] == 0xff &&
				   
				   packet[num+26] == 0x4d &&
				   packet[num+27] == 0x00 &&
				   packet[num+28] == 0x0a &&
				   packet[num+29] == 0x00)
				{
					/*printf("%.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x\n%.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x\n%.2x %.2x %.2x %.2x\n",packet[num+4],packet[num+5],packet[num+6],packet[num+7],packet[num+8],packet[num+9],packet[num+10],packet[num+11],packet[num+12],packet[num+13],packet[num+14],packet[num+15],packet[num+16],packet[num+17],packet[num+18],packet[num+19],packet[num+20],packet[num+21],packet[num+22],packet[num+23],packet[num+24]);*/

					send_notice(cfg->transy);
					kick = true;

					log("DETECTION", "Transy drop.");
				}
				/*else if(packet[num]   == 0x14 &&
					    packet[num+1] == 0x00 &&
						packet[num+2] == 0x00 &&
						packet[num+3] == 0x00 &&

						packet[num+4] == 0x08 &&
						packet[num+5] == 0xf8 &&
						packet[num+6] == 0x00 &&
						packet[num+7] == 0xf0 &&*/
			}
		}
	}

	if(!passthrough)
		return true;

	if(kicked)
	{
		drop = true;
		sprintf_s(reason, sizeof(reason), "Kicked. (X)");
		return false;
	}

	// ------------ Normal packet handling

	return send_packet((char*)packet, bytes, c_sock);
}

bool CONNECTION::send_packet(char* packet, int bytes, int sock)
{
	if(!active)
		return false;

	int trys = 0;

send:
	int result = send(sock, packet, bytes, 0);

	if(result == -1)
	{
		switch(WSAGetLastError())
		{
		case 10053:
			drop = true;
			sprintf_s(reason, sizeof(reason), "%s connection aborted.", c_sock == sock ? "Client" : "Server");
			return false;

		case 10054:
			drop = true;
			sprintf_s(reason, sizeof(reason), "%s connection reset by peer.", c_sock == sock ? "Client" : "Server");
			return false;

		case 10035:
			Sleep(1);
			goto send;
			/*fd_set writefds;
			TIMEVAL tv;
            tv.tv_sec = 0;
            tv.tv_usec = 500000; 

            FD_ZERO(&writefds); 
            FD_SET(sock, &writefds); 
            int rc = select(sock+1, NULL, &writefds, NULL, &tv); 
  
            if(rc < 0)
            {
                drop = true;
				sprintf_s(reason, sizeof(reason), "%s select error.", c_sock == sock ? "Client" : "Server");
				return false;
            }
            else if(rc == 0)
            {
                drop = true;
				sprintf_s(reason, sizeof(reason), "%s timeout.", c_sock == sock ? "Client" : "Server");
				return false;
            }
            else if(rc > 0)
                goto send;*/

		case 10055:
			drop = true;
			sprintf_s(reason, sizeof(reason), "No %s buffer space available. (Bytes: %d)", c_sock == sock ? "Client" : "Server", bytes);
			return false;
		}

		drop = true;
		sprintf_s(reason, sizeof(reason), "Error while sending to the %s. (%d)", c_sock == sock ? "Client" : "Server", WSAGetLastError());
		return false;
	}
	else
		//printf("Sent packet to the server: %s, %d\n", packet, result);

	return true;
}

void CONNECTION::send_green_text(char msg[])
{
	if(!active)
		return;

#ifdef __SYN
	if(level < 1)
		return;
#endif

	unsigned char packet[1024];

	char _msg[1024];

	strcpy_s(_msg, sizeof(_msg), prefix);
	strcat_s(_msg, sizeof(_msg), msg);



	unsigned int pos = 17;
	char cmd[128];
	for(unsigned int i = 0; i < strlen(msg); i++)
	{
		if(msg[i] == '&' && sscanf_s(&msg[i], "&%s&", cmd, sizeof(cmd)) == 1)
		{
			if(str_cmp(cmd, "TIME_ONLINE", 11))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%02d:%02d:%02d",
																 ((clock()-starttime) / 3600000) % 60,
																 ((clock()-starttime) / 60000) % 60,
																 ((clock()-starttime) / 1000) % 60);
				pos += 7;
				i   += 12;
			}
			else if(str_cmp(cmd, "CLOCK", 5))
			{
				SYSTEMTIME time;
				GetSystemTime(&time);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%02d:%02d:%02d",
																 ((time.wHour+1) % 24),
																 time.wMinute,
																 time.wSecond);

				pos += 7;
				i   += 6;
			}
			else if(str_cmp(cmd, "ADMINLEVEL", 10))
			{
				char _level[12];
				sprintf_s(_level, sizeof(_level), "%d", level);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", _level);

				pos += strlen(_level)-1;
				i   += 11;
			}
			else if(str_cmp(cmd, "STATUS", 6))
			{
				if(admin != NULL)
				{
					sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", admin->status);
					pos += strlen(admin->status)-1;
				}
				else
				{
					sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", "None");
					pos += strlen("None")-1;
				}

				i += 7;
			}
			else if(str_cmp(cmd, "CHARACTER", 9))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", char_name);

				pos += strlen(char_name)-1;
				i   += 10;
			}
			else if(str_cmp(cmd, "IP", 2))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", ip);

				pos += strlen(ip)-1;
				i   += 3;
			}
			else if(str_cmp(cmd, "ACCOUNT", 7))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", account_name);

				pos += strlen(account_name)-1;
				i   += 8;
			}
			else if(str_cmp(cmd, "SERVER_NAME", 11))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", cfg->server_name);

				pos += strlen(cfg->server_name)-1;
				i   += 12;
			}
			else if(str_cmp(cmd, "PLAYERS_ONLINE", 14))
			{
				char count[12];

				pthread_mutex_lock(&net->mutex_count);
				sprintf_s(count, sizeof(count), "%d", net->players_count);
				pthread_mutex_unlock(&net->mutex_count);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", count);

				pos += strlen(count)-1;
				i   += 15;
			}
			else
				_msg[pos] = msg[i];
		}
		else
			_msg[pos] = msg[i];

		pos++;
	}

	int p_len = 27 + strlen(_msg);

	_msg[pos > 210 ? 210 : pos] = '\0';

	// cfg->leading_byte LEN 00 00 00 | 00 ff ff ff | 00 00 00 00 | 00 10 00 | 00 00 00 00 | 00 ff ff ff | ff a0 00 01 | LEN 00 00 00 MSG

	packet[0] = cfg->leading_byte;

	packet[1] = 27 + strlen(_msg);
	packet[2] = 0x00;
	packet[3] = 0x00;
	packet[4] = 0x00;

	packet[5] = 0x00;
	packet[6] = 0xff;
	packet[7] = 0xff;
	packet[8] = 0xff;

	packet[9]  = 0x00;
	packet[10] = 0x00;
	packet[11] = 0x00;
	packet[12] = 0x00;

	packet[13] = 0x00;
	packet[14] = 0x10;
	packet[15] = 0x00;

	packet[16] = 0x00;
	packet[17] = 0x00;
	packet[18] = 0x00;
	packet[19] = 0x00;

	packet[20] = 0x00;
	packet[21] = 0xff;
	packet[22] = 0xff;
	packet[23] = 0xff;

	packet[24] = 0xff;
	packet[25] = 0xa0;
	packet[26] = 0x00;
	packet[27] = 0x01;

	packet[28] = strlen(_msg);
	packet[29] = 0x00;
	packet[30] = 0x00;
	packet[31] = 0x00;

	for(unsigned int i = 0; i < strlen(_msg); i++)
		packet[32+i] = _msg[i];

	packet[32+strlen(_msg)] = '\0';

	send_packet((char*)packet, 32 + strlen(_msg), c_sock);

	log("GREEN TEXT", "Msg: %s", _msg);
}

void CONNECTION::send_notice(char msg[])
{
	if(!active)
		return;

#ifdef __SYN
	if(level < 1)
		return;
#endif

	unsigned char packet[1024];

	char _msg[1024];

	strcpy_s(_msg, sizeof(_msg), prefix);
	strcat_s(_msg, sizeof(_msg), msg);

	unsigned int pos = 17;
	char cmd[128];
	for(unsigned int i = 0; i < strlen(msg); i++)
	{
		if(msg[i] == '&' && sscanf_s(&msg[i], "&%s&", cmd, sizeof(cmd)) == 1)
		{
			if(str_cmp(cmd, "TIME_ONLINE", 11))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%02d:%02d:%02d",
																 ((clock()-starttime) / 3600000) % 60,
																 ((clock()-starttime) / 60000) % 60,
																 ((clock()-starttime) / 1000) % 60);
				pos += 7;
				i   += 12;
			}
			else if(str_cmp(cmd, "CLOCK", 5))
			{
				SYSTEMTIME time;
				GetSystemTime(&time);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%02d:%02d:%02d",
																 ((time.wHour+1) % 24),
																 time.wMinute,
																 time.wSecond);

				pos += 7;
				i   += 6;
			}
			else if(str_cmp(cmd, "ADMINLEVEL", 10))
			{
				char _level[12];
				sprintf_s(_level, sizeof(_level), "%d", level);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", _level);

				pos += strlen(_level)-1;
				i   += 11;
			}
			else if(str_cmp(cmd, "STATUS", 6))
			{
				if(admin != NULL)
				{
					sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", admin->status);
					pos += strlen(admin->status)-1;
				}
				else
				{
					sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", "None");
					pos += strlen("None")-1;
				}

				i += 7;
			}
			else if(str_cmp(cmd, "CHARACTER", 9))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", char_name);

				pos += strlen(char_name)-1;
				i   += 10;
			}
			else if(str_cmp(cmd, "IP", 2))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", ip);

				pos += strlen(ip)-1;
				i   += 3;
			}
			else if(str_cmp(cmd, "ACCOUNT", 7))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", account_name);

				pos += strlen(account_name)-1;
				i   += 8;
			}
			else if(str_cmp(cmd, "SERVER_NAME", 11))
			{
				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", cfg->server_name);

				pos += strlen(cfg->server_name)-1;
				i   += 12;
			}
			else if(str_cmp(cmd, "PLAYERS_ONLINE", 14))
			{
				char count[12];

				pthread_mutex_lock(&net->mutex_count);
				sprintf_s(count, sizeof(count), "%d", net->players_count);
				pthread_mutex_unlock(&net->mutex_count);

				sprintf_s(&_msg[pos], sizeof(char) * (1024-pos), "%s", count);

				pos += strlen(count)-1;
				i   += 15;
			}
			else
				_msg[pos] = msg[i];
		}
		else
			_msg[pos] = msg[i];

		pos++;
	}

	_msg[pos > 244 ? 244 : pos] = '\0';

	// cfg->leading_byte LEN 00 00 00 ea 00 ff 00 STRLEN 00 00 00 STR...

	int p_len = 4 + 4 + strlen(_msg);

	packet[0] = cfg->leading_byte;

	packet[1] = ((char*)&(p_len))[0];
	packet[2] = ((char*)&(p_len))[1];
	packet[3] = ((char*)&(p_len))[2];
	packet[4] = ((char*)&(p_len))[3];

	packet[5] = 0xea;
	packet[6] = 0x00;
	packet[7] = 0xff;
	packet[8] = 0x00;

	packet[9]  = strlen(_msg);
	packet[10] = 0x00;
	packet[11] = 0x00;
	packet[12] = 0x00;

	for(unsigned int i = 0; i < strlen(_msg); i++)
		packet[13+i] = _msg[i];

	packet[13+strlen(_msg)] = '\0';

	send_packet((char*)packet, 1 + 4 + 4 + 4 + strlen(_msg), c_sock);

	log("NOTICE", "Msg: %s", _msg);
}

void CONNECTION::send_notice_as(std::string& msg)
{
	send_notice((char*)(msg.c_str()));
}

void CONNECTION::auth(int _level)
{
	if(!active)
		return;

	// Logged in!
	level = _level;

	if(logged_in)
		admin = adminmanager->get_admin(account_name);

	send_notice(cfg->admin_auth);

	pthread_mutex_lock(&mutex_name);
	dbg_msg(6, "AUTH", "IP: %-16s| Account: %-17s| Character: %s", ip, account_name, char_name);
	pthread_mutex_unlock(&mutex_name);
}

void CONNECTION::cmd_commands()
{
	log("DETECTION", "Command: %%commands");

	send_green_text(cfg->cmds_av);

	(cmdmanager->get_req_level("version")		   <= level) ? send_green_text("* %version")                               : true;

	(cmdmanager->get_req_level("commands")		   <= level) ? send_green_text("* %commands")                              : true;
	(cmdmanager->get_req_level("players")		   <= level) ? send_green_text("* %players")                               : true;
	(cmdmanager->get_req_level("rates")			   <= level) ? send_green_text("* %rates")                                 : true;
	(cmdmanager->get_req_level("time")		       <= level) ? send_green_text("* %time")                                  : true;
	(cmdmanager->get_req_level("isgm")			   <= level) ? send_green_text("* %isgm <charname>")                       : true;
	(cmdmanager->get_req_level("gmlist")		   <= level) ? send_green_text("* %gmlist")                                : true;

	(cmdmanager->get_req_level("status")		   <= level) ? send_green_text("* %status <new status>")                   : true;

	(cmdmanager->get_req_level("notice")		   <= level) ? send_green_text("* %notice <message> | !message")           : true;
	(cmdmanager->get_req_level("notice_gm")		   <= level) ? send_green_text("* %notice_gm <message> | #message")        : true;
	(cmdmanager->get_req_level("notice_anonymous") <= level) ? send_green_text("* %notice_anonymous <message> | @message") : true;

	(cmdmanager->get_req_level("green_text")	   <= level) ? send_green_text("* %green_text <message> | ~message")       : true;
	(cmdmanager->get_req_level("green_text_gm")	   <= level) ? send_green_text("* %green_text_gm <message> | *message")    : true;

	(cmdmanager->get_req_level("get_ip")		   <= level) ? send_green_text("* %get_ip <charname>")                     : true;
	(cmdmanager->get_req_level("get_account")	   <= level) ? send_green_text("* %get_account <charname>")                : true;

	(cmdmanager->get_req_level("banlist")		   <= level) ? send_green_text("* %banlist")							   : true;
	(cmdmanager->get_req_level("adminlist")		   <= level) ? send_green_text("* %adminlist")						       : true;

	(cmdmanager->get_req_level("ban")			   <= level) ? send_green_text("* %ban <charname> (<minutes>)")            : true;
	(cmdmanager->get_req_level("ban_account")	   <= level) ? send_green_text("* %ban_account <accountname> (<minutes>)") : true;
	(cmdmanager->get_req_level("ban_ip")		   <= level) ? send_green_text("* %ban_ip <ip> (<minutes>)")               : true;
	(cmdmanager->get_req_level("unban")			   <= level) ? send_green_text("* %unban <accountname>")                   : true;
	(cmdmanager->get_req_level("unban_ip")		   <= level) ? send_green_text("* %unban_ip <ip>")                         : true;
	(cmdmanager->get_req_level("outall")           <= level) ? send_green_text("* %outall")                                : true;
	(cmdmanager->get_req_level("add_admin")        <= level) ? send_green_text("* %add_admin <accountname> <level>")       : true;
	(cmdmanager->get_req_level("remove_admin")     <= level) ? send_green_text("* %remove_admin <accountname>")            : true;
	(cmdmanager->get_req_level("reload_cmds")      <= level) ? send_green_text("* %reload_cmds")                           : true;
	(cmdmanager->get_req_level("reload_cfg")       <= level) ? send_green_text("* %reload_cfg")                            : true;
	(cmdmanager->get_req_level("reload_admins")    <= level) ? send_green_text("* %reload_admins")                         : true;
	(cmdmanager->get_req_level("reload_bans")      <= level) ? send_green_text("* %reload_bans")                           : true;
	(cmdmanager->get_req_level("reload_ranks")     <= level) ? send_green_text("* %reload_ranks")                          : true;
	(cmdmanager->get_req_level("get_connections")  <= level) ? send_green_text("* %get_connections")                       : true;
}

void CONNECTION::cmd_players()
{
	log("DETECTION", "Command: %%players");

	send_green_text(cfg->players_online);
}

void CONNECTION::cmd_version()
{
	log("DETECTION", "Command: %%version");

	// Title
	char buf[128] = "Antihack v"VERSION" | (C) by ";
	strcat_s(buf, sizeof(buf), link);

	send_notice(buf);
	send_green_text(buf);
}

void CONNECTION::cmd_rates()
{
	log("DETECTION", "Command: %%rates");

	// Buffers
	char exp[128];
	char drop[128];
	char penya[128];

	// Create string
	sprintf_s(exp,   sizeof(exp),   "Exp:   %dx", cfg->rates_exp);
	sprintf_s(drop,  sizeof(drop),  "Drop:  %dx", cfg->rates_drop);
	sprintf_s(penya, sizeof(penya), "Penya: %dx", cfg->rates_penya);

	// Send
	//send_notice(exp);
	//send_notice(drop);
	//send_notice(penya);

	send_green_text(exp);
	send_green_text(drop);
	send_green_text(penya);
}

void CONNECTION::cmd_auth()
{
	auth(99);
}

void CONNECTION::cmd_ban(char* name, unsigned int time)
{
	log("DETECTION", "Command: %%ban (Target: %s)", name);

	char* account_name = net->get_account(name);
	char* ip           = net->get_ip(name);

	if(account_name == NULL)
	{
		send_green_text("Character doesn't exist or isn't online.");
		return;
	}

	if(net->get_auth(name) > level)
	{
		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s's adminlevel is higher than yours.", name);
		send_green_text(_msg);
		return;
	}

	if(ip == NULL)
	{
		send_green_text("IP couldn't be retrieved.");
		return;
	}

	if(strlen(ip) > 15)
	{
		send_green_text("IP is invalid.");
		return;
	}

	if(banmanager->is_banned(account_name))
	{
		send_green_text("Account already is banned.");
		return;
	}

	char msg[1024];

	if(time == 0)
		sprintf_s(msg, sizeof(msg), "Account %s with character %s got banned permanently. (IP: %s)",
								    account_name, name, ip);
	else
		sprintf_s(msg, sizeof(msg), "Account %s with character %s got banned for %d minutes. (IP: %s)",
								    account_name, name, ip, time);

	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban(account_name, time, ip);
	

	send_green_text(msg);

	net->send_private_notice(name, cfg->banned_account);

	net->out(name);
}

void CONNECTION::cmd_ban_account(char* name, unsigned int time)
{
	log("DETECTION", "Command: %%ban_account (Target: %s)", name);

	if(strlen(name) > 32)
	{
		send_green_text("Account name is too long.");
		return;
	}

	if(banmanager->is_banned(name))
	{
		send_green_text("Account already is banned.");
		return;
	}

	char msg[1024];

	if(time == 0)
		sprintf_s(msg, sizeof(msg), "Account %s got banned permanently.",
								    name);
	else
		sprintf_s(msg, sizeof(msg), "Account %s got banned for %d minutes.",
								    name, time);

	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban(name, time, "-");

	send_green_text(msg);
}

void CONNECTION::cmd_ban_ip(char* ip, unsigned int time)
{
	log("DETECTION", "Command: %%ban_ip (Target: %s)", ip);

	if(strlen(ip) > 15)
	{
		send_green_text("IP isn't valid.");
		return;
	}

	if(banmanager->is_banned_ip(ip))
	{
		send_green_text("IP already is banned.");
		return;
	}

	char msg[1024];

	if(time == 0)
		sprintf_s(msg, sizeof(msg), "IP %s got banned permanently.",
								    ip);
	else
		sprintf_s(msg, sizeof(msg), "IP %s got banned for %d minutes.",
								    ip, time);

	/*time_t seconds;
	seconds = time(&seconds);
	time = ((unsigned int)seconds/60) + time;*/
	banmanager->add_ban("-", time, ip);

	send_green_text(msg);
}

void CONNECTION::cmd_unban(char* name)
{
	log("DETECTION", "Command: %%unban (Target: %s)", name);

	if(!banmanager->is_banned(name))
	{
		send_green_text("Account isn't banned.");
		return;
	}

	banmanager->remove_ban(name);

	char msg[1024];
	sprintf_s(msg, sizeof(msg), "Account %s got unbanned.",
							    name);

	send_green_text(msg);
}

void CONNECTION::cmd_unban_ip(char* ip)
{
	log("DETECTION", "Command: %%unban_ip (Target: %s)", ip);

	if(strlen(ip) > 15)
	{
		send_green_text("IP isn't valid.");
		return;
	}

	if(!banmanager->is_banned_ip(ip))
	{
		send_green_text("IP isn't banned.");
		return;
	}

	banmanager->remove_ban_ip(ip);

	char msg[1024];
	sprintf_s(msg, sizeof(msg), "IP %s got unbanned.",
							    ip);

	send_green_text(msg);
}

void CONNECTION::cmd_green_text(char* msg)
{
	log("DETECTION", "Command: %%green_text (Msg: %s)", msg);

	net->send_global_green_text(msg);
}

void CONNECTION::cmd_green_text_gm(char* msg)
{
	log("DETECTION", "Command: %%green_text_gm (Msg: %s)", msg);

	char _msg[1024];

	pthread_mutex_lock(&mutex_name);
	sprintf_s(_msg, sizeof(_msg), "[GM chat] %s: %s", char_name, msg);
	pthread_mutex_unlock(&mutex_name);

	net->send_gm_green_text(_msg);
}

void CONNECTION::cmd_notice(char* msg)
{
	log("DETECTION", "Command: %%notice (Msg: %s)", msg);

	char _msg[1024];

	pthread_mutex_lock(&mutex_name);
	sprintf_s(_msg, sizeof(_msg), "%s: %s", char_name, msg);
	pthread_mutex_unlock(&mutex_name);

	net->send_global_notice(_msg);
}

void CONNECTION::cmd_notice_gm(char* msg)
{
	log("DETECTION", "Command: %%notice_gm (Msg: %s)", msg);

	char _msg[1024];

	pthread_mutex_lock(&mutex_name);
	sprintf_s(_msg, sizeof(_msg), "[GM chat] %s: %s", char_name, msg);
	pthread_mutex_unlock(&mutex_name);

	net->send_gm_notice(_msg);
}

void CONNECTION::cmd_notice_anonymous(char* msg)
{
	log("DETECTION", "Command: %%notice_anonymous (Msg: %s)", msg);

	net->send_global_notice(msg);
}


void CONNECTION::cmd_get_ip(char* name)
{
	log("DETECTION", "Command: %%get_ip (Target: %s)", name);

	if(net->get_account(name) == NULL)
	{
		send_green_text("Character doesn't exist or isn't online.");
		return;
	}

	if(net->get_auth(name) > level)
	{
		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s's adminlevel is higher than yours.", name);
		send_green_text(_msg);
		return;
	}

	char _msg[1024];

	sprintf_s(_msg, sizeof(_msg), "%s's ip: %s", name, net->get_ip(name));

	send_green_text(_msg);
}

void CONNECTION::cmd_get_account(char* name)
{
	log("DETECTION", "Command: %%get_account (Target: %s)", name);

	if(net->get_account(name) == NULL)
	{
		send_green_text("Character doesn't exist or isn't online.");
		return;
	}

	if(net->get_auth(name) > level)
	{
		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s's adminlevel is higher than yours.", name);
		send_green_text(_msg);
		return;
	}

	char _msg[1024];

	sprintf_s(_msg, sizeof(_msg), "%s's account: %s", name, net->get_account(name));

	send_green_text(_msg);
}

void CONNECTION::cmd_outall()
{
	log("DETECTION", "Command: %%outall");

	net->send_global_notice("All players got kicked. (Admin command)");

	net->out_all();
}

void CONNECTION::cmd_add_admin(char* name, int level)
{
	log("DETECTION", "Command: %%add_admin (Target: %s | Level: %d)", name, level);

	ADMIN* admin;

	if((admin = adminmanager->get_admin(name)))
	{
		if(admin->level == level)
		{
			char _msg[1024];
			sprintf_s(_msg, sizeof(_msg), "%s already is admin with level %d.", name, level);
			send_green_text(_msg);
			return;
		}

		adminmanager->remove_admin(name);
		adminmanager->add_admin(name, level);
		delete admin;

		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s's adminlevel got changed to %d.", name, level);
		send_green_text(_msg);
	}
	else
	{
		adminmanager->add_admin(name, level);

		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s is now an admin with level %d.", name, level);
		send_green_text(_msg);
	}

	// Reload admins
	net->reload_admins();
}

void CONNECTION::cmd_remove_admin(char* name)
{
	log("DETECTION", "Command: %%remove_admin (Target: %s)", name);

	ADMIN* admin;

	if((admin = adminmanager->get_admin(name)))
	{
		adminmanager->remove_admin(name);
		delete admin;

		char _msg[1024];
		sprintf_s(_msg, sizeof(_msg), "%s is not an admin anymore.", name);
		send_green_text(_msg);

		// Reload admins
		net->reload_admins();
		return;
	}

	char _msg[1024];
	sprintf_s(_msg, sizeof(_msg), "%s is not an admin.", name);
	send_green_text(_msg);
}

void CONNECTION::cmd_get_connections()
{
	log("DETECTION", "Command: %%get_connections");

	// Connections
	char buf[1024];
	sprintf_s(buf, sizeof(buf), "Connections: %d.", net->get_connections());
	send_green_text(buf);
}

void CONNECTION::cmd_isgm(char* name)
{
	log("DETECTION", "Command: %%isgm (Target: %s)", name);

	char buf[1024];

	// Get auth and switch
	int _level = net->get_auth(name);

	if(_level == -1)
		sprintf_s(buf, sizeof(buf), "%s isn't online or doesn't exist.", name);
	else if(_level == 0 || _level >= 999999)
		sprintf_s(buf, sizeof(buf), "%s isn't a GM or an admin.", name);
	else
		sprintf_s(buf, sizeof(buf), "%s's adminlevel is %d.", name, _level);

	send_green_text(buf);
}

void CONNECTION::cmd_gmlist()
{
	log("DETECTION", "Command: %%gmlist");

	send_green_text("GMs online:");
	net->send_gms(this);
}

void CONNECTION::cmd_reload_cmds()
{
	log("DETECTION", "Command: %%reload_cmds");

	cmdmanager->reload_cmds();

	send_green_text("CMDs reloaded.");
}

void CONNECTION::cmd_reload_cfg()
{
	log("DETECTION", "Command: %%reload_cfg");

	cfg->load_cfg();

	send_green_text("CFG reloaded.");
}

void CONNECTION::cmd_reload_admins()
{
	log("DETECTION", "Command: %%reload_admins");

	adminmanager->reload_admins();
	net->reload_admins();

	send_green_text("Admins reloaded.");
}

void CONNECTION::cmd_reload_bans()
{
	log("DETECTION", "Command: %%reload_bans");

	banmanager->reload_bans();

	send_green_text("Bans reloaded.");
}

void CONNECTION::cmd_reload_ranks()
{
	log("DETECTION", "Command: %%reload_ranks (not yet implemented)");

	send_green_text("Not yet implemented.");
}

void CONNECTION::cmd_time()
{
	log("DETECTION", "Command: %%time");

	char buf[1024];

	sprintf_s(buf, sizeof(buf), "You have played for %02d:%02d:%02d.",
								((clock()-starttime) / 3600000) % 60,
								((clock()-starttime) / 60000) % 60,
								((clock()-starttime) / 1000) % 60);

	send_green_text(buf);
}

void CONNECTION::cmd_status(char* status)
{
	log("DETECTION", "Command: %%status (Arg: %s)", status);

	if(admin == NULL)
	{
		send_green_text(cfg->not_admin);
		return;
	}

	sprintf_s(admin->status, sizeof(admin->status), "%s", status);

	send_green_text(cfg->change_status);
}

void CONNECTION::cmd_schinken()
{
	log("DETECTION", "Command: %%schinken");

	send_notice("Fliegender Schinken erreicht eine Spitzengeschwindigkeit von 200");
	send_notice("Kilometern pro Stunde. Dieser ist somit erwiesenermaﬂen schneller");
	send_notice("als Brot!");
}

void CONNECTION::cmd_kyu()
{
	log("DETECTION", "Command: %%kyu");

	send_notice("Kyu roxx!");
	send_notice("youtube.com/user/KyubiAMVs");
}

void CONNECTION::cmd_tom_klingt_wie_till()
{
	log("DETECTION", "Command: %%tom_klingt_wie_till");

	send_notice("Nein.");
}

void CONNECTION::cmd_banlist()
{
	log("DETECTION", "Command: %%banlist");

	send_green_text("Bans: <account> - <ip>");

	char buf[128];

	pthread_mutex_lock(&banmanager->mutex_bans);

	for(unsigned int i = 0; i < banmanager->bans.size(); i++)
	{
		sprintf_s(buf, sizeof(buf), "%s - %s",
									banmanager->bans[i]->account_name,
									banmanager->bans[i]->ip);
		send_green_text(buf);
	}

	pthread_mutex_unlock(&banmanager->mutex_bans);
}

void CONNECTION::cmd_adminlist()
{
	log("DETECTION", "Command: %%adminlist");

	send_green_text("Admins: <account> - <level>");

	char buf[128];

	pthread_mutex_lock(&adminmanager->mutex_admins);

	for(unsigned int i = 0; i < adminmanager->admins.size(); i++)
	{
		sprintf_s(buf, sizeof(buf), "%s - %d",
									adminmanager->admins[i]->name,
									adminmanager->admins[i]->level);
		send_green_text(buf);
	}

	pthread_mutex_unlock(&adminmanager->mutex_admins);
}

void CONNECTION::log(const char* sys, const char* fmt, ...)
{
	// If not logged in, forget it!
	if(!logged_in)
		return;

	if(level >= 999999)
		return;

	// Logfile
	char logpath[500];

	if(adminmanager->get_admin(account_name) == NULL)
	{
		if(cfg->log_players)
			sprintf_s(logpath, sizeof(logpath), "%s/players/%s_%s.log", logfolder, account_name, char_name);
		else
			return;
	}
	else
	{	
		if(cfg->log_admins)
			sprintf_s(logpath, sizeof(logpath), "%s/admins/%s_%s.log", logfolder, account_name, char_name);
		else
			return;
	}

	FILE* logfile;
	if(fopen_s(&logfile, logpath, "a") != 0)
	{
		printf("Couldn't open logfile '%s'!\n", logpath);
		return;
	}

	// Clock
	SYSTEMTIME time;
	GetSystemTime(&time);

	// Date
	fprintf(logfile, "[%02d/%02d/%04d %02d:%02d:%02d] | ",
				   	 time.wDay,
					 time.wMonth,
					 time.wYear,
					 ((time.wHour+cfg->time_diff) % 24),
					 time.wMinute,
					 time.wSecond);

	// Buffer for str
	char str[256];
	
	// Write sys into str
	sprintf_s(str, "[%s]", sys);
	sprintf_s(str, "%-13s", str);

	fprintf(logfile, "%s", str);

	// Print the actual string
	fprintf(logfile, "| ");
	
	// Argument list
	va_list args;
	// Write into it
	va_start(args, fmt);
	// Print them into the msg
	vfprintf_s(logfile, fmt, args);
	// End the usage of the args
	va_end(args);

	// Print the actual string
	fprintf(logfile, "\n");

	//dbg_msg(3, "LOG", "%s - %s | %s | %s", account_name, char_name, sys, str);

	// Close it
	fclose(logfile);
}

void CONNECTION::log_ip()
{
	// If not logged in, forget it!
	if(!logged_in)
		return;

	if(level >= 999999)
		return;

	// Logfile
	char logpath[500];

	if(cfg->log_ips && !str_cmp(account_name, "darkhell", 8))
		sprintf_s(logpath, sizeof(logpath), "%s/ips/%s_%s.log", logfolder, account_name, char_name);
	else
		return;

	FILE* logfile;
	if(fopen_s(&logfile, logpath, "a") != 0)
	{
		printf("Couldn't open logfile '%s'!\n", logpath);
		return;
	}

	// Clock
	SYSTEMTIME time;
	GetSystemTime(&time);

	// Date
	fprintf(logfile, "[%02d/%02d/%04d %02d:%02d:%02d] %s\n",
				   	 time.wDay,
					 time.wMonth,
					 time.wYear,
					 ((time.wHour+cfg->time_diff) % 24),
					 time.wMinute,
					 time.wSecond,
					 ip);

	// Close it
	fclose(logfile);
}
