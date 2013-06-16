#include "stdafx.h"
#include "engine.hpp"
#include "network.hpp"

// Get the extern global
extern pthread_mutex_t mutex_msg;
extern char logfolder[500];
extern char allowed_name_chars[500];
extern int time_diff;


char tom[5];
char tom94[7];
char prefix[19];
char link[50];

char md5tom[33];
char md5tom94[33];
char md5prefix[33];
char md5link[33];


//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
int main(int argc, char* argv[])
{
	strcpy(tom, "Tom");
	strcpy(tom94, "Tom94");
	strcpy(prefix, "[Tom's Antihack] ");
	strcpy(link, "www.Tom94.net");



	md5(tom, md5tom);
	md5(tom94, md5tom94);
	md5(prefix, md5prefix);
	md5(link, md5link);



	if(strcmp("d9ffaca46d5990ec39501bcdf22ee7a1", md5tom) ||
	   strcmp("d271aef3a8d0b03f11876898957d40f2", md5tom94) ||
	   strcmp("e90e771710d95e5ad012ba87a48eecfd", md5prefix) ||
	   strcmp("73347c6e530d11dbccafc9b70d7bd039", md5link))
	{
		// Er hats verdient
		printf("He du Spast, mach nen eigenen Antihack wenn dein Name drin stehen soll!\n");
		system("PAUSE");
		return 0;
	}


	printf("%s %s %s %s\n", tom, tom94, prefix, link);



	/*FILE* file;
	file = fopen("hashs.txt", "w");
	fprintf(file, "%s\n%s\n%s\n%s\n", md5tom, md5tom94, md5prefix, md5link);
	fclose(file);*/




	// Colors
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 15);

	// Title
	char buf[128] = "Antihack v"VERSION" console | (C) by ";
	strcat_s(buf, sizeof(buf), tom94);
	SetConsoleTitle(buf);

	// Init the mutex
	pthread_mutex_init(&mutex_msg, NULL);

	// Read the log folder
	char gamefile[500];
	GetPrivateProfileString("files", "gamefile", "./game.ini", gamefile, sizeof(gamefile), "./config.ini"); 
	GetPrivateProfileString("files", "logfolder", "./log", logfolder, sizeof(logfolder), "./config.ini"); 
	time_diff = GetPrivateProfileInt("game", "time_diff", 0, gamefile);
	GetPrivateProfileString("game", "allowed_name_chars", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ‰ˆ¸ƒ÷‹1234567890[]", allowed_name_chars, sizeof(allowed_name_chars), gamefile);

	dbg_msg(0, "",     "----------------------------------------------");
	dbg_msg(3, "INFO", "-------- Antihack v"VERSION" (C) by %s --------", tom94);
	dbg_msg(0, "",     "----------------------------------------------");

	NETWORK* network = new NETWORK();
	delete network;

	dbg_msg(0, "", "----------------------------------------------");
	dbg_msg(3, "INFO", "Shutting down...");
	dbg_msg(0, "", "----------------------------------------------\n");

	// Destroy mutex
	pthread_mutex_destroy(&mutex_msg);

	//Sleep(1000);

	//system("PAUSE");

	// Exit it for sure!
	exit(0);

	return 0;
}
