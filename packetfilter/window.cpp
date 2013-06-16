#include "stdafx.h"
#include "engine.hpp"
#include "window.h"
#include "resource.h"
#include "network.hpp"
#include "cfg.hpp"
#include "cmd.hpp"
#include "admin.hpp"
#include "ban.hpp"

// Extern globals
extern NETWORK* net;
extern CFGMANAGER* cfg;
extern CMDMANAGER* cmdmanager;
extern ADMINMANAGER* adminmanager;
extern BANMANAGER* banmanager;

extern char tom94[7];

// Eventmanager des windows
LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	// Title
	char title[128] = "Antihack v"VERSION"\n(C) by ";
	strcat_s(title, sizeof(title), tom94);

	// Create the sys menu
	static HMENU hSysMenu;

	// Window handles of the buttons
	static HWND button_outall;
	static HWND button_execute;
	static HWND button_reload;
	static HWND button_sconsole;
	static HWND button_hconsole;

	static HWND edit_execute;

	switch(umsg)
	{
	case WM_CREATE:

		// Alter sys menu
		{
			hSysMenu = GetSystemMenu(hWnd, FALSE);

			// Entries 1, 3, 5... 0,1,2 because it always counts from the current state! (beginning from 0)
			RemoveMenu(hSysMenu, 0, MF_BYPOSITION);
			RemoveMenu(hSysMenu, 1, MF_BYPOSITION);
			RemoveMenu(hSysMenu, 2, MF_BYPOSITION);
		}

		// Create the buttons
		{
			button_outall = CreateWindow("button",
										 "Outall",
										 WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
										 10,
										 213,
										 60,
										 20,
										 hWnd,
										 (HMENU)ID_BUTTON_OUTALL,
										 ((LPCREATESTRUCT)lParam)->hInstance,
										 NULL);

			button_reload = CreateWindow("button",
										 "Reload",
										 WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
										 75,
										 213,
										 60,
										 20,
										 hWnd,
										 (HMENU)ID_BUTTON_RELOAD,
										 ((LPCREATESTRUCT)lParam)->hInstance,
										 NULL);

			button_execute = CreateWindow("button",
								 		  "Execute",
							 			  WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
						 				  10,
										  188,
										  60,
										  20,
										  hWnd,
										  (HMENU)ID_BUTTON_EXECUTE,
										  ((LPCREATESTRUCT)lParam)->hInstance,
										  NULL);

			button_sconsole = CreateWindow("button",
										   "Show console",
										   WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
										   350,
										   155,
										   175,
										   20,
										   hWnd,
										   (HMENU)ID_BUTTON_SCONSOLE,
										   ((LPCREATESTRUCT)lParam)->hInstance,
										   NULL);

			button_hconsole = CreateWindow("button",
										   "Hide console",
										   WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
										   350,
										   155,
										   175,
										   20,
										   hWnd,
										   (HMENU)ID_BUTTON_HCONSOLE,
										   ((LPCREATESTRUCT)lParam)->hInstance,
										   NULL);

			// Disable the show button :>
			ShowWindow(GetDlgItem(hWnd, ID_BUTTON_SCONSOLE), SW_HIDE);
		}

		// Edit fields
		{
			edit_execute   = CreateWindow("edit",
										  "This is an input command.",
										  WS_CHILD|WS_VISIBLE|WS_BORDER,
										  75,
										  188,
										  450,
										  20,
										  hWnd,
										  (HMENU)ID_EDIT_EXECUTE,
										  ((LPCREATESTRUCT)lParam)->hInstance,
										  NULL);

			/*edit_notice_gm = CreateWindow("edit",
										  "This is a GM notice.",
										  WS_CHILD|WS_VISIBLE|WS_BORDER,
										  75,
										  213,
										  550,
										  20,
										  hWnd,
										  (HMENU)ID_EDIT_NOTICE_GM,
										  ((LPCREATESTRUCT)lParam)->hInstance,
										  NULL);*/

			/*edit_account = CreateWindow("edit",
										"Accountname.",
										WS_CHILD|WS_VISIBLE|WS_BORDER,
										325,
										250,
										300,
										20,
										hWnd,
										(HMENU)ID_EDIT_ACCOUNT,
										((LPCREATESTRUCT)lParam)->hInstance,
										NULL);

			edit_character = CreateWindow("edit",
										  "Charactername.",
										  WS_CHILD|WS_VISIBLE|WS_BORDER,
										  325,
										  275,
										  300,
										  20,
										  hWnd,
										  (HMENU)ID_EDIT_CHARACTER,
										  ((LPCREATESTRUCT)lParam)->hInstance,
										  NULL);

			edit_ip = CreateWindow("edit",
								   "IP address.",
								   WS_CHILD|WS_VISIBLE|WS_BORDER,
								   325,
								   300,
								   300,
								   20,
								   hWnd,
								   (HMENU)ID_EDIT_IP,
								   ((LPCREATESTRUCT)lParam)->hInstance,
								   NULL);

			edit_level = CreateWindow("edit",
								      "Adminlevel.",
								      WS_CHILD|WS_VISIBLE|WS_BORDER,
								      325,
								      325,
								      300,
								      20,
								      hWnd,
								      (HMENU)ID_EDIT_LEVEL,
								      ((LPCREATESTRUCT)lParam)->hInstance,
								      NULL);*/
		}
		break;

	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hDC;
		char buf[128];

		hDC = BeginPaint(hWnd, &ps);

		// Draw the messages
		{
			// Left side
			sprintf_s(buf, sizeof(buf), "Server:");
			TextOut(hDC, 10, 5,   buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Rates:");
			TextOut(hDC, 10, 35,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Version:");
			TextOut(hDC, 10, 65,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Time running:");
			TextOut(hDC, 10, 95,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Clock:");
			TextOut(hDC, 10, 125, buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Date:");
			TextOut(hDC, 10, 155, buf, strlen(buf));

			// Right side
			sprintf_s(buf, sizeof(buf), "Players online:");
			TextOut(hDC, 350, 5,   buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Connections:");
			TextOut(hDC, 350, 35,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Existing admins:");
			TextOut(hDC, 350, 65,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Existing bans:");
			TextOut(hDC, 350, 95,  buf, strlen(buf));
			sprintf_s(buf, sizeof(buf), "Existing commands:");
			TextOut(hDC, 350, 125, buf, strlen(buf));
		}

		// Draw the values
		{
			// Left side
			 // Servername
			 TextOut(hDC, 110, 5,   cfg->server_name, strlen(cfg->server_name));

			 // Rates
			 sprintf_s(buf, sizeof(buf), "EXP: %d | Drop: %d | Penya: %d",
										 cfg->rates_exp,
										 cfg->rates_drop,
										 cfg->rates_penya);
			 TextOut(hDC, 110, 35,  buf, strlen(buf));

			 // Version
			 TextOut(hDC, 110, 65,  "Antihack v"VERSION, strlen("Antihack v"VERSION));

			 // Time running
			 sprintf_s(buf, sizeof(buf), "%02d:%02d:%02d",
										 ((clock()-net->starttime) / 3600000) % 60,
										 ((clock()-net->starttime) / 60000) % 60,
										 ((clock()-net->starttime) / 1000) % 60);
			 TextOut(hDC, 110, 95,  buf, strlen(buf));

			 // Clock
			 SYSTEMTIME time;
			 GetSystemTime(&time);
			 sprintf_s(buf, sizeof(buf), "%02d:%02d:%02d",
										 ((time.wHour+cfg->time_diff) % 24),
										 time.wMinute,
										 time.wSecond);
			 TextOut(hDC, 110, 125, buf, strlen(buf));

			 // Date
			 sprintf_s(buf, sizeof(buf), "%02d/%02d/%04d",
										 time.wDay,
										 time.wMonth,
										 time.wYear);
			 TextOut(hDC, 110, 155, buf, strlen(buf));


			// Right side
			 // Players
			 sprintf_s(buf, sizeof(buf), "%d", net->win_players_count);
			 TextOut(hDC, 490, 5,   buf, strlen(buf));

			 // Conns
			 sprintf_s(buf, sizeof(buf), "%d", net->win_connections_count);
			 TextOut(hDC, 490, 35,  buf, strlen(buf));

			 // Admins
			 sprintf_s(buf, sizeof(buf), "%d", net->win_admins_count);
			 TextOut(hDC, 490, 65,  buf, strlen(buf));

			 // Bans
			 sprintf_s(buf, sizeof(buf), "%d", net->win_bans_count);
			 TextOut(hDC, 490, 95,  buf, strlen(buf));

			 // Cmds
			 sprintf_s(buf, sizeof(buf), "%d", net->win_cmds_count);
			 TextOut(hDC, 490, 125, buf, strlen(buf));
		}

		// Vertical lines
		{
			// Init pen
			HPEN pen;
			pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            SelectObject(hDC, pen);

			// Draw line
            MoveToEx(hDC, 0, 28, NULL);
            LineTo(hDC, 650, 28);

			// Draw line
            MoveToEx(hDC, 0, 58, NULL);
            LineTo(hDC, 650, 58);

			// Draw line
            MoveToEx(hDC, 0, 88, NULL);
            LineTo(hDC, 650, 88);

			// Draw line
            MoveToEx(hDC, 0, 118, NULL);
            LineTo(hDC, 650, 118);

			// Draw line
            MoveToEx(hDC, 0, 148, NULL);
            LineTo(hDC, 650, 148);

			// Draw line
            MoveToEx(hDC, 0, 178, NULL);
            LineTo(hDC, 650, 178);

			// Draw line
            MoveToEx(hDC, 0, 240, NULL);
            LineTo(hDC, 650, 240);

			// Unload pen
            DeleteObject(pen);
		}

		// Horizontal lines
		{
			// Init pen
			HPEN pen;
			pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            SelectObject(hDC, pen);

			// Draw line
            MoveToEx(hDC, 340, 0, NULL);
            LineTo(hDC, 340, 178);

			// Unload pen
            DeleteObject(pen);
		}

		EndPaint(hWnd, &ps);
		break;

	case WM_CLOSE:
		net->send_global_notice("All players got kicked. (Antihack shutdown)");
		net->out_all();
		Sleep(1000);
		net->crashed = true;
		break;


    case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		// Menu buttons
		case ID_BUTTON_ABOUT:
            MessageBox(hWnd, title, "ABOUT", MB_ICONINFORMATION);
            break;


		// Buttons
		case ID_BUTTON_SHUTDOWN:
			net->send_global_notice("All players got kicked. (Antihack shutdown)");
			net->out_all();
			Sleep(1000);
			net->crashed = true;
			break;


		case ID_BUTTON_OUTALL:
			net->send_global_notice("All players got kicked. (Admin command)");
			net->out_all();
			break;


		case ID_BUTTON_RELOAD:
			cfg->load_cfg();
			cmdmanager->reload_cmds();
			adminmanager->reload_admins();
			net->reload_admins();
			banmanager->reload_bans();
			break;


		case ID_BUTTON_EXECUTE:
			{
				CHAR _msg[256];
				WORD _len;

				_len = (WORD)SendDlgItemMessage(hWnd, 
												ID_EDIT_EXECUTE, 
												EM_LINELENGTH, 
												(WPARAM) 0, 
												(LPARAM) 0); 

				*((LPWORD)_msg) = _len;

				SendDlgItemMessage(hWnd, 
	                               ID_EDIT_EXECUTE, 
	                               EM_GETLINE, 
	                               (WPARAM)0, 
	                               (LPARAM)_msg); 

				_msg[_len] = '\0';

				dbg_msg(3, "INFO", "Executing command '%s' from GUI.", _msg);
				net->execute_cmd(_msg);
			}
			break;


		case ID_BUTTON_SCONSOLE:
			{
				/*char _msg[256] = "[GM chat] ";
				int _len;

				_len = (WORD)SendDlgItemMessage(hWnd, 
												ID_EDIT_NOTICE_GM, 
												EM_LINELENGTH, 
												(WPARAM) 0, 
												(LPARAM) 0); 

				*((LPWORD)&_msg[10]) = _len;

				SendDlgItemMessage(hWnd, 
	                               ID_EDIT_NOTICE_GM, 
	                               EM_GETLINE, 
	                               (WPARAM)0, 
	                               (LPARAM)&_msg[10]); 

				_msg[_len+10] = '\0';

				net->send_gm_notice(_msg);*/

				// Show console
				HWND hConsole = GetConsoleWindow();
				ShowWindow(hConsole, SW_SHOW);

				ShowWindow(GetDlgItem(hWnd, ID_BUTTON_HCONSOLE), SW_SHOW);
				ShowWindow(GetDlgItem(hWnd, ID_BUTTON_SCONSOLE), SW_HIDE);

				SetActiveWindow(hWnd);
			}
			break;

		case ID_BUTTON_HCONSOLE:
			{
				// Hide console
				HWND hConsole = GetConsoleWindow();
				ShowWindow(hConsole, SW_HIDE);

				ShowWindow(GetDlgItem(hWnd, ID_BUTTON_SCONSOLE), SW_SHOW);
				ShowWindow(GetDlgItem(hWnd, ID_BUTTON_HCONSOLE), SW_HIDE);
			}
			break;
		}
		break;
	}

	return DefWindowProc(hWnd, umsg, wParam, lParam);
}