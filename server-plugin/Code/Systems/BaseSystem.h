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

#ifndef BASESYSTEM
#define BASESYSTEM

#include <stdint.h>

#include "Console/convar.h"

#include "Misc/temp_basiclist.h"
#include "Misc/temp_basicstring.h"


/////////////////////////////////////////////////////////////////////////
// BaseSystem
/////////////////////////////////////////////////////////////////////////

class BaseSystem :
	public ListMe<BaseSystem>
{
	typedef ListMe<BaseSystem> ListMeClass;

private:
	char const * const m_name;
	char const * const m_cmd_list;

protected:
	int m_verbose;

#ifdef NCZ_USE_METRICS
	MetricsContainer<> m_metrics;
#endif

protected:
	BaseSystem ( char const * const name, char const * const commands = "Verbose" );
	virtual ~BaseSystem ();

public:
	virtual char const * cmd_list () const final
	{
		return m_cmd_list;
	};

	/* Donne le nom du système pour pouvoir être identifié dans la console */
	virtual char const * GetName () const final
	{
		return m_name;
	};

	/* Calls ShouldUnload for each systems and unloads them if true */

	/* Process Load when m_isActive changes to true
	"    Unload when m_isActive changes to false */
	virtual void SetActive ( bool active ) = 0;

	/* Must be used before all processing operations */
	virtual bool IsActive () const = 0;

	/* Retourne vrai si le fichier de configuration dit d'activer ce système */
	virtual bool IsEnabledByConfig () const = 0;

	virtual void SetConfig ( bool enabled ) = 0;

	virtual void SetDisabledByConfigIni () = 0;

	virtual bool GetDisabledByConfigIni () const = 0;

protected:
	virtual void Init () = 0;

	/* Commande(s) console du système,
	donne vrai si la commande existe
	Le premier argument est en fait le troisième */
	virtual bool sys_cmd_fn(const SourceSdk::CCommand &args)
	{
		return false;
	}

	/* Returns true if the system got any job to do and must be active (A player is in the filter or any other thing) */
	virtual bool GotJob () const = 0;

	virtual bool IsStatic () const = 0;

	virtual bool IsDynamic () const = 0;

	virtual bool IsTester() const = 0;

	virtual bool IsBlocker() const = 0;

protected:
	virtual void Load () = 0;
	virtual void Unload () = 0; // Defined by child, unregister from callbacks

public:
	static void ManageSystems ()
	{
		TryUnloadSystems (); TryLoadSystems ();
	};

	/* Permet de détruire tous les systèmes créés jusqu'à présent */
	static void UnloadAllSystems ();

#ifdef NCZ_USE_METRICS
	MetricsContainer<> & GetMetrics ()
	{
		return m_metrics;
	};
#endif

	static void InitSystems ();

	static BaseSystem* FindSystemByName ( const char * name );

	/* Commande console de base "ncz" */
	static void ncz_cmd_fn ( const SourceSdk::CCommand &args );

private:
	static void TryUnloadSystems ();

	static void TryLoadSystems ();
};

class BaseStaticSystem :
	public BaseSystem
{
protected:
	BaseStaticSystem ( char const * const name, char const * const commands = "Verbose" );

	virtual ~BaseStaticSystem ();

public:
	virtual void SetActive ( bool active ) override final
	{}

	virtual bool IsActive () const override final
	{
		return true;
	}

	virtual bool IsEnabledByConfig () const override final
	{
		return true;
	}

	virtual void SetConfig ( bool enabled ) override final
	{}

	virtual void SetDisabledByConfigIni () override final
	{}

	virtual bool GetDisabledByConfigIni () const override final
	{
		return false;
	}

protected:

	virtual bool GotJob () const override
	{
		return true;
	}

	virtual bool IsStatic () const override final
	{
		return true;
	}

	virtual bool IsDynamic () const override final
	{
		return false;
	}

	virtual bool IsTester() const override final
	{
		return false;
	}

	virtual bool IsBlocker() const override final
	{
		return false;
	}

private:
	virtual void Load () override final
	{}
	virtual void Unload () override final
	{}
};

class BaseDynamicSystem :
	public BaseSystem
{
private:
	bool m_isActive;
	bool m_isDisabled;
	bool m_configState;

protected:
	BaseDynamicSystem ( char const * const name, char const * const commands = "Enable - Disable - Verbose" );

	virtual ~BaseDynamicSystem ();

public:
	virtual void SetActive ( bool active ) override final;

	virtual bool IsActive () const override final
	{
		return m_isActive;
	}

	virtual bool IsEnabledByConfig () const override final
	{
		return m_configState;
	}

	virtual void SetConfig ( bool enabled ) override final
	{
		m_configState = enabled;
	}

	virtual void SetDisabledByConfigIni () override final
	{
		m_isDisabled = true;
	}

	virtual bool GetDisabledByConfigIni () const override final
	{
		return m_isDisabled;
	}

protected:

	virtual bool GotJob () const override
	{
		return true;
	}

	virtual bool IsStatic () const override final
	{
		return false;
	}

	virtual bool IsDynamic () const override final
	{
		return true;
	}

	virtual bool IsTester() const override
	{
		return false;
	}

	virtual bool IsBlocker() const override
	{
		return false;
	}
};

class BaseTesterSystem :
	public BaseDynamicSystem
{
public:
	typedef enum DetectionAction : size_t
	{
		BAN_ASYNC = 0,
		BAN_NOW,
		KICK,
		LOG
	} DetectionAction_t;

private:
	DetectionAction_t m_action_on_detection;

protected:
	BaseTesterSystem(char const * const name, char const * const commands = "Enable - Disable - Verbose - SetAction");

	virtual ~BaseTesterSystem();

public:
	void SetAction(DetectionAction_t action)
	{
		m_action_on_detection = action;
	}

	bool SetAction(basic_string const & in)
	{
		basic_string lower_in(in);
		lower_in.lower();

		if (lower_in == "ban_async")
		{
			SetAction(DetectionAction_t::BAN_ASYNC);
			return true;
		}
		else if (lower_in == "ban_now")
		{
			SetAction(DetectionAction_t::BAN_NOW);
			return true;
		}
		else if (lower_in == "kick")
		{
			SetAction(DetectionAction_t::KICK);
			return true;
		}
		else if (lower_in == "log")
		{
			SetAction(DetectionAction_t::LOG);
			return true;
		}
		else
		{
			return false;
		}
	}

	DetectionAction_t GetAction() const
	{
		return m_action_on_detection;
	}

	void GetAction(basic_string & out) const
	{
		switch (m_action_on_detection)
		{
		case DetectionAction_t::BAN_ASYNC:
			out = "BAN_ASYNC";
			break;
		case DetectionAction_t::BAN_NOW:
			out = "BAN_NOW";
			break;
		case DetectionAction_t::KICK:
			out = "KICK";
			break;
		case DetectionAction_t::LOG:
			out = "LOG";
			break;
		default:
			out = "ERROR VALUE";
			break;
		}
	}

	virtual bool sys_cmd_fn(const SourceSdk::CCommand &args);

	virtual bool IsTester() const override final
	{
		return true;
	}
};

class BaseBlockerSystem :
	public BaseDynamicSystem
{
protected:
	BaseBlockerSystem(char const * const name, char const * const commands = "Enable - Disable - Verbose");

	virtual ~BaseBlockerSystem();

public:
	virtual bool IsBlocker() const override final
	{
		return true;
	}
};

#endif
