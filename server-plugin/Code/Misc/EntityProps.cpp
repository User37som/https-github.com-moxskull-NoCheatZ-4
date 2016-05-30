#include "EntityProps.h"

#include "Console/convar.h"
#include "Containers/utlvector.h"
#include "Interfaces/InterfacesProxy.h"
#include "Systems/ConfigManager.h"

#include "Hooks/Hook.h"

#undef GetProp

typedef SourceSdk::datamap_t* (HOOKFN_EXT *GetDataDescMap_t)(SourceSdk::CBaseEntity*);

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

		void * pTable = pClass->m_pTable;
		while(++depth < MaxDepth) // Follow path
		{
			int const numprops = static_cast<SourceSdk::SendTable*>(pTable)->m_nProps;
			for(offset_t prop = 0; prop < numprops; ++prop)
			{
				if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
				{
					const SourceSdk::SendProp_csgo* const pProp = static_cast<SourceSdk::SendProp_csgo*>(static_cast<SourceSdk::SendTable_csgo*>(pTable)->GetProp(prop));
					if(paths[depth].operator==(pProp->GetName()))
					{
						pTable = pProp->GetDataTable();
						break;
					}
				}
				else
				{
					const SourceSdk::SendProp* const pProp = static_cast<SourceSdk::SendProp*>(static_cast<SourceSdk::SendTable*>(pTable)->GetProp(prop));
					if (paths[depth].operator==(pProp->GetName()))
					{
						pTable = pProp->GetDataTable();
						break;
					}
				}
			}
		}
		// Find prop
		int const numprops = static_cast<SourceSdk::SendTable*>(pTable)->m_nProps;
		for(offset_t prop = 0; prop < numprops; ++prop)
		{
			if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
			{
				const SourceSdk::SendProp_csgo* const pProp = static_cast<SourceSdk::SendProp_csgo*>(static_cast<SourceSdk::SendTable_csgo*>(pTable)->GetProp(prop));
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
				const SourceSdk::SendProp* const pProp = static_cast<SourceSdk::SendProp*>(static_cast<SourceSdk::SendTable*>(pTable)->GetProp(prop));
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

	DebugMessage(Helpers::format("EntityProps : Prop %s Not found in cache.", path.c_str()).c_str());
	return false;
}

SourceSdk::datamap_t* GetDataDescMap(SourceSdk::edict_t* const pEntity)
{
	SourceSdk::CBaseEntity* const baseEnt = pEntity->GetUnknown()->GetBaseEntity();
	const DWORD* pdwInterface = IFACE_PTR(baseEnt);

	GetDataDescMap_t fn;
	*(DWORD*)&(fn) = pdwInterface[ConfigManager::GetInstance()->GetVirtualFunctionId("getdatadescmap")];

	return fn(baseEnt);
}

/*bool EntityProps::GetDataOffset_old(SourceSdk::datamap_t* dt, const basic_string& data_name, void* prop, offset_t* offset)
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
		}
	}
	while((dt = dt->baseMap) != nullptr);

	return false;
}*/

bool EntityProps::GetDataOffset(SourceSdk::datamap_t* dt, basic_string const & path, offset_t* offset)
{
	Assert(dt && path.size() && offset);

	CUtlVector<basic_string> paths;
	SplitString<char>(path, '.', paths);

	basic_string const & data_class = paths[0];
	basic_string const & data_name = paths[1];

	do
	{
		if (data_class.operator==(dt->dataClassName))
		{
			for (int i = 0; i < dt->dataNumFields; ++i)
			{
				if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
				{
					SourceSdk::typedescription_t_csgo const & td = static_cast<SourceSdk::typedescription_t_csgo const *>(dt->dataDesc)[i];

					if (data_name.operator==(td.fieldName))
					{
						*offset = td.fieldOffset;

						m_cache.AddToTail(PropertyCacheS(path, *offset, true));

						return true;
					}
				}
				else
				{
					SourceSdk::typedescription_t const & td = static_cast<SourceSdk::typedescription_t const *>(dt->dataDesc)[i];

					if (data_name.operator==(td.fieldName))
					{
						*offset = *(td.fieldOffset);

						m_cache.AddToTail(PropertyCacheS(path, *offset, true));

						return true;
					}
				}
			}
		}
	} while ((dt = dt->baseMap) != nullptr);

	return false;
}
