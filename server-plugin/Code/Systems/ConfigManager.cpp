#include "ConfigManager.h"

#include <fstream>

#include "Interfaces/InterfacesProxy.h"

#include "BaseSystem.h"
#include "Systems/Logger.h"

#define FILE_LINE_BUFFER 1024

#ifdef WIN32
#	define ATTRIB_POST "_windows"
#else
#	define ATTRIB_POST "_linux"
#endif

bool GetIniAttributeValue ( std::ifstream & file, basic_string const & root, basic_string const & attrib, basic_string & value, bool first_chance = true )
{
	LoggerAssert ( file.is_open () );

	file.clear ( std::ios::goodbit );
	file.seekg ( 0, std::ios::beg );
	if( file )
	{
		// find root
		basic_string root_expect ( "[" );
		root_expect.append ( root ).append ( "]" ).lower ();

		basic_string buf_ref;
		char buf[ FILE_LINE_BUFFER ];
		bool root_found ( false );

		do
		{
			file.getline ( buf, FILE_LINE_BUFFER );
			buf_ref = buf;
			buf_ref.lower ();

			if( buf_ref == root_expect )
			{
				root_found = true;
				break;
			}

		}
		while( !file.eof () );

		if( root_found )
		{
			// find attribute
			basic_string attrib_expect ( attrib );
			attrib_expect.append ( "=" );
			attrib_expect.lower ();

			do
			{
				file.getline ( buf, FILE_LINE_BUFFER );
				buf_ref = buf;
				buf_ref.lower ();
				
				if(buf_ref[0] == '[')
					break;

				if( buf_ref.find ( attrib_expect ) == 0 )
				{
					value = ( buf_ref.c_str () + attrib_expect.size () );
					return true;
				}

			}
			while( !file.eof () );
		}
		else
		{
			g_Logger.Msg<MSG_ERROR> ( Helpers::format ( "ConfigManager::GetIniAttributeValue : Cannot find root %s", root.c_str ()) );
		}
	}
	else
	{
		g_Logger.Msg<MSG_ERROR> (  "ConfigManager::GetIniAttributeValue : Cannot read ini file" );
	}
	value = "";
	if(!first_chance) g_Logger.Msg<MSG_ERROR> ( Helpers::format("ConfigManager::GetIniAttributeValue : Cannot find attribute %s -> %s", root.c_str(), attrib.c_str() ));
	return false;
}

float ConfigManager::tickinterval(0.0f);

ConfigManager::ConfigManager () :
	Singleton (),
	content_version ( 2 ),
	m_playerdataclass ( "" ),
	m_smoke_radius ( 0.0f ),
	m_innersmoke_radius_sqr ( 0.0f ),
	m_smoke_timetobang ( 0.0f ),
	m_smoke_time ( 0.0f ),
	tickrate_override(0.0f),
	vfid_getdatadescmap ( 0 ),
	vfid_settransmit ( 0 ),
	vfid_mhgroundentity ( 0 ),
	vfid_weaponequip ( 0 ),
	vfid_weapondrop ( 0 ),
	vfid_playerruncommand ( 0 ),
	vfid_dispatch ( 0 ),
	vfid_thinkpost ( 0 )
{

}

ConfigManager::~ConfigManager ()
{

}

bool ConfigManager::LoadConfig ()
{
	basic_string path;
	SourceSdk::InterfacesProxy::GetGameDir ( path );
	basic_string gamename ( path );
	path.append ( "/addons/NoCheatZ/config.ini" );
	std::ifstream file ( path.c_str (), std::ios::in );
	if( file )
	{
		basic_string value;
		if( !GetIniAttributeValue ( file, "CONFIG", "config_version", value, false ) ) return false;
		if( atoi ( value.c_str () ) == content_version )
		{
			// load virtual functions id
			if (!GetIniAttributeValue(file, "CONFIG", "getdatadescmap" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "getdatadescmap" ATTRIB_POST, value, false ) ) return false;
			vfid_getdatadescmap = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "settransmit" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "settransmit" ATTRIB_POST, value, false ) ) return false;
			vfid_settransmit = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "mhgroundentity" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "mhgroundentity" ATTRIB_POST, value, false ) ) return false;
			vfid_mhgroundentity = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "weaponequip" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "weaponequip" ATTRIB_POST, value, false ) ) return false;
			vfid_weaponequip = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "weapondrop" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "weapondrop" ATTRIB_POST, value, false ) ) return false;
			vfid_weapondrop = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "playerruncommand" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "playerruncommand" ATTRIB_POST, value, false ) ) return false;
			vfid_playerruncommand = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "dispatch" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "dispatch" ATTRIB_POST, value, false ) ) return false;
			vfid_dispatch = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "thinkpost" ATTRIB_POST, value))
				if( !GetIniAttributeValue ( file, gamename, "thinkpost" ATTRIB_POST, value, false ) ) return false;
			vfid_thinkpost = atoi ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "tickrate_override", value))
				if (!GetIniAttributeValue(file, gamename, "tickrate_override", value, false)) return false;
			tickrate_override = (float)atof(value.c_str());
			if (tickrate_override == 0.0f) return false;
			tickinterval = 1.0f / tickrate_override;

			// load some strings

			if (!GetIniAttributeValue(file, "CONFIG", "playerdataclass", m_playerdataclass))
				if( !GetIniAttributeValue ( file, gamename, "playerdataclass", m_playerdataclass, false ) ) return false;

			// load some values

			if (!GetIniAttributeValue(file, "CONFIG", "f_smoketime", value))
				if( !GetIniAttributeValue ( file, gamename, "f_smoketime", value, false ) ) return false;
			m_smoke_time = ( float ) atof ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "f_smoke_time_to_bang", value))
				if( !GetIniAttributeValue ( file, gamename, "f_smoke_time_to_bang", value, false ) ) return false;
			m_smoke_timetobang = ( float ) atof ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "f_inner_smoke_radius_sqr", value))
				if( !GetIniAttributeValue ( file, gamename, "f_inner_smoke_radius_sqr", value, false ) ) return false;
			m_innersmoke_radius_sqr = ( float ) atof ( value.c_str () );

			if (!GetIniAttributeValue(file, "CONFIG", "f_smoke_radius", value))
				if( !GetIniAttributeValue ( file, gamename, "f_smoke_radius", value, false ) ) return false;
			m_smoke_radius = ( float ) atof ( value.c_str () );

			// disable systems

			if (GetIniAttributeValue(file, "CONFIG", "disable_systems", value))
			{
				CUtlVector<basic_string> systems;
				SplitString<char>(value, ';', systems);

				for (CUtlVector<basic_string>::iterator it(systems.begin()); it != systems.end(); ++it)
				{
					if (!it->isempty())
					{
						BaseSystem* system(BaseSystem::FindSystemByName(it->c_str()));
						if (system)
						{
							system->SetDisabledByConfigIni();
							g_Logger.Msg<MSG_CONSOLE>(Helpers::format("ConfigManager : Disabled system %s", it->c_str()));
						}
						else
						{
							g_Logger.Msg<MSG_ERROR>(Helpers::format("ConfigManager : Unable to disable system %s -> system not known", it->c_str()));
						}
					}
				}
			}

			if (GetIniAttributeValue(file, gamename, "disable_systems", value))
			{
				CUtlVector<basic_string> systems;
				SplitString<char>(value, ';', systems);

				for (CUtlVector<basic_string>::iterator it(systems.begin()); it != systems.end(); ++it)
				{
					if (!it->isempty())
					{
						BaseSystem* system(BaseSystem::FindSystemByName(it->c_str()));
						if (system)
						{
							system->SetDisabledByConfigIni();
							g_Logger.Msg<MSG_CONSOLE>(Helpers::format("ConfigManager : Disabled system %s", it->c_str()));
						}
						else
						{
							g_Logger.Msg<MSG_ERROR>(Helpers::format("ConfigManager : Unable to disable system %s -> system not known", it->c_str()));
						}
					}
				}
			}

			if (GetIniAttributeValue(file, "CONFIG", "admin_ids", value))
			{
				value.lower();
				SplitString<char>(value, ';', m_admins);
			}

			return true;

		}
		else
		{
			g_Logger.Msg<MSG_ERROR> (  "ConfigManager::LoadConfig : content_version mismatch"  );
		}
	}
	else
	{
		g_Logger.Msg<MSG_ERROR> ( "ConfigManager::LoadConfig : Cannot open config.ini"  );
	}

	return false;
}

bool ConfigManager::IsAdmin(PlayerHandler::iterator ph)
{
	if (ph >= SlotStatus::PLAYER_CONNECTING)
	{
		return IsAdmin(ph->GetSteamID());
	}
	return false;
}

bool ConfigManager::IsAdmin(char const * steamid)
{
	basic_string lower_id(steamid);
	lower_id.lower();
	if (m_admins.Find(lower_id) != -1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

ConfigManager g_ConfigManager;
