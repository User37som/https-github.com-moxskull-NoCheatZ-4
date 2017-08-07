/*
	Copyright 2012 - Le Padellec Sylvain

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef LOGGER_H
#define LOGGER_H

#include "Interfaces/InterfacesProxy.h"

#include "Preprocessors.h"
#include "Misc/temp_basicstring.h"
#include "Misc/temp_singleton.h"

#include "Containers/utlvector.h"
#include "Systems/BaseSystem.h"

#include "Misc/Tier0Linker.h"

/*
	Messages to be written on the plugin's logfile.

	To prevent the game to hang on write, the flush is done only when we are outside a round.
*/

enum msg_type
{
	// Prints error to stdout and, if possible, nocheatz logs + game chat
	MSG_ERROR = -2,
	// Prints warning to stdout and, if possible, nocheatz logs
	MSG_WARNING = -1,
	// Prints to stdout only
	MSG_CONSOLE = 0,
	// Prints to stdout and game chat
	MSG_CHAT,
	// Prints to stdout and add to queue for logfile
	MSG_LOG,
	MSG_LOG_CHAT,
	// Prints to stderr only
	MSG_HINT,
	// Prints to stderr if verbose is 1
	MSG_VERBOSE1,
	// Prints to stderr if verbose is 2
	MSG_VERBOSE2,
	// Prints to stderr only if the project build is in debug mode and verbose is at least 3
	MSG_DEBUG,
	MSG_CHAT_ADMIN,
	// Used inside ConCommands
	MSG_CMD_REPLY
};

typedef enum logger_chat : size_t
{
	ON = 0,
	ADMIN_AUTO,
	ADMIN_IDS,
	OFF
} logger_chat_t;

class Logger :
	public BaseStaticSystem,
	public Singleton
{
	

private:
	CUtlVector<basic_string> m_msg;
	basic_string const chat_prolog;
	basic_string const log_prolog;
	logger_chat_t m_allow_chat;
	size_t m_current_memory_used;
	void * m_sm_chat;

	bool m_always_flush;
	bool m_bypass_msg;

public:
	Logger () :
		BaseStaticSystem ( "Logger", "Verbose - AlwaysFlush - AllowChat" ),
		Singleton (),
		m_msg (),
		chat_prolog ( basic_string("\x01[\x04NoCheatZ ").append(NCZ_VERSION_GIT_SHORT).append("\x01] ") ),
		log_prolog(basic_string("[NoCheatZ ").append(NCZ_VERSION_GIT_SHORT).append("] ")),
		m_allow_chat(logger_chat_t::ON),
		m_current_memory_used ( 0 ),
		m_sm_chat(nullptr),
		m_always_flush(false),
		m_bypass_msg(false)
	{
	};
	virtual ~Logger () override final
	{
	};

	virtual void Init () override final
	{};

	virtual bool sys_cmd_fn ( const SourceSdk::CCommand &args ) override final;

	void OnLevelInit();

	void SetAlwaysFlush (bool v)
	{
		m_always_flush = v;
	}

	void SetBypassServerConsoleMsg(bool b);

	static void SpewAssert ( char const * expr, char const * file, unsigned int line );

	logger_chat_t GetAllowChat() const
	{
		return m_allow_chat;
	}

	void Push ( const char * msg );
	void Flush ();

	template <msg_type type = MSG_CONSOLE>
	void Msg ( const char * msg, int verbose = 0 );
};

extern Logger g_Logger;

#define SystemVerbose1(x) if( this->m_verbose >= 1 ) g_Logger.Msg<MSG_VERBOSE1>(x, 1)
#define SystemVerbose2(x) if( this->m_verbose >= 2 ) g_Logger.Msg<MSG_VERBOSE2>(x, 2)

#ifndef NO_LOGGER_ASSERT
#	ifdef DEBUG
#		define DebugMessage(x) g_Logger.Msg<MSG_DEBUG>(x, 3)
#		define LoggerAssert(expression) if( (! (!!(expression))) ) Logger::SpewAssert(#expression, __FILE__, __LINE__); Assert(expression)
#	else
#		define DebugMessage(x)
#		define LoggerAssert(expression)
#	endif
#endif
#endif
