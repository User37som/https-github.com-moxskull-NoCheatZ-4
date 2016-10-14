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
	// Used inside ConCommands
	MSG_CMD_REPLY
};

class Logger :
	public BaseStaticSystem,
	public Singleton<Logger>
{
	typedef Singleton<Logger> singleton_class;

	typedef void ( *MsgFunc_t ) ( const char* pMsg, ... );

private:
	CUtlVector<basic_string> m_msg;
	basic_string const prolog;

	MsgFunc_t m_msg_func;

public:
	Logger () : BaseStaticSystem ( "Logger" ), singleton_class (), m_msg (), prolog ( "[NoCheatZ " NCZ_VERSION_STR "] " ), m_msg_func( nullptr )
	{
		ConnectToServerConsole ();
	};
	virtual ~Logger () override final
	{
		m_msg_func = nullptr;
	};

	virtual void Init () override final
	{};

	void ConnectToServerConsole ();

	inline bool IsConsoleConnected () const;

	void Push ( const char * msg );
	void Flush ();

	template <msg_type type = MSG_CONSOLE>
	void Msg ( const char * msg, int verbose = 0 );

	template <msg_type type = MSG_CONSOLE>
	inline void Msg ( const basic_string& msg, int verbose = 0 )
	{
		Msg<type> ( msg.c_str (), verbose );
	}
};

inline bool Logger::IsConsoleConnected () const
{
	return m_msg_func != nullptr;
}

#define SystemVerbose1(x) if( this->m_verbose >= 1 ) Logger::GetInstance()->Msg<MSG_VERBOSE1>(x, 1)
#define SystemVerbose2(x) if( this->m_verbose >= 2 ) Logger::GetInstance()->Msg<MSG_VERBOSE2>(x, 2)

#ifdef DEBUG
#	define DebugMessage(x) Logger::GetInstance()->Msg<MSG_DEBUG>(x, 3)
#else
#	define DebugMessage(x)
#endif

#endif
