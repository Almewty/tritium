#ifndef CFG_H
#define CFG_H

class CFGMANAGER
{
public:
	CFGMANAGER();
	~CFGMANAGER();

	// Files
	char langfile[500];
	// ---------------------
	char connfile[500];
	char gamefile[500];
	char spamfile[500];
	char detectfile[500];
	char adminfile[500];
	char logfile[500];
	char cmdfile[500];
	char inputfile[500];
	// ---------------------
	char adminlist[500];
	char banlist[500];
	// ---------------------
	char asfile[500];

	// Connection
	char server_ip[500];
	int server_port;
	int client_port;
	int max_ip_connections;
	int ip_connections_per_minute;
	int leading_byte;
	int max_recv;
	int max_recv_space;

	// Spamlimiters
	int no_limit_shouts;
	int no_limit_chats;
	int no_limit_drops;
	int no_limit_items;
	int no_limit_buffpang;
	int no_limit_facechange;
	int no_limit_safety_upgrade;

	int shouts_per_minute;
	int chats_per_minute;
	int drops_per_minute;
	int items_per_minute;
	int buffpang_interval;
	int facechange_interval;
	int upgrades_per_minute;

	int attack_24_tolerance;

	// Detection stuff
	int check_face_change;
	int check_buff_pang;
	int check_transy;
	int check_namechange;
	int check_guildnamechange;
	int check_drop;
	int check_element_scroll;
	int check_npc_upgrade;
	int check_invalid_name;
	int check_safety_upgrade;
	int check_equip_crash;
	int check_bag_dupe;
	int check_bag_crash;
	int check_24_attack;
	int check_null_quest;
	int check_negative_values;
	int check_trade_crash;
	int check_buying_info;
	int check_modify_mode;
	int check_infiltration;

	// Admin stuff
	char login_password[500];
	int login_allow;
	int anti_corrupt_begin;
	int anti_corrupt_end;

	int _no_gm_drop;
	int _no_gm_attack;
	int _no_gm_skill;
	int _no_gm_guild_bank;
	int _no_gm_bank;
	int _no_gm_mail;
	int _no_gm_shop_create;
	int _no_gm_shop_buy;
	int _no_gm_trade_item;
	int _no_gm_trade_money;
	int _no_gm_namechange;
	int _no_gm_gnamechange;

	// Log stuff
	int log_admins;
	int log_players;
	int log_ips;

	// Ingame stuff
	char server_name[500];
	int rates_exp;
	int rates_drop;
	int rates_penya;
	int show_chats;
	char notice_loop_message_file[500];
	int notice_loop_interval;
	int min_admin_level;
	int time_diff;

	// Lang files etc
	char session_error[500];
	char login_error[500];
	char banned_account[500];
	char banned_ip[500];

	char welcome_notice[500];

	char cmds_welcome[500];
	char admin_too_low[500];
	char admin_auth[500];
	char cmds_av[500];
	char players_online[500];

	char change_status[500];

	char inv_cmd[500];
	char inv_arg[500];
	char inv_lvl[500];

	char not_admin[500];

	char inv_face_change[500];
	char inv_name_change[500];
	char inv_gname_change[500];

	char exc_face_change[500];
	char exc_buffing[500];
	char exc_chatting[500];
	char exc_shouting[500];
	char exc_item[500];
	char exc_drop[500];
	char exc_upgrade[500];

	char no_gm_drop[500];
	char no_gm_attack[500];
	char no_gm_skill[500];
	char no_gm_guild_bank[500];
	char no_gm_bank[500];
	char no_gm_mail[500];
	char no_gm_shop_create[500];
	char no_gm_shop_buy[500];
	char no_gm_trade_item[500];
	char no_gm_trade_money[500];
	char no_gm_namechange[500];
	char no_gm_gnamechange[500];

	char element_scroll[500];
	char npc_upgrade[500];
	char equip_crash[500];
	char transy[500];
	char bag_dupe[500];
	char bag_crash[500];
	char attack_24[500];

	// ------ Funcs -----

	// Load
	void load_cfg();
};

#endif