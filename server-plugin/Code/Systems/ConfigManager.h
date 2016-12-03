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

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Containers/utlvector.h"

#include "Misc/temp_basicstring.h"
#include "Misc/temp_singleton.h"

class ConfigManager :
	public Singleton<ConfigManager>
{
	typedef Singleton<ConfigManager> singleton_class;

private:

	int const content_version; // Backward compatible if we ever change the layout of the config file

public:
	basic_string m_playerdataclass;
	// absolute path to server as : /var/steamcmd/CSGO/csgo/   meaning if you the cfg dir it is : /var/steamcmd/CSGO/csgo/cfg/
	basic_string m_root_server_path;
	// game name like cstrike or csgo
	basic_string m_game_name;

	// values
	float m_smoke_radius;
	float m_innersmoke_radius_sqr;
	float m_smoke_timetobang;
	float m_smoke_time;

	// virtual functions
	int vfid_getdatadescmap;
	int vfid_settransmit;
	int vfid_mhgroundentity;
	int vfid_weaponequip;
	int vfid_weapondrop;
	int vfid_playerruncommand;
	int vfid_dispatch;
	int vfid_thinkpost;

public:
	ConfigManager ();
	virtual ~ConfigManager () override final;

	/*
		Load the config file containing configuration. Returns false if error parsing the config file.
	*/
	bool LoadConfig ();
};

#endif // CONFIGMANAGER_H
