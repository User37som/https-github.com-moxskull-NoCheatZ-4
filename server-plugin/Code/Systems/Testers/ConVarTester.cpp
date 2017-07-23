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

	/*
	Sig g_iQueryCvarCookie
		"StartQueryCvarValue: not a client"

		CSGO
			dll :
			VEngineServer::StartQueryCvarValue

			55
			8B EC
			83 E4 F8
			83 EC 3C
			56
			8B 75 08
			28 35 ?D4 ?12 ?7B ?10
			C1 FE 04
			83 FE 01
			7C 08
			8B 35 ?98 ?11 ?7B ?10
			68 ?B0 ?39 ?4A ?10
			E8 ?63 ?0B ?06 ?00
			83 C4 04
			A1 ?8C ?11 ?7B ?10
			8D 4C 24 08
			8B 74 B0 FC
			8D 46 04
			F7 DE
			1B F6
			23 F0
			E8 15 4F EF FF
			A1 TT TT TT TT
			8B C8

			so :
			Engine.SendCvarValueQueryToClient

			55
			89 E5
			57
			56
			53
			83 EC 5C
			C7 45 C0 ?68 ?8A ?54 ?00
			0F B6 45 10
			89 34 24
			8B 7D 0C
			88 45 B7
			E8 ?0B ?4B ?F6 ?FF
			8B 15 TT TT TT TT
			C6 45 DC 01
		CSS:
			dll:
			Engine.SendCvarValueQueryToClient

			55
			8B EC
			81 EC 18 01 00 00
			A1 TT TT TT TT
			8B 8C
			40
			C6 85 EC FE FF FF 01
			80 7D 10 00
			?A3 ?7C ?2A ?3C ?10
			8B 45 0C
			C7 ?85 ?F0 ?FE ?FF ?FF ?00 ?00 ?00 ?00
			89 8D F8 FE FF FF
			89 85 FC FE FF FF

			so:
			Engine.SendCvarValueQueryToClient

			55
			89 E5
			81 EC 38 01 00
			A1 TT TT TT TT
			C6 85 DC FE FF FF 01
			C7 E5 E0 FE FF FF 00 00 00 00
			80 7D 10 00
			C7 85 FE FF FF ?68 ?2E ?23 ?00
			8B 55 08
			8D 48 01
			89 85 E8 FE FF FF
			89 0D TT TT TT TT
	*/

	if (m_engine_cvar_cookie) return;

	DebugMessage("Trying to get g_iQueryCvarCookie ...");

	basic_string modulename("engine");

#ifdef GNUC
	if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
	{
		modulename.append("_srv");
	}
#endif

#ifdef WIN32
	modulename.append(".dll");

	HMODULE engine_module_handle(GetModuleHandleA(modulename.c_str()));
	if (engine_module_handle != NULL)
	{
		DebugMessage("Trying to hook game engine ...");

		mem_byte *sig_code;

		if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
		{
			mem_byte sig_code[] =
			{
				0x55,
				0x8B, 0xEC,
				0x83, 0xE4, 0xF8,
				0x83, 0xEC, 0x3C,
				0x56,
				0x8B, 0x75, 0x08,
				0x28, 0x35, 0xD4, 0x12, 0x7B, 0x10,
				0xC1, 0xFE, 0x04,
				0x83, 0xFE, 0x01,
				0x7C, 0x08,
				0x8B, 0x35, 0x98, 0x11, 0x7B, 0x10,
				0x68, 0xB0, 0x39, 0x4A, 0x10,
				0xE8, 0x63, 0x0B, 0x06, 0x00,
				0x83, 0xC4, 0x04,
				0xA1,0x8C,0x11, 0x7B, 0x10,
				0x8D, 0x4C, 0x24, 0x08,
				0x8B, 0x74, 0xB0, 0xFC,
				0x8D, 0x46, 0x04,
				0xF7, 0xDE,
				0x1B, 0xF6,
				0x23, 0xF0,
				0xE8, 0x15, 0x4F, 0xEF, 0xFF,
				0xA1, 0xTT, 0xTT, 0xTT, 0xTT,
				0x8B, 0xC8
			};
		}
		else
		{

		}

		sig_ctx ctx(metafactory_sig_code, metafactory_sig_mask, 45, 0x67);

#else
	modulename.append(".so");

	basic_string relpath(Helpers::format("./bin/%s", modulename.c_str()));

	void ** modinfo = (void **)dlopen(relpath.c_str(), RTLD_NOW | RTLD_NOLOAD);
	void * mm_module_handle = nullptr;
	if (modinfo != NULL)
	{
		DebugMessage("Trying to hook engine ...");

		//mm_module_handle = dlsym(modinfo, ".init_proc");
		// FIXME : Use link_map to get memory bounds of the module
		engine_module_handle = *modinfo;
		dlclose(modinfo);
	}
	if (engine_module_handle)
	{
		mem_byte const metafactory_sig_code[48] = {
			0x55, 0x53, 0x57, 0x56, 0x83, 0xEC, 0x2C, 0x8B,
			0x44, 0x24, 0x4C, 0x8B, 0x5C, 0x24, 0x44, 0x85,
			0xC0, 0x74, 0x06, 0xC7, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x31, 0xC0, 0x85, 0xDB, 0x0F, 0x84, 0x3D,
			0x01, 0x00, 0x00, 0x8B, 0x74, 0x24, 0x48, 0x89,
			0x1C, 0x24, 0xC7, 0x44, 0x24, 0x04, 0x3A, 0xFC
		};

		mem_byte const metafactory_sig_mask[48] = {
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
			0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
		};

		sig_ctx ctx(metafactory_sig_code, metafactory_sig_mask, 45, 0x89);

#endif

		ScanMemoryRegion(reinterpret_cast<mem_byte *>(mm_module_handle), reinterpret_cast<mem_byte *>(mm_module_handle) + 0x30000, &ctx);

		if (ctx.m_out != nullptr)
		{
			g_SourceHook = reinterpret_cast<ISourceHook_Skeleton *>(*(reinterpret_cast<size_t**>(ctx.m_out)));

			if (g_SourceHook != nullptr)
			{
				DebugMessage(Helpers::format("g_SourceHook is at 0x%X (Interface Version %d, Impl Version %d)", g_SourceHook, g_SourceHook->GetIfaceVersion(), g_SourceHook->GetImplVersion()));
				HookInfo haddhook(g_SourceHook, 2, (DWORD)my_AddHook);
				HookInfo hremovehook(g_SourceHook, 3, (DWORD)my_RemoveHook);

				local_HookGuardSourceHookSafety.VirtualTableHook(haddhook, "ISourceHook::AddHook");
				local_HookGuardSourceHookSafety.VirtualTableHook(hremovehook, "ISourceHook::RemoveHook");
			}
			else
			{
				g_Logger.Msg<MSG_ERROR>("Failed to get ISourceHook interface.");
			}
		}
		else
		{
			g_Logger.Msg<MSG_ERROR>("Sigscan failed for MetaFactory.");
		}
	}
	else
	{
		DebugMessage("metamod module not found.");
	}
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

void ConVarTester::RT_ProcessOnTick ( float const & curtime )
{
	if( m_convars_rules.IsEmpty () ) return;
	if( !IsActive () ) return;

	if( SourceSdk::InterfacesProxy::ConVar_GetBool ( var_sv_cheats ) )
	{
		return;
	}
	ProcessFilter::HumanAtLeastConnected filter_class;
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
	/*m_current_player = PlayerHandler::end ();
	m_current_player += &filter_class;
	for( ; m_current_player != PlayerHandler::end (); m_current_player += &filter_class )
	{
		RT_ProcessPlayerTest ( m_current_player, curtime );
	}*/

	m_current_player += &filter_class;
	if(m_current_player)
		RT_ProcessPlayerTest(m_current_player, curtime);
}

void ConVarTester::RT_ProcessPlayerTest ( PlayerHandler::iterator ph, float const & curtime )
{
	CurrentConVarRequestT* const req ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	switch( req->status )
	{
		case ConVarRequestStatus::SENT: // Not yet replyed, check for timeout
			{
				if( curtime >= req->timeEnd )
				{
					if( req->attempts >= 2 )
					{
						req->answer = "NO ANSWER - TIMED OUT";
						Detection_ConVarRequestTimedOut pDetection;
						pDetection.PrepareDetectionData ( req );
						pDetection.PrepareDetectionLog ( *ph, this );
						pDetection.Log ();
						ph->Kick ( "ConVar request timed out" );
					}
					g_Logger.Msg<MSG_WARNING> ( Helpers::format ( "ConVarTester : ConVar request timed out for %s (%s) : %d attempts.", ph->GetName (), m_convars_rules[ req->ruleset ].name, req->attempts+1) );
					++req->attempts;
					req->SendCurrentRequest ( ph, curtime, m_convars_rules ); // Send the request again
				}
				// else wait ...
				break;
			}

		default: // Continue to work ...
			{
				req->PrepareNextRequest ( m_convars_rules );
				req->SendCurrentRequest ( ph, curtime, m_convars_rules );
				break;
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
			req->status = ConVarRequestStatus::NOT_PROCESSING;

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
