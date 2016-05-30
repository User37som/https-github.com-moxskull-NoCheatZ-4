#include "ConfigManager.h"

#include <fstream>

#include "Interfaces/InterfacesProxy.h"

#include "BaseSystem.h"

#define FILE_LINE_BUFFER 64

#ifdef WIN32
#	define ATTRIB_POST "_windows"
#else
#	define ATTRIB_POST "_linux"
#endif

bool GetIniAttributeValue(std::ifstream & file, basic_string const & root, basic_string const & attrib, basic_string & value)
{
	Assert(file.is_open());

	file.clear(std::ios::goodbit);
	file.seekg(0, std::ios::beg);
	if (file)
	{
		// find root
		basic_string root_expect("[");
		root_expect.append(root).append("]").lower();

		basic_string buf_ref;
		char buf[FILE_LINE_BUFFER];
		bool root_found = false;

		do
		{
			file.getline(buf, FILE_LINE_BUFFER);
			buf_ref = buf;
			buf_ref.lower();

			if (buf_ref == root_expect)
			{
				root_found = true;
				break;
			}

		} while (!file.eof());

		if (root_found)
		{
			// find attribute
			basic_string attrib_expect(attrib);
			attrib_expect.append("=");
			attrib_expect.lower();

			do
			{
				file.getline(buf, FILE_LINE_BUFFER);
				buf_ref = buf;
				buf_ref.lower();

				if (buf_ref.find(attrib_expect) == 0)
				{
					value = (buf_ref.c_str() + attrib_expect.size());
					return true;
				}

			} while (!file.eof());
		}
		else
		{
			Assert(0 && "root not found in config.ini file");
		}
	}
	value = "";
	Assert(0 && "Attribute not found in config.ini file");
	return false;
}

ConfigManager::ConfigManager() :
	content_version(1),
	singleton_class()
{

}

ConfigManager::~ConfigManager()
{

}

bool ConfigManager::LoadConfig()
{
	basic_string path;
	basic_string gamename;
	SourceSdk::GetGameDir(path);
	gamename = path;
	path.append("/addons/NoCheatZ/config.ini");
	std::ifstream file(path.c_str(), std::ios::in);
	if (file)
	{
		basic_string value;
		if (!GetIniAttributeValue(file, "CONFIG", "config_version", value)) return false;
		if (atoi(value.c_str()) == content_version)
		{
			m_vfuncs.RemoveAll();

			// load virtual functions id
			if(!GetIniAttributeValue(file, gamename, "getdatadescmap" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("getdatadescmap", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "settransmit" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("settransmit", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "mhgroundentity" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("mhgroundentity", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "weaponequip" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("weaponequip", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "weapondrop" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("weapondrop", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "playerruncommand" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("playerruncommand", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "dispatch" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("dispatch", atoi(value.c_str())));

			if (!GetIniAttributeValue(file, gamename, "thinkpost" ATTRIB_POST, value)) return false;
			m_vfuncs.AddToTail(virtual_function("thinkpost", atoi(value.c_str())));

			// load some strings

			if (!GetIniAttributeValue(file, gamename, "playerdataclass", m_playerdataclass)) return false;

			// load some values

			if (!GetIniAttributeValue(file, gamename, "f_smoketime", value)) return false;
			m_smoke_time = (float)atof(value.c_str());

			if (!GetIniAttributeValue(file, gamename, "f_smoke_time_to_bang", value)) return false;
			m_smoke_timetobang = (float)atof(value.c_str());

			if (!GetIniAttributeValue(file, gamename, "f_inner_smoke_radius_sqr", value)) return false;
			m_innersmoke_radius_sqr = (float)atof(value.c_str());

			if (!GetIniAttributeValue(file, gamename, "f_smoke_radius", value)) return false;
			m_smoke_radius = (float)atof(value.c_str());

			// disable systems

			if (!GetIniAttributeValue(file, gamename, "disable_systems", value)) return false;

			CUtlVector<basic_string> systems;
			SplitString<char>(value, ';', systems);

			for (CUtlVector<basic_string>::iterator it = systems.begin(); it != systems.end(); ++it)
			{
				BaseSystem* system = BaseSystem::FindSystemByName(it->c_str());
				if (system)
				{
					system->SetDisabledByConfigIni();
				}
			}

			return true;

		}
		else
		{
			Assert("config.ini content version doesn't match what we expect. Update it." && 0);
		}
	}

	return false;
}

int ConfigManager::GetVirtualFunctionId(basic_string const & name)
{
	int pos = m_vfuncs.Find(name);
	if (pos > -1)
	{
		return m_vfuncs[pos].m_vfid;
	}
	Assert(0 && "vfid not found");
	return 0;
}
