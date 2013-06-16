#include "stdafx.h"
#include "engine.hpp"
#include "cfg.hpp"


// Intern global
CFGMANAGER* cfg = 0;
// Extern global
extern char logfolder[500];
extern char allowed_name_chars[500];
extern int time_diff;

CFGMANAGER::CFGMANAGER()
{
	load_cfg();
}

CFGMANAGER::~CFGMANAGER()
{
}

void CFGMANAGER::load_cfg()
{
	// Info about loading
	dbg_msg(3, "INFO", "Attempting to load the CFG.");

	int begin = clock();

	// Files
	GetPrivateProfileString("files", "langfile", "./lang/ENG.ini", langfile, sizeof(langfile), "./config.ini"); 
	// -------------------------
	GetPrivateProfileString("files", "connfile",   "./config/connections.ini",  connfile,   sizeof(connfile),   "./config.ini"); 
	GetPrivateProfileString("files", "gamefile",   "./config/game.ini",         gamefile,   sizeof(gamefile),   "./config.ini"); 
	GetPrivateProfileString("files", "spamfile",   "./config/spamlimiters.ini", spamfile,   sizeof(spamfile),   "./config.ini"); 
	GetPrivateProfileString("files", "detectfile", "./config/detections.ini",   detectfile, sizeof(detectfile), "./config.ini"); 
	GetPrivateProfileString("files", "adminfile",  "./config/admin.ini",        adminfile,  sizeof(adminfile),  "./config.ini");
	GetPrivateProfileString("files", "logfile",    "./config/log.ini",          logfile,    sizeof(logfile),    "./config.ini");
	GetPrivateProfileString("files", "cmdfile",    "./config/commands.ini",     cmdfile,    sizeof(cmdfile),    "./config.ini"); 
	// -------------------------
	GetPrivateProfileString("files", "adminlist", "./data/adminlist.txt", adminlist, sizeof(adminlist), "./config.ini"); 
	GetPrivateProfileString("files", "banlist",   "./data/banlist.txt",   banlist,   sizeof(banlist),   "./config.ini");
	// -------------------------
	GetPrivateProfileString("files", "logfolder", "./log", logfolder, sizeof(logfolder), "./config.ini"); 
	// -------------------------
	GetPrivateProfileString("files", "inputfile", "./input.txt", inputfile, sizeof(inputfile), "./config.ini"); 
	// -------------------------
	GetPrivateProfileString("files", "asfile", "./script/main.as", asfile, sizeof(asfile), "./config.ini"); 
	dbg_msg(2, "SUCCESS", " * Paths loaded from '%s'.", "./config.ini");

	// Connection
	                     GetPrivateProfileString("connection", "server_ip",          "localhost", server_ip, sizeof(server_ip), connfile); 
	server_port        = GetPrivateProfileInt(   "connection", "server_port",        15000,                                     connfile);
	client_port        = GetPrivateProfileInt(   "connection", "client_port",        15400,                                     connfile);
	max_ip_connections = GetPrivateProfileInt(   "connection", "max_ip_connections", 3,                                         connfile);
	ip_connections_per_minute = GetPrivateProfileInt("connection", "ip_connections_per_minute", 5,                              connfile);
	leading_byte       = GetPrivateProfileInt(   "connection", "leading_byte",       0x5e,                                      connfile);
	max_recv           = GetPrivateProfileInt(   "connection", "max_recv",           50000,                                     connfile);
	max_recv_space     = GetPrivateProfileInt(   "connection", "max_recv_space",     10000,                                     connfile);
	dbg_msg(2, "SUCCESS", " * Connection parameters loaded from '%s'. 0x%02x", connfile, leading_byte);

	// Spamlimiters
	no_limit_shouts         = GetPrivateProfileInt("spamlimiters", "no_limit_shouts",         15, spamfile);
	no_limit_chats          = GetPrivateProfileInt("spamlimiters", "no_limit_chats",          5,  spamfile);
	no_limit_drops          = GetPrivateProfileInt("spamlimiters", "no_limit_drops",          40, spamfile);
	no_limit_items          = GetPrivateProfileInt("spamlimiters", "no_limit_items",          80, spamfile);
	no_limit_buffpang       = GetPrivateProfileInt("spamlimiters", "no_limit_buffpang",       80, spamfile);
	no_limit_facechange     = GetPrivateProfileInt("spamlimiters", "no_limit_facechange",     80, spamfile);
	no_limit_safety_upgrade = GetPrivateProfileInt("spamlimiters", "no_limit_safety_upgrade", 99, spamfile);

	shouts_per_minute   = GetPrivateProfileInt("spamlimiters", "shouts_per_minute",          5,  spamfile);
	chats_per_minute    = GetPrivateProfileInt("spamlimiters", "chats_per_minute",           12, spamfile);
	drops_per_minute    = GetPrivateProfileInt("spamlimiters", "drops_per_minute",           30, spamfile);
	items_per_minute    = GetPrivateProfileInt("spamlimiters", "items_per_minute",           60, spamfile);
	buffpang_interval   = GetPrivateProfileInt("spamlimiters", "buffpang_interval",          10, spamfile);
	facechange_interval = GetPrivateProfileInt("spamlimiters", "facechange_interval",        10, spamfile);
	upgrades_per_minute = GetPrivateProfileInt("spamlimiters", "safety_upgrades_per_minute", 60, spamfile);

	attack_24_tolerance = GetPrivateProfileInt("spamlimiters", "attack_24_tolerance",        1,  spamfile);
	dbg_msg(2, "SUCCESS", " * Spamlimiter parameters loaded from '%s'.", spamfile);

	// Detection cfg
	check_face_change = GetPrivateProfileInt("detect", "check_face_change", 1, detectfile);
	check_buff_pang = GetPrivateProfileInt("detect", "check_buff_pang", 1, detectfile);
	check_transy = GetPrivateProfileInt("detect", "check_transy", 1, detectfile);
	check_namechange = GetPrivateProfileInt("detect", "check_namechange", 1, detectfile);
	check_guildnamechange = GetPrivateProfileInt("detect", "check_guildnamechange", 1, detectfile);
	check_drop = GetPrivateProfileInt("detect", "check_drop", 1, detectfile);
	check_element_scroll = GetPrivateProfileInt("detect", "check_element_scroll", 1, detectfile);
	check_npc_upgrade = GetPrivateProfileInt("detect", "check_npc_upgrade", 1, detectfile);
	check_invalid_name = GetPrivateProfileInt("detect", "check_invalid_name", 1, detectfile);
	check_equip_crash = GetPrivateProfileInt("detect", "check_equip_crash", 1, detectfile);
	check_bag_dupe = GetPrivateProfileInt("detect", "check_bag_dupe", 1, detectfile);
	check_bag_crash = GetPrivateProfileInt("detect", "check_bag_crash", 1, detectfile);
	check_24_attack = GetPrivateProfileInt("detect", "check_24_attack", 1, detectfile);
	check_null_quest = GetPrivateProfileInt("detect", "check_null_quest", 1, detectfile);
	check_negative_values = GetPrivateProfileInt("detect", "check_negative_values", 1, detectfile);
	check_trade_crash = GetPrivateProfileInt("detect", "check_trade_crash", 1, detectfile);
	check_buying_info = GetPrivateProfileInt("detect", "check_buying_info", 1, detectfile);
	check_modify_mode = GetPrivateProfileInt("detect", "check_modify_mode", 1, detectfile);
	check_infiltration = GetPrivateProfileInt("detect", "check_infiltration", 1, detectfile);

	dbg_msg(2, "SUCCESS", " * Detection parameters loaded from '%s'.", detectfile);

	// Admin stuff
	                       GetPrivateProfileString("admin", "login_password",     "G.T5(d3Fd&", login_password, sizeof(login_password), adminfile);
	login_allow          = GetPrivateProfileInt(   "admin", "login_allow",        1,                                                    adminfile);
	anti_corrupt_begin   = GetPrivateProfileInt(   "admin", "anti_corrupt_begin", 30,                                                   adminfile);
	anti_corrupt_end     = GetPrivateProfileInt(   "admin", "anti_corrupt_end",   80,                                                   adminfile);

	_no_gm_drop = GetPrivateProfileInt("admin", "no_gm_drop", 1, adminfile);
	_no_gm_attack = GetPrivateProfileInt("admin", "no_gm_attack", 1, adminfile);
	_no_gm_skill = GetPrivateProfileInt("admin", "no_gm_skill", 1, adminfile);
	_no_gm_guild_bank = GetPrivateProfileInt("admin", "no_gm_guild_bank", 1, adminfile);
	_no_gm_bank = GetPrivateProfileInt("admin", "no_gm_bank", 1, adminfile);
	_no_gm_mail = GetPrivateProfileInt("admin", "no_gm_mail", 1, adminfile);
	_no_gm_shop_create = GetPrivateProfileInt("admin", "no_gm_shop_create", 1, adminfile);
	_no_gm_shop_buy = GetPrivateProfileInt("admin", "no_gm_shop_buy", 1, adminfile);
	_no_gm_trade_item = GetPrivateProfileInt("admin", "no_gm_trade_item", 1, adminfile);
	_no_gm_trade_money = GetPrivateProfileInt("admin", "no_gm_trade_money", 1, adminfile);
	_no_gm_namechange = GetPrivateProfileInt("admin", "no_gm_namechange", 1, adminfile);
	_no_gm_gnamechange = GetPrivateProfileInt("admin", "no_gm_gnamechange", 1, adminfile);
	dbg_msg(2, "SUCCESS", " * Admin parameters loaded from '%s'.", adminfile);

	// Log stuff
	log_admins = GetPrivateProfileInt("log", "log_admins", 1, logfile);
	log_players = GetPrivateProfileInt("log", "log_players", 1, logfile);
	log_ips = GetPrivateProfileInt("log", "log_ips", 1, logfile);
	dbg_msg(2, "SUCCESS", " * Log parameters loaded from '%s'.", logfile);

	// Ingame stuff
	                         GetPrivateProfileString("game", "server_name",            "my FlyFF server", server_name, sizeof(server_name), gamefile);
	rates_exp              = GetPrivateProfileInt(   "game", "rates_exp",              10,                                                  gamefile);
	rates_drop             = GetPrivateProfileInt(   "game", "rates_drop",             10,                                                  gamefile);
	rates_penya            = GetPrivateProfileInt(   "game", "rates_penya",            10,                                                  gamefile);
	show_chats             = GetPrivateProfileInt(   "game", "show_chats",             0,                                                   gamefile);
	notice_loop_interval   = GetPrivateProfileInt(   "game", "notice_loop_interval",   3600,                                                gamefile);
							 GetPrivateProfileString("game", "notice_loop_message_file","./config/notice_loop.txt", notice_loop_message_file, sizeof(notice_loop_message_file), gamefile); 
	min_admin_level        = GetPrivateProfileInt(   "game", "min_admin_level",        0,                                                   gamefile);
	time_diff= ::time_diff = GetPrivateProfileInt(   "game", "time_diff",              0,                                                   gamefile);
							 GetPrivateProfileString("game", "allowed_name_chars",     "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZäöüÄÖÜ1234567890[]", allowed_name_chars, sizeof(allowed_name_chars), gamefile);
	dbg_msg(2, "SUCCESS", " * Game parameters loaded from '%s'.", gamefile);


	// Lang
	GetPrivateProfileString("lang", "session_error", "Error while retrieving the session ID.", session_error,  sizeof(session_error), langfile); 
	GetPrivateProfileString("lang", "login_error", "Error while logging in.", login_error,  sizeof(login_error), langfile); 
	GetPrivateProfileString("lang", "banned_account", "Your account is banned permanently.", banned_account,  sizeof(banned_account), langfile); 
	GetPrivateProfileString("lang", "banned_ip", "Your IP is banned permanently!", banned_ip,  sizeof(banned_ip), langfile); 

	GetPrivateProfileString("lang", "welcome_notice", "Welcome to &SERVER_NAME&, &CHARACTER&.", welcome_notice,  sizeof(welcome_notice), langfile); 

	GetPrivateProfileString("lang", "cmds_welcome", "Use %commands to get a list of all commands.", cmds_welcome,  sizeof(cmds_welcome), langfile); 
	GetPrivateProfileString("lang", "admin_too_low", "The server is not open to your adminlevel.", admin_too_low,  sizeof(admin_too_low), langfile); 
	GetPrivateProfileString("lang", "admin_auth", "Successfully authed with level &ADMINLEVEL&.", admin_auth,  sizeof(admin_auth), langfile); 
	GetPrivateProfileString("lang", "cmds_av", "Commands available:", cmds_av,  sizeof(cmds_av), langfile); 
	GetPrivateProfileString("lang", "players_online", "Players online: &PLAYERS_ONLINE&.", players_online,  sizeof(players_online), langfile); 

	GetPrivateProfileString("lang", "change_status", "Status changed to &STATUS&.", change_status,  sizeof(change_status), langfile); 

	GetPrivateProfileString("lang", "inv_cmd", "Invalid command.", inv_cmd,  sizeof(inv_cmd), langfile); 
	GetPrivateProfileString("lang", "inv_arg", "Invalid argument.", inv_arg,  sizeof(inv_arg), langfile); 
	GetPrivateProfileString("lang", "inv_lvl", "Invalid level.", inv_lvl,  sizeof(inv_lvl), langfile);

	GetPrivateProfileString("lang", "not_admin", "You aren't an admin.", not_admin,  sizeof(not_admin), langfile); 

	GetPrivateProfileString("lang", "inv_face_change", "Invalid face change -> Kicked.", inv_face_change,  sizeof(inv_face_change), langfile); 
	GetPrivateProfileString("lang", "inv_name_change", "Invalid name change -> Blocked.", inv_name_change,  sizeof(inv_name_change), langfile); 
	GetPrivateProfileString("lang", "inv_gname_change", "Invalid guildname change -> Blocked.", inv_gname_change,  sizeof(inv_gname_change), langfile); 

	GetPrivateProfileString("lang", "exc_face_change", "Excessive facechange -> Blocked.", exc_face_change,  sizeof(exc_face_change), langfile); 
	GetPrivateProfileString("lang", "exc_buffing", "Excessive buffing -> Blocked.", exc_buffing,  sizeof(exc_buffing), langfile); 
	GetPrivateProfileString("lang", "exc_chatting", "Excessive chatting -> Blocked.", exc_chatting,  sizeof(exc_chatting), langfile); 
	GetPrivateProfileString("lang", "exc_shouting", "Excessive shouting -> Blocked.", exc_shouting,  sizeof(exc_shouting), langfile); 
	GetPrivateProfileString("lang", "exc_item", "Excessive item usage -> Blocked.", exc_item,  sizeof(exc_item), langfile); 
	GetPrivateProfileString("lang", "exc_drop", "Excessive dropping -> Blocked.", exc_drop,  sizeof(exc_drop), langfile); 
	GetPrivateProfileString("lang", "exc_safety_upgrading", "Excessive safety upgrading -> Blocked.", exc_upgrade,  sizeof(exc_upgrade), langfile); 

	GetPrivateProfileString("lang", "no_gm_drop", "Your adminlevel is not allowed to drop items.", no_gm_drop,  sizeof(no_gm_drop), langfile); 
	GetPrivateProfileString("lang", "no_gm_attack", "Your adminlevel is not allowed to attack.", no_gm_attack,  sizeof(no_gm_attack), langfile); 
	GetPrivateProfileString("lang", "no_gm_skill", "Your adminlevel is not allowed to use skills.", no_gm_skill,  sizeof(no_gm_skill), langfile); 
	GetPrivateProfileString("lang", "no_gm_guild_bank", "Your adminlevel is not allowed to use the guild bank.", no_gm_guild_bank,  sizeof(no_gm_guild_bank), langfile); 
	GetPrivateProfileString("lang", "no_gm_bank", "Your adminlevel is not allowed to use the bank.", no_gm_bank,  sizeof(no_gm_bank), langfile); 
	GetPrivateProfileString("lang", "no_gm_mail", "Your adminlevel is not allowed to send mails.", no_gm_mail,  sizeof(no_gm_mail), langfile); 
	GetPrivateProfileString("lang", "no_gm_shop_create", "Your adminlevel is not allowed to create shops.", no_gm_shop_create,  sizeof(no_gm_shop_create), langfile); 
	GetPrivateProfileString("lang", "no_gm_shop_buy", "Your adminlevel is not allowed to buy from shops.", no_gm_shop_buy,  sizeof(no_gm_shop_buy), langfile); 
	GetPrivateProfileString("lang", "no_gm_trade_item", "Your adminlevel is not allowed to trade items.", no_gm_trade_item,  sizeof(no_gm_trade_item), langfile); 
	GetPrivateProfileString("lang", "no_gm_trade_money", "Your adminlevel is not allowed to trade money.", no_gm_trade_money,  sizeof(no_gm_trade_money), langfile);
	GetPrivateProfileString("lang", "no_gm_namechange", "Your adminlevel is not allowed to change the name.", no_gm_namechange,  sizeof(no_gm_namechange), langfile); 
	GetPrivateProfileString("lang", "no_gm_gnamechange", "Your adminlevel is not allowed to change the guild name.", no_gm_gnamechange,  sizeof(no_gm_gnamechange), langfile);

	GetPrivateProfileString("lang", "element_scroll", "Elemental scroll can crash the server. -> Blocked.", element_scroll,  sizeof(element_scroll), langfile); 
	GetPrivateProfileString("lang", "npc_upgrade", "NPC upgrading can crash the server. -> Blocked.", npc_upgrade,  sizeof(npc_upgrade), langfile); 
	GetPrivateProfileString("lang", "equip_crash", "Equipment crash packet. -> Blocked.", equip_crash,  sizeof(equip_crash), langfile); 
	GetPrivateProfileString("lang", "transy", "Transy used -> To prevent stacking, please relog.", transy,  sizeof(transy), langfile);
	GetPrivateProfileString("lang", "bag_dupe", "Bag dupe. -> Blocked.", bag_dupe,  sizeof(bag_dupe), langfile); 
	GetPrivateProfileString("lang", "bag_crash", "Bag crash. -> Blocked.", bag_crash,  sizeof(bag_crash), langfile); 
	GetPrivateProfileString("lang", "attack_24", "2/4 attack. -> Blocked.", attack_24,  sizeof(attack_24), langfile); 
	dbg_msg(2, "SUCCESS", " * Language loaded from '%s'.", langfile);


	// SUCESS! :D
	dbg_msg(2, "SUCCESS", " -> Configuration loaded in %d msec.", clock() - begin);
}