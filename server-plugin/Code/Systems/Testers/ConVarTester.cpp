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

#include <stdio.h>

#include "ConVarTester.h"

#include "Systems/BanRequest.h"
#include "Systems/Logger.h"
#include "Hooks/SigScan.h"

/////////////////////////////////////////////////////////////////////////
// ConVarTester
/////////////////////////////////////////////////////////////////////////

ConVarTester::ConVarTester () :
	BaseTesterSystem ( "ConVarTester", "Enable - Disable - Verbose - SetAction - AddRule - RemoveRule - ResetRules" ),
	OnTickListener (),
	playerdata_class (),
	Singleton (),
	var_sv_cheats ( nullptr )
{}

ConVarTester::~ConVarTester ()
{
	Unload ();
}

void ConVarTester::Init ()
{
	InitDataStruct ();
	LoadDefaultRules ();
}

bool ConVarTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void ConVarTester::RT_ProcessOnTick (double const & curtime )
{
	if( m_convars_rules.IsEmpty () ) return;
	if( !IsActive () ) return;

	if( SourceSdk::InterfacesProxy::ConVar_GetBool ( var_sv_cheats ) )
	{
		InitDataStruct();
		return;
	}
	ProcessFilter::InTestsNoBot filter_class;
	/*m_current_player += &filter_class;
	if( m_current_player )
	{
#ifdef DEBUG
		SystemVerbose2 ( Helpers::format ( "ConVarTester : Processing player %s", m_current_player->GetName () ) );
#endif
		RT_ProcessPlayerTest ( m_current_player, curtime );
	}
	else
	{
#ifdef DEBUG
		SystemVerbose2 (  "ConVarTester : Not processing any player this tick" );
#endif
	}*/
	m_current_player = PlayerHandler::end ();
	m_current_player += &filter_class;
	for( ; m_current_player != PlayerHandler::end (); m_current_player += &filter_class )
	{
		RT_ProcessPlayerTest ( m_current_player, curtime );
	}
}

void ConVarTester::RT_ProcessPlayerTest ( PlayerHandler::iterator ph, double const & curtime )
{
	if (ph->GetChannelInfo()->IsTimingOut()) // Do nothing with a legit time out.
	{
		ResetPlayerDataStructByIndex(ph.GetIndex());
	}
	else
	{
		CurrentConVarRequestT* const req(GetPlayerDataStructByIndex(ph.GetIndex()));

		switch (req->status)
		{
			case ConVarRequestStatus::SENT: // Not yet replyed, check for timeout
			{
				if (curtime >= req->timeEnd)
				{
					if (++(req->attempts) > 4)
					{
						req->answer = "NO ANSWER - TIMED OUT";

						ProcessDetectionAndTakeAction<CurrentConVarRequestT>(Detection_ConVarRequestTimedOut(), req, ph, this);
					}
					//g_Logger.Msg<MSG_WARNING> ( Helpers::format ( "ConVarTester : ConVar request timed out for %s (%s) : %d attempts (Cookie %d).", ph->GetName (), m_convars_rules[ req->ruleset ].name, req->attempts+1, req->cookie) );
					req->SendCurrentRequest(ph, curtime, m_convars_rules); // Send the request again
				}
				break;
			}

			default: // Continue to work ...
			{
				req->PrepareNextRequest(m_convars_rules);
				req->SendCurrentRequest(ph, curtime, m_convars_rules);
				break;
			}
		}
	}
}

ConVarInfoT* ConVarTester::RT_FindConvarRuleset ( const char * name )
{
	size_t pos ( 0 );
	size_t const max ( m_convars_rules.Size () );
	while( pos < max )
	{
		if( stricmp ( m_convars_rules[ pos ].name, name ) == 0 )
		{
			return &m_convars_rules[ pos ];
		}
		++pos;
	}
	return nullptr;
}

bool ConVarTester::sys_cmd_fn ( const SourceSdk::CCommand &args )
{
	if (!BaseTesterSystem::sys_cmd_fn(args))
	{
		if (stricmp("AddRule", args.Arg(2)) == 0)
			// example : ncz ConVarTester AddRule sv_cheats 0 NO_VALUE
		{
			if (args.ArgC() == 6)
			{
				ConVarRuleT rule;

				if (stricmp("NO_VALUE", args.Arg(5)) == 0) rule = ConVarRule::NO_VALUE;
				else if (stricmp("SAME", args.Arg(5)) == 0) rule = ConVarRule::SAME;
				else if (stricmp("SAME_FLOAT", args.Arg(5)) == 0) rule = ConVarRule::SAME_FLOAT;
				else if (stricmp("SAME_AS_SERVER", args.Arg(5)) == 0) rule = ConVarRule::SAME_AS_SERVER;
				else if (stricmp("SAME_FLOAT_AS_SERVER", args.Arg(5)) == 0) rule = ConVarRule::SAME_FLOAT_AS_SERVER;
				else if (stricmp("LOWER", args.Arg(5)) == 0) rule = ConVarRule::LOWER;
				else if (stricmp("HIGHER", args.Arg(5)) == 0) rule = ConVarRule::HIGHER;
				/*else if( stricmp ( "INRANGE", args.Arg(5)) == 0) // Need to re-order AddRule arguments ...
				{
					if (args.ArgC() == 7)
					{
						rule = ConVarRule::RANGE;
					}
					else
					{
						g_Logger.Msg<MSG_CMD_REPLY>("INRANGE expects 2 values");
					}
				}*/
				else
				{
					g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Arg %s not found.", args.Arg(5)));
					return true;
				}

				if (strnicmp("sv_", args.Arg(3), 3) == 0) rule = ConVarRule::SAME_AS_SERVER;
				else rule = ConVarRule::SAME;

				basic_string value = args.Arg(4);

				if (value.find('.') != basic_string::npos)
				{
					if (rule == ConVarRule::SAME_AS_SERVER) rule = ConVarRule::SAME_FLOAT_AS_SERVER;
					else rule = ConVarRule::SAME_FLOAT;
				}
				else
				{
					if (rule != ConVarRule::SAME_AS_SERVER) rule = ConVarRule::SAME_FLOAT_AS_SERVER;
					else rule = ConVarRule::SAME_FLOAT;
				}

				AddConvarRuleset(args.Arg(3), args.Arg(4), rule, false);
				g_Logger.Msg<MSG_CMD_REPLY>("Added convar test rule.");
				return true;
			}
			else
			{
				g_Logger.Msg<MSG_CMD_REPLY>("ncz ConVarTester AddRule [ConVar] [value] [TestType]\nTestType : NO_VALUE - SAME - SAME_FLOAT - SAME_AS_SERVER - SAME_FLOAT_AS_SERVER - LOWER - HIGHER");
				return true;
			}
		}
		else if (stricmp("RemoveRule", args.Arg(2)) == 0)
		{
			size_t pos(0);
			size_t const max(m_convars_rules.Size());
			while (pos < max)
			{
				if (stricmp(m_convars_rules[pos].name, args.Arg(3)) == 0)
				{
					m_convars_rules.Remove(pos);
					return true;
				}
				++pos;
			}
			g_Logger.Msg<MSG_CMD_REPLY>("Can't find such convar to remove.");
			return false;
		}
		else if (stricmp("ResetRules", args.Arg(2)) == 0)
			// example : ncz ConVarTester ResetRules
		{
			LoadDefaultRules();
			return true;
		}
	}
	else
	{
		return true;
	}
	return false;
}

void ConVarTester::AddConvarRuleset ( const char * name, const char * value, ConVarRuleT rule, bool safe )
{
	if( rule == ConVarRule::SAME_AS_SERVER || rule == ConVarRule::SAME_FLOAT_AS_SERVER )
	{
		void * sv_cvar ( SourceSdk::InterfacesProxy::ICvar_FindVar ( name ) );
		if( sv_cvar )
		{
			m_convars_rules.AddToTail ( ConVarInfo ( name, SourceSdk::InterfacesProxy::ConVar_GetString ( sv_cvar ), rule, safe, sv_cvar ) );
		}
		else
		{
			g_Logger.Msg<MSG_ERROR> ( Helpers::format ( "ConVarTester : Failed to link the server convar %s (Not found server-side)", name ) );
		}
	}
	else
	{
		m_convars_rules.AddToTail ( ConVarInfo ( name, value, rule, safe ) );
	}
}

void ConVarTester::RT_OnQueryCvarValueFinished ( PlayerHandler::iterator ph, SourceSdk::QueryCvarCookie_t cookie, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	CurrentConVarRequest* const req ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );
	if (req->cookie == cookie)
	{
		LoggerAssert(req->status == ConVarRequestStatus::SENT);

		if (SourceSdk::InterfacesProxy::ConVar_GetBool(var_sv_cheats))
		{
			/* Some servers silently set sv_cheats to 1 in a short timespan to make some mods.
			 This is where some players can get kicked without reason.
			 We will send the same request another time. */
			InitDataStruct();

			//g_Logger.Msg<MSG_WARNING> ( Helpers::format ( "ConVarTester : Cannot process RT_OnQueryCvarValueFinished because server-side sv_cheats is not 0", ph->GetName ()) );
			return;
		}

		ConVarInfoT* ruleset(RT_FindConvarRuleset(pCvarName));
		LoggerAssert(ruleset);
		req->status = ConVarRequestStatus::REPLYED;
		req->answer = pCvarValue;

		switch (eStatus)
		{
		case SourceSdk::eQueryCvarValueStatus_ValueIntact:
		{
			req->answer_status = "ValueIntact";

			float fcval((float)atof(pCvarValue));
			float fsval((float)atof(ruleset->value));

			switch (ruleset->rule)
			{
			case ConVarRule::SAME:
			{
				if (strcmp(ruleset->value, pCvarValue))
				{
					goto unexpected2;
				}

				break;
			}

			case ConVarRule::SAME_AS_SERVER:
			{
				if (strcmp(SourceSdk::InterfacesProxy::ConVar_GetString(ruleset->sv_var), pCvarValue))
				{
					goto unexpected2;
				}

				break;
			}

			case ConVarRule::SAME_FLOAT_AS_SERVER:
				fsval = (float)atof(SourceSdk::InterfacesProxy::ConVar_GetString(ruleset->sv_var));

			case ConVarRule::SAME_FLOAT:
				if (fcval != fsval)
				{
					goto unexpected2;
				}

				break;

			case ConVarRule::LOWER:
				if (fcval >= fsval)
				{
					goto unexpected2;
				}

				break;

			case ConVarRule::HIGHER:
				if (fcval <= fsval)
				{
					goto unexpected2;
				}

				break;

			case ConVarRule::NO_VALUE:
			{
				{
					ProcessDetectionAndTakeAction<Detection_IllegalConVar::data_type>(Detection_IllegalConVar(), req, ph, this);

					break;
				}
			}

			default:
				g_Logger.Msg<MSG_ERROR>(Helpers::format("ConVarTester : Unknown code, server memory is crashed.", ph->GetName()));
				break;

			}
			break;
		}

		case SourceSdk::eQueryCvarValueStatus_CvarNotFound:
		{
			req->answer_status = "CvarNotFound";
			req->answer = "NO VALUE";
			if (ruleset->rule != ConVarRule::NO_VALUE)
			{
				ProcessDetectionAndTakeAction<Detection_ConVar::data_type>(Detection_ConVar(), req, ph, this);
			}
			break;
		}

		case SourceSdk::eQueryCvarValueStatus_NotACvar:
		{
			req->answer_status = "NotACvar";
			req->answer = "CONCOMMAND";
			goto unexpected2;
		}

		case SourceSdk::eQueryCvarValueStatus_CvarProtected:
		{
			req->answer_status = "CvarProtected";
			req->answer = "NO VALUE";
			goto unexpected2;
		}

		default:
		{
			g_Logger.Msg<MSG_LOG>("ConVarTester : The following player is banned because of an unknown ConVar Request Answer code.");
			req->answer_status = "NO STATUS";
			req->answer = "NO VALUE - UNKNOWN eQueryCvarValueStatus ";
			goto unexpected2;
		}
		}

		return;
	unexpected2:
		ProcessDetectionAndTakeAction<Detection_ConVar::data_type>(Detection_ConVar(), req, ph, this);
	}
	else
	{
		// Do not detect invalid cookie because it means this request comes from another plugin
	}
}

void ConVarTester::LoadDefaultRules ()
{
	m_convars_rules.RemoveAll ();

	AddConvarRuleset ( "developer", "0", ConVarRule::SAME );
	AddConvarRuleset ( "sv_cheats", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "sv_accelerate", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "sv_showimpacts", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "sv_showlagcompensation", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "host_framerate", "0", ConVarRule::SAME_FLOAT_AS_SERVER );
	AddConvarRuleset ( "host_timescale", "0", ConVarRule::SAME_FLOAT_AS_SERVER );
	AddConvarRuleset ( "r_visualizetraces", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "mat_normalmaps", "0", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "mp_playerid", "0", ConVarRule::SAME_AS_SERVER );
	//AddConvarRuleset ( "mp_forcecamera", "1", ConVarRule::SAME_AS_SERVER );
	AddConvarRuleset ( "net_fakeloss", "0", ConVarRule::SAME );
	AddConvarRuleset ( "net_fakelag", "0", ConVarRule::SAME );
	AddConvarRuleset ( "net_fakejitter", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawothermodels", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_shadowwireframe", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_avglight", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_novis", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawparticles", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawopaqueworld", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawtranslucentworld", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawmodelstatsoverlay", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_skybox", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_aspectratio", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawskybox", "1", ConVarRule::SAME );
	AddConvarRuleset ( "r_showenvcubemap", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawlights", "0", ConVarRule::SAME );
	AddConvarRuleset ( "r_drawrenderboxes", "0", ConVarRule::SAME );
	AddConvarRuleset ( "mat_wireframe", "0", ConVarRule::SAME );
	AddConvarRuleset ( "mat_drawwater", "1", ConVarRule::SAME );
	AddConvarRuleset ( "mat_loadtextures", "1", ConVarRule::SAME );
	AddConvarRuleset ( "mat_showlowresimage", "0", ConVarRule::SAME );
	AddConvarRuleset ( "mat_fillrate", "0", ConVarRule::SAME );
	AddConvarRuleset ( "mat_proxy", "0", ConVarRule::SAME );
	AddConvarRuleset ( "mem_force_flush", "0", ConVarRule::SAME );
	AddConvarRuleset ( "fog_enable", "1", ConVarRule::SAME );
	AddConvarRuleset ( "cl_pitchup", "89", ConVarRule::SAME );
	AddConvarRuleset ( "cl_pitchdown", "89", ConVarRule::SAME );
	AddConvarRuleset ( "net_graph", "3", ConVarRule::LOWER);
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		AddConvarRuleset ( "cl_bobcycle", "0.98", ConVarRule::SAME_FLOAT );
	}
	else
	{
		AddConvarRuleset ( "cl_bobcycle", "0.8", ConVarRule::SAME_FLOAT );
	}
	AddConvarRuleset ( "cl_leveloverviewmarker", "0", ConVarRule::SAME );
	AddConvarRuleset ( "snd_visualize", "0", ConVarRule::SAME );
	AddConvarRuleset ( "snd_show", "0", ConVarRule::SAME );
	AddConvarRuleset ( "openscript", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "0penscript", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "aim_fov", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "aim_bot", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "bat_version", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "fm_attackmode", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "openscript_version", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "lua-engine", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "lua_open", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "runnscript", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "runscript", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "smadmintakeover", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "tb_enabled", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "zb_enabled", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "ms_sv_cheats", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "ms_r_drawothermodels", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "ms_chat", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "ms_aimbot", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "wallhack", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "cheat_chat", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "cheat_chams", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "cheat_dlight", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "byp_svc", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_speed_hts", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_speed_hfr", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_render_rdom", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_render_mwf", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_render_rdp", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_fake_lag", "", ConVarRule::NO_VALUE );
	//AddConvarRuleset ( "byp_fake_loss", "", ConVarRule::NO_VALUE );
	AddConvarRuleset ( "k0ntr011_aim", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "esp", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "aimbot", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "prepareuranus", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "zhyklua", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "zapuskscript", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "ru_show_team", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "show_team", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "servishack_steam_set_id", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "starts", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "cs_show_team", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "openhack", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "aaa123_steam_set_random_id", "", ConVarRule::NO_VALUE);
	AddConvarRuleset ( "steam_set_id", "", ConVarRule::NO_VALUE);
}

void ConVarTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	var_sv_cheats = SourceSdk::InterfacesProxy::ICvar_FindVar ( "sv_cheats" );

	m_current_player = PlayerHandler::end ();

	OnTickListener::RegisterOnTickListener ( this );
}

void ConVarTester::Unload ()
{
	OnTickListener::RemoveOnTickListener ( this );
	//m_convars_rules.RemoveAll ();
}

ConVarTester g_ConVarTester;

const char* ConvertRule ( ConVarRule rule )
{
	switch( rule )
	{
		case  ConVarRule::SAME:
			return "SAME";
		case  ConVarRule::SAME_FLOAT:
			return "SAME_FLOAT";
		case  ConVarRule::SAME_AS_SERVER:
			return "SAME_AS_SERVER";
		case  ConVarRule::SAME_FLOAT_AS_SERVER:
			return "SAME_FLOAT_AS_SERVER";
		case  ConVarRule::NO_VALUE:
			return "NO_VALUE";
		default:
			return "SAME";
	};
}

const char* ConvertRequestStatus ( ConVarRequestStatus status )
{
	switch( status )
	{
		case  ConVarRequestStatus::NOT_PROCESSING:
			return "NOT_PROCESSING";
		case  ConVarRequestStatus::SENT:
			return "SENT";
		case  ConVarRequestStatus::REPLYED:
			return "REPLYED";
		default:
			return "UNKNOWN";
	};
}

basic_string Detection_ConVar::GetDataDump ()
{
	return Helpers::format(
		":::: CurrentConVarRequest {\n"
		":::::::: Request Status : %s,\n"
		":::::::: Request Sent At : %f,\n"
		":::::::: Attempts : %d,\n"
		":::::::: ConVarInfo {\n"
		":::::::::::: ConVar Name : %s,\n"
		":::::::::::: Expected Value : %s,\n"
		":::::::::::: Got Value : %s,\n"
		":::::::::::: Answer Status : %s,\n"
		":::::::::::: Comparison Rule : %s\n"
		"::::::::}\n"
		"::::}",
		ConvertRequestStatus(GetDataStruct()->status),
		GetDataStruct()->timeStart,
		GetDataStruct()->attempts+1,
		g_ConVarTester.m_convars_rules[GetDataStruct()->ruleset].name,
		g_ConVarTester.m_convars_rules[GetDataStruct()->ruleset].value,
		GetDataStruct()->answer.c_str(),
		GetDataStruct()->answer_status.c_str(),
		ConvertRule(g_ConVarTester.m_convars_rules[GetDataStruct()->ruleset].rule));
}

basic_string Detection_ConVar::GetDetectionLogMessage ()
{
	return Helpers::format ( "%s ConVar Bypasser", g_ConVarTester.m_convars_rules[ GetDataStruct ()->ruleset ].name );
}

basic_string Detection_ConVarRequestTimedOut::GetDetectionLogMessage()
{
	return Helpers::format("%s ConVar request time out", g_ConVarTester.m_convars_rules[GetDataStruct()->ruleset].name);
}

basic_string Detection_IllegalConVar::GetDetectionLogMessage()
{
	return Helpers::format("%s illegal ConVar", g_ConVarTester.m_convars_rules[GetDataStruct()->ruleset].name);
}
