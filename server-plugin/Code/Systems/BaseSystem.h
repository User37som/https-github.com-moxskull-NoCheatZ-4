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

#include "Misc/temp_basiclist.h"

#include "Console/convar.h"

#include "Players/NczPlayer.h"
#include "Misc/temp_Metrics.h"

/////////////////////////////////////////////////////////////////////////
// BaseSystem
/////////////////////////////////////////////////////////////////////////

class BaseSystem : 
	public ListMe<BaseSystem>,
	public SlotFilter
{
	typedef ListMe<BaseSystem> ListMeClass;

private:
	char const * const m_name;
	char const * const m_cmd_list;
	bool m_isActive;
	bool m_isDisabled;
	bool m_configState;

protected:
	int m_verbose;

#ifdef NCZ_USE_METRICS
	MetricsContainer<> m_metrics;
#endif

protected:
	BaseSystem(char const * const name, SlotStatus filter = PLAYER_IN_TESTS, SlotStatus load_filter = PLAYER_CONNECTED, SlotFilterBehavior filter_behavior = STATUS_STRICT, char const * const commands = "Enable - Disable - Verbose");
	virtual ~BaseSystem();

protected:
	virtual void Init() = 0;

private: // called by SetActive()
	virtual void Load() = 0; // Defined by child, attach to callbacks
	virtual void Unload() = 0; // Defined by child, unregister from callbacks

protected:
	/* Commande(s) console du système,
	donne vrai si la commande existe
	Le premier argument est en fait le troisième */
	virtual bool sys_cmd_fn(const SourceSdk::CCommand &args) { return false; };

public:
	virtual const char * cmd_list() final { return m_cmd_list; };

	/* Donne le nom du système pour pouvoir être identifié dans la console */
	virtual const char * GetName() const final { return m_name; };

	void SetDisabledByConfigIni() { m_isDisabled = true; SetActive(false); }
	bool GetDisabledByConfigIni() const { return m_isDisabled; }

private:
	/* Returns true if the system has no player to test and can be unloaded */
	bool ShouldUnload();

public:
	/* Process Load when m_isActive changes to true
	"    Unload when m_isActive changes to false */
	void SetActive(bool active);

	/* Must be used before all processing operations */
	bool IsActive() const { return m_isActive; };

	/* Retourne vrai si le fichier de configuration dit d'activer ce système */
	bool IsEnabledByConfig() const { return m_configState; };

	void SetConfig(bool enabled) { m_configState = enabled; };

#ifdef NCZ_USE_METRICS
	MetricsContainer<> & GetMetrics() { return m_metrics; };
#endif

	/* Permet de détruire tous les systèmes créés jusqu'à présent */
	static void UnloadAllSystems();

	/* Calls ShouldUnload for each systems and unloads them if true */

private:
	static void TryUnloadSystems();

	static void TryLoadSystems();

public:
	static void ManageSystems() { TryUnloadSystems(); TryLoadSystems(); };

	static void InitSystems();

	static BaseSystem* FindSystemByName(const char * name);

	/* Commande console de base "ncz" */
	static void ncz_cmd_fn(const SourceSdk::CCommand &args);
};

#endif
