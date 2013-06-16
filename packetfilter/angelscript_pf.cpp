#include "stdafx.h"
#include "engine.hpp"

#include <assert.h>

#include <angelscript.h>
#include "scriptbuilder.h"
#include "scriptstring.h"
#include "contextmgr.h"

#include "angelscript_pf.hpp"

#include "cfg.hpp"
#include "network.hpp"
#include "connection.hpp"


// Extern globals
extern CFGMANAGER* cfg;
extern NETWORK*    net;

// Intern global
PF_AS* as;


using namespace std;

PF_AS::PF_AS()
:
engine(0)
{
	init();
}

PF_AS::~PF_AS()
{
	// Release the engine
	engine->Release();
}

asIScriptContext* PF_AS::add_script(char* name)
{
	int funcId = engine->GetModule(0)->GetFunctionIdByDecl(name);
	if(funcId < 0)
	{
		dbg_msg(4, "ERROR", "The function '%s' was not found.", name);
		exit(-1);
	}

	return context_manager.AddContext(engine, funcId);
}

void PF_AS::execute_scripts()
{
	pthread_mutex_lock(&add_ctx);
	context_manager.ExecuteScripts();
	pthread_mutex_unlock(&add_ctx);
}

static void msg_callback(const asSMessageInfo *msg, void *param)
{
	if(msg->type == asMSGTYPE_WARNING)
		dbg_msg(4, "WARNING", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
	else if(msg->type == asMSGTYPE_INFORMATION)
		dbg_msg(3, "INFO", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
	else
		dbg_msg(4, "ERROR", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
}

void PF_AS::init()
{
	dbg_msg(3, "INFO", "Attempting to initialize AngelScript.");

	int begin = clock();

	// Setup the context manager
	context_manager.SetGetTimeCallback((TIMEFUNC_t)&clock);

	// Create the script engine
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if(engine == 0)
	{
		dbg_msg(4, "ERROR", " -> Failed to create script engine.");
		exit(-1);
	}

	// The script compiler will send any compiler messages to the callback function
	engine->SetMessageCallback(asFUNCTION(msg_callback), 0, asCALL_CDECL);

	// Configure the engine
	configure_engine();

	// Compile the script code
	int r = compile_script();
	if(r < 0)
		exit(-1);

	dbg_msg(2, "SUCCESS", " -> Initialized AngelScript in %d msec.", clock() - begin);
}

int PF_AS::compile_script()
{
	int r;
	CScriptBuilder builder;

	r = builder.StartNewModule(engine, 0);
	if(r < 0)
	{
		dbg_msg(4, "ERROR", "Failed to start new module.");
		return r;
	}

	r = builder.AddSectionFromFile(cfg->asfile);
	if( r < 0 )
	{
		dbg_msg(4, "ERROR", "Failed to add script file.");
		return r;
	}

	r = builder.BuildModule();
	if( r < 0 )
	{
		dbg_msg(4, "ERROR", "Failed to build the module.");
		return r;
	}

	return 0;
}

// -------------------------
// --- Proxy Funcs ---------
// -------------------------

static void print_string(int color, string &type, string &str)
{
	dbg_msg(color, type.c_str(), str.c_str());
}

void PF_AS::configure_engine()
{
	int r;

	RegisterScriptString(engine);

	r = engine->RegisterGlobalFunction("void dbg_msg(int color, string &type, string &str)", asFUNCTION(print_string), asCALL_CDECL); assert( r >= 0 );
	
	// Register network
	r = engine->RegisterObjectType("World", 0, asOBJ_REF | asOBJ_NOHANDLE); assert( r >= 0 );
	r = engine->RegisterObjectMethod("World", "void out_all()", asMETHOD(NETWORK, out_all), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("World", "void send_global_notice(string& msg)", asMETHOD(NETWORK, send_global_notice_as), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("World world", net); assert( r >= 0 );

	// Register connections
	r = engine->RegisterObjectType("Player", 0, asOBJ_REF); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("Player", asBEHAVE_ADDREF, "void f()", asMETHOD(CONNECTION, AddRef),  asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Player", asBEHAVE_RELEASE, "void f()", asMETHOD(CONNECTION, Release),  asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Player", "void send_notice(string& msg)", asMETHOD(CONNECTION, send_notice_as), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectProperty("Player", "bool active", offsetof(CONNECTION, active)); assert( r >= 0 );



	context_manager.RegisterThreadSupport(engine);
}