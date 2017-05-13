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

#ifndef CONCOMMANDTESTER_H
#define CONCOMMANDTESTER_H

#include "Systems/Testers/Detections/temp_BaseDetection.h" // + basic_string + memset/cpy + logger + basesystem + singleton + helpers + cutlvector
#include "Hooks/ConCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"

typedef struct PlayerConCommandS
{
	basic_string cmd;
	bool isSpamIgnored;
	float time;

	PlayerConCommandS () : 
		cmd ()
	{
		isSpamIgnored = false;
		time = 0.0;
	};
	PlayerConCommandS ( const basic_string& command, bool ignore_spam, float cmd_time ) : cmd ()
	{
		cmd = command;
		isSpamIgnored = ignore_spam;
		time = cmd_time;
	};
	PlayerConCommandS ( const PlayerConCommandS& other ) : cmd ()
	{
		cmd = other.cmd;
		isSpamIgnored = other.isSpamIgnored;
		time = other.time;
	};

	bool operator== ( const PlayerConCommandS& other ) const
	{
		if( cmd != other.cmd ) return false;
		if( time != other.time ) return false;
		return true;
	};
} PlayerConCommandT;

typedef struct LastPlayerCommandsS :
	public Helpers::CRC32_Specialize
{
	typedef CUtlVector<PlayerConCommandT> cmd_list_t;
	cmd_list_t commands;

	LastPlayerCommandsS () : 
		CRC32_Specialize (),
		commands ()
	{};

	LastPlayerCommandsS ( const LastPlayerCommandsS& other ) : commands ()
	{
		commands = commands;
	};

	virtual uint32_t Hash_CRC32 ()
	{
		Helpers::CRC32_Digestive ctx;
		for( cmd_list_t::iterator it ( commands.begin () ); it != commands.end (); ++it )
		{
			ctx.Digest ( it->cmd.c_str (), it->cmd.size () );
			//ctx.Digest ( &(it->isSpamIgnored), sizeof(bool) );
			//ctx.Digest ( &(it->time), sizeof ( float ) );
		}
		return ctx.Final ();
	}

	// Remove commands that are older than 1 second from now
	void Clean ()
	{
		const float now = Plat_FloatTime ();
		while( !commands.IsEmpty () )
		{
			if( fabs ( now ) - fabs ( commands[ 0 ].time ) >= 1.0 ) commands.Remove ( 0 );
			else break;
		}
	}

	void AddCmd ( const basic_string& command, bool ignore_spam )
	{
		Clean ();

		commands.AddToTail ( PlayerConCommandS ( command, ignore_spam, Plat_FloatTime () ) );
	};

	size_t GetSpamCmdCount ()
	{
		size_t ret = 0;
		size_t id = 0;
		size_t const max = commands.Size ();

		for( PlayerConCommandT const * current = &commands[ id ]; id < max; current = &commands[ ++id ] )
		{
			if( !current->isSpamIgnored )	++ret;
		}
		return ret;
	};
} LastPlayerCommandsT;

typedef struct CommandInfoS
{
	basic_string command_name;
	bool ignore;
	bool ban;

	CommandInfoS () : command_name ()
	{
		ignore = ban = false;
	};
	CommandInfoS ( const basic_string& name, bool b1, bool b2 ) : command_name ()
	{
		command_name = name;
		ignore = b1;
		ban = b2;
	};
	CommandInfoS ( const CommandInfoS& other ) : command_name ()
	{
		command_name = other.command_name;
		ignore = other.ignore;
		ban = other.ban;
	};
	bool operator== ( const CommandInfoS& other ) const
	{
		if( command_name == other.command_name ) return true;
		return false;
	};
} CommandInfoT;

typedef CUtlVector<CommandInfoT> CommandListT;

class ConCommandTester :
	public BaseDynamicSystem,
	public ConCommandHookListener,
	public PlayerDataStructHandler<LastPlayerCommandsT>,
	public Singleton<ConCommandTester>
{
	typedef Singleton<ConCommandTester> singleton_class;
	typedef PlayerDataStructHandler<LastPlayerCommandsT> playerdatahandler_class;

private:
	CommandListT m_commands_list;

public:
	ConCommandTester ();
	virtual ~ConCommandTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual bool RT_ConCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, SourceSdk::CCommand const & args ) override final;

public:
	/* Called by the plugin */
	bool RT_TestPlayerCommand ( PlayerHandler::const_iterator ph, const basic_string& command );
	bool RT_TestPlayerCommand_Anon ( PlayerHandler::const_iterator ph, const basic_string& command );

private:
	void AddCommandInfo ( const basic_string& name, const bool ignore = false, const bool ban = false );

	void RemoveCommandInfo ( const basic_string& name );

	void RT_AddPlayerCommand ( PlayerHandler::const_iterator ph, const basic_string& command );

	static bool RT_HookSayCallback ( PlayerHandler::const_iterator ph, const void* const command, const SourceSdk::CCommand & args );

	static bool RT_HookEntCallback ( PlayerHandler::const_iterator ph, const void* const command, const SourceSdk::CCommand & args );
};

class Detection_CmdFlood : public LogDetection<LastPlayerCommandsT>
{
	typedef LogDetection<LastPlayerCommandsT> hClass;
public:
	Detection_CmdFlood ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, LastPlayerCommandsT const * data ) :
		hClass ( player, tester, UniqueDetectionID::CMD_FLOOD, data )
	{}
	virtual ~Detection_CmdFlood ()
	{}

	virtual void TakeAction () override final;

	virtual void WriteXMLOutput ( FILE * const ) const final;

	virtual bool CloneWhenEqual () const final
	{
		return true;
	}

	virtual basic_string GetDetectionLogMessage () const final
	{
		return "ConCommand Flood";
	}
};

class Detection_CmdViolation : public LogDetection<LastPlayerCommandsT>
{
	typedef LogDetection<LastPlayerCommandsT> hClass;
public:
	Detection_CmdViolation ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, LastPlayerCommandsT const * data ) :
		hClass ( player, tester, UniqueDetectionID::CMD_VIOLATION, data )
	{};
	virtual  ~Detection_CmdViolation ()
	{}

	virtual void TakeAction () override final;

	virtual void WriteXMLOutput ( FILE * const ) const final;

	virtual bool CloneWhenEqual () const final
	{
		return true;
	}

	virtual basic_string GetDetectionLogMessage () const final
	{
		return "ConCommand Violation";
	}
};

#endif
