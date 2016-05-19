#include "EntityProps.h"

#include "Console/convar.h"
#include "Containers/utlvector.h"
#include "Interfaces/InterfacesProxy.h"

#include "Hooks/Hook.h"

#undef GetProp

void EntityProps::GetPropOffset(const basic_string& name, offset_t* pOffset)
{
	CUtlVector<basic_string> paths;
	SplitString<char>(name, '.', paths);

	size_t depth = 0;
	const size_t MaxDepth = paths.Size()-1;

	SourceSdk::ServerClass * pClass = SourceSdk::InterfacesProxy::Call_GetAllServerClasses();
	do
	{
		if(paths[depth].operator!=(pClass->m_pNetworkName)) continue;

		SourceSdk::SendTable * pTable = pClass->m_pTable;
		while(++depth < MaxDepth) // Follow path
		{
			for(offset_t prop = 0; prop < pTable->GetNumProps(); ++prop)
			{
				if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
				{
					const SourceSdk::SendProp_csgo* const pProp = static_cast<SourceSdk::SendProp_csgo*>(pTable->GetProp(prop));
					if(paths[depth].operator==(pProp->GetName()))
					{
						pTable = pProp->GetDataTable();
						break;
					}
				}
				else
				{
					const SourceSdk::SendProp* const pProp = static_cast<SourceSdk::SendProp*>(pTable->GetProp(prop));
					if (paths[depth].operator==(pProp->GetName()))
					{
						pTable = pProp->GetDataTable();
						break;
					}
				}
			}
		}
		// Find prop
		for(offset_t prop = 0; prop < pTable->m_nProps; ++prop)
		{
			if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
			{
				const SourceSdk::SendProp_csgo* const pProp = static_cast<SourceSdk::SendProp_csgo*>(pTable->GetProp(prop));
				if (paths[depth].operator==(pProp->GetName()))
				{
					*pOffset = pProp->GetOffset();
					// Register in our cache
					m_cache.AddToTail(PropertyCacheS(name, *pOffset, false));
					// Return the offset
					return;
				}
			}
			else
			{
				const SourceSdk::SendProp* const pProp = static_cast<SourceSdk::SendProp*>(pTable->GetProp(prop));
				if (paths[depth].operator==(pProp->GetName()))
				{
					*pOffset = pProp->GetOffset();
					// Register in our cache
					m_cache.AddToTail(PropertyCacheS(name, *pOffset, false));
					// Return the offset
					return;
				}
			}
		}
	} while((pClass = pClass->m_pNext) != nullptr);
	pOffset = 0;
}

bool EntityProps::FindInCache(const basic_string & path, offset_t * offset, bool type)
{
	size_t v = 0;
	size_t const max = m_cache.Size();
	for (; v != max; ++v)
	{
		PropertyCacheS & current = m_cache[v];
		if (current.prop_type != type) continue;
		if (current.prop_name.size() != path.size()) continue;

		bool comp_ok = true;

		char const * s1 = current.prop_name.c_str() + current.comp_pred;
		char const * s2 = path.c_str() + current.comp_pred;
		do
		{
			if (*s1 != *s2)
			{
				comp_ok = false;
				break;
			}
			++s1;
			++s2;
		} while (*s1 != '\0');

		if (comp_ok)
		{
			*offset = m_cache[v].prop_offset;
			return true;
		}
	}

	return false;
}

EntityProps g_EntityProps;

static SourceSdk::ConVar var_getdatadescmap_offset("ncz_getdatadescmap_offset", DEFAULT_GETDATADESCMAP_OFFSET);

typedef SourceSdk::datamap_t* (HOOKFN_EXT *GetDataDescMap_t)(SourceSdk::CBaseEntity*);

SourceSdk::datamap_t* GetDataDescMap(SourceSdk::edict_t* const pEntity)
{
	SourceSdk::CBaseEntity* const baseEnt = pEntity->GetUnknown()->GetBaseEntity();
	const DWORD* pdwInterface = IFACE_PTR(baseEnt);

	GetDataDescMap_t fn;
	*(DWORD*)&(fn) = pdwInterface[var_getdatadescmap_offset.GetInt()];

	return fn(baseEnt);
}

bool EntityProps::GetDataOffset(SourceSdk::datamap_t* dt, const basic_string& data_name, void* prop, offset_t* offset)
{
	Assert(dt && data_name.size() && offset);

	do
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			for (int i = 0; i < dt->dataNumFields; ++i)
			{
				SourceSdk::typedescription_t_csgo & dd = static_cast<SourceSdk::typedescription_t_csgo*>(dt->dataDesc)[i];
				if (dd.fieldName == nullptr)
				{
					continue;
				}
				if (strcmp(data_name.c_str(), dd.fieldName) == 0)
				{
					prop = &(dd);
					*offset = (dd.fieldOffset);

					m_cache.AddToTail(PropertyCacheS(data_name, *offset, true));

					return true;
				}
				if (dd.td == nullptr || !GetDataOffset(dd.td, data_name, prop, offset))
				{
					continue;
				}
				*offset += (offset_t)(dd.fieldOffset);
				m_cache.AddToTail(PropertyCacheS(data_name, *offset, true));
				return true;
			}

			dt = dt->baseMap;
		}
		else
		{
			for (int i = 0; i < dt->dataNumFields; ++i)
			{
				SourceSdk::typedescription_t & dd = static_cast<SourceSdk::typedescription_t*>(dt->dataDesc)[i];
				if (dd.fieldName == nullptr)
				{
					continue;
				}
				if (strcmp(data_name.c_str(), dd.fieldName) == 0)
				{
					prop = &(dd);
					*offset = *(dd.fieldOffset);

					m_cache.AddToTail(PropertyCacheS(data_name, *offset, true));

					return true;
				}
				if (dd.td == nullptr || !GetDataOffset(dd.td, data_name, prop, offset)) // HOT
				{
					continue;
				}
				*offset += (offset_t)(*dd.fieldOffset);
				m_cache.AddToTail(PropertyCacheS(data_name, *offset, true));
				return true;
			}

			dt = dt->baseMap;
		}
	}
	while(dt);

	return false;
}
