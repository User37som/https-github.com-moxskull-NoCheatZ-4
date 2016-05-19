#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Containers/utlvector.h"

#include "Misc/temp_basicstring.h"

struct virtual_function
{
	basic_string m_name;
	int m_vfid;

	virtual_function(basic_string const & name, int vfid)
	{
		m_name = name;
		m_vfid = vfid;
	}
	virtual_function(const virtual_function & other)
	{
		m_name = other.m_name;
		m_vfid = other.m_vfid;
	}

	virtual_function(basic_string const & name)
	{
		m_name = name;
	}
	bool operator==(virtual_function const & other) const
	{
		return m_name == other.m_name;
	}
};

typedef CUtlVector<virtual_function> virtual_functions_t;

class ConfigManager
{
private:
	virtual_functions_t m_vfuncs;
	
	int const content_version; // Backward compatible if we ever change the layout of the config file

public:
	ConfigManager();
	~ConfigManager();

	basic_string m_playerdataclass;

	/*
		Load the config file containing configuration. Returns false if error parsing the config file.
	*/
	bool LoadConfig();

	/*
		Used for virtual table hooking
	*/
	int GetVirtualFunctionId(basic_string const & name);
};

extern ConfigManager g_ConfigManager;

#endif // CONFIGMANAGER_H
