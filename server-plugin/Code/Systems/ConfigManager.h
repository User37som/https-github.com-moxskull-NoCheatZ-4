#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Containers/utlvector.h"

#include "Misc/temp_basicstring.h"
#include "Misc/temp_singleton.h"
#include "Players/NczPlayerManager.h"

class ConfigManager :
	public Singleton
{
private:

	int const content_version; // Backward compatible if we ever change the layout of the config file

	CUtlVector<basic_string> m_admins;

public:
	basic_string m_playerdataclass;

	// values
	float m_smoke_radius;
	float m_innersmoke_radius_sqr;
	float m_smoke_timetobang;
	float m_smoke_time;
	static float tickinterval;
	float tickrate_override;

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
	virtual ~ConfigManager () ;

	/*
		Load the config file containing configuration. Returns false if error parsing the config file.
	*/
	bool LoadConfig ();

	bool IsAdmin(PlayerHandler::iterator ph);

	bool IsAdmin(char const * steamid);
};

extern ConfigManager g_ConfigManager;

#endif // CONFIGMANAGER_H
