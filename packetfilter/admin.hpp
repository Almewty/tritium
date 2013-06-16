#ifndef ADMIN_H
#define ADMIN_H



// Admin names and rights themselves
class ADMIN
{
public:
	ADMIN(char* _name, int _level);
	~ADMIN();

	// Name
	char name[33];

	// Status
	char status[128];

	// Rights
	int level;

private:
};


// The manager
class ADMINMANAGER
{
public:
	ADMINMANAGER();
	~ADMINMANAGER();


	// Get admin object per name
	ADMIN* get_admin(char* _name);

	void add_admin(char* _name, int _level);
	void remove_admin(char* _name);

	// Reload
	void reload_admins();

	// Mutex
	pthread_mutex_t mutex_admins;

	// Vector for the admin objects
	std::vector<ADMIN*> admins;

private:

	// Load admin objects
	void load_admins();
	void unload_admins();
};

#endif