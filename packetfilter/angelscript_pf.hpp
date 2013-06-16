#include "stdafx.h"

#include <iostream>
#include <assert.h>
#include <string.h>

#include <angelscript.h>
#include "scriptbuilder.h"
#include "scriptstring.h"
#include "contextmgr.h"

class PF_AS
{
public:
	PF_AS();
	~PF_AS();

	// Dynamic management of some permanent scripts
	asIScriptContext* add_script(char* name);
	void execute_scripts();

	// To create own contexes
	asIScriptEngine* engine;

	pthread_mutex_t add_ctx;

private:
	void init();

	int compile_script();
	void configure_engine();

	CContextMgr context_manager;
};