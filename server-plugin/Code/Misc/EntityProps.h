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

#ifndef ENTPROPS_H
#define ENTPROPS_H

#undef GetClassName
#include "SdkPreprocessors.h"
#include "Interfaces/InterfacesProxy.h"
#include "Interfaces/edict.h"
#include "Interfaces/server_class.h"
#include "Interfaces/datamap.h"

#include "temp_basiclist.h"
#include "temp_basicstring.h"
#include "temp_singleton.h"
#include "Helpers.h"
#include "Systems/Logger.h"
#include "Systems/ConfigManager.h"

typedef int offset_t;

class CBaseEntity;

enum UsedProps
{
	PROP_ABS_VELOCITY,
	PROP_FLASH_MAX_ALPHA,
	PROP_FLASH_DURATION,
	PROP_FLAGS,
	PROP_PLAYER_SPOTTED,
	PROP_BOMB_SPOTTED,
	PROP_OBSERVER_MODE,
	PROP_OBSERVER_TARGET,
	PROP_LERP_TIME,
	PROP_SHADOW_DIRECTION,
	PROP_SHADOW_MAX_DIST,
	PROP_DISABLE_SHADOW,
	PROP_VIEW_OFFSET,
	PROP_OWNER,

	PROP_COUNT
};

typedef struct PropertyCacheS
{
	bool cache_set;
	offset_t prop_offset;

	PropertyCacheS () : cache_set ( false ), prop_offset ( 0 )
	{};
	PropertyCacheS ( const PropertyCacheS& other )
	{
		memcpy ( this, &other, sizeof ( PropertyCacheS ) );
	};
	PropertyCacheS& operator=( const PropertyCacheS& other )
	{
		cache_set = other.cache_set;
		prop_offset = other.prop_offset;
		return *this;
	};
	PropertyCacheS ( offset_t offset ) : cache_set ( true ), prop_offset ( offset )
	{};
} PropertyCacheT;

SourceSdk::datamap_t* GetDataDescMap ( SourceSdk::edict_t* const pEntity );

#undef GetProp

class EntityProps :
	public Singleton
{
private:
	/* I believe offsets are consistents so don't need to reset the cache during runtime */
	PropertyCacheS m_cache[ PROP_COUNT ];

	template <UsedProps id>
	basic_string const & PropIdToString ();

	template <UsedProps id>
	void GetPropOffset ( offset_t* pOffset )
	{
		CUtlVector<basic_string> paths;
		SplitString<char> ( PropIdToString<id> (), '.', paths );

		size_t depth ( 0 );
		const size_t MaxDepth ( paths.Size () - 1 );

		*pOffset = 0;

		SourceSdk::ServerClass * pClass = SourceSdk::InterfacesProxy::Call_GetAllServerClasses ();
		do
		{
			if( paths[ depth ].operator!=( pClass->m_pNetworkName ) ) continue;

			void * pTable ( pClass->m_pTable );
			while( ++depth < MaxDepth ) // Follow path
			{
				int const numprops ( static_cast< SourceSdk::SendTable* >( pTable )->m_nProps );
				for( offset_t prop ( 0 ); prop < numprops; ++prop )
				{
					if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
					{
#undef GetProp
						const SourceSdk::SendProp_csgo* const pProp ( static_cast< SourceSdk::SendProp_csgo* >( static_cast< SourceSdk::SendTable_csgo* >( pTable )->GetProp ( prop ) ) );
						if( paths[ depth ].operator==( pProp->GetName () ) )
						{
							pTable = pProp->GetDataTable ();
							*pOffset += pProp->GetOffset();
							break;
						}
					}
					else
					{
						const SourceSdk::SendProp* const pProp ( static_cast< SourceSdk::SendProp* >( static_cast< SourceSdk::SendTable* >( pTable )->GetProp ( prop ) ) );
						if( paths[ depth ].operator==( pProp->GetName () ) )
						{
							pTable = pProp->GetDataTable ();
							*pOffset += pProp->GetOffset();
							break;
						}
					}
				}
			}
			// Find prop
			int const numprops ( static_cast< SourceSdk::SendTable* >( pTable )->m_nProps );
			for( offset_t prop ( 0 ); prop < numprops; ++prop )
			{
				if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
				{
					const SourceSdk::SendProp_csgo* const pProp ( static_cast< SourceSdk::SendProp_csgo* >( static_cast< SourceSdk::SendTable_csgo* >( pTable )->GetProp ( prop ) ) );
					if( paths[ depth ].operator==( pProp->GetName () ) )
					{
						*pOffset += pProp->GetOffset ();
						// Register in our cache
						m_cache[ id ] = PropertyCacheS ( *pOffset );
						// Return the offset
						return;
					}
				}
				else
				{
					const SourceSdk::SendProp* const pProp ( static_cast< SourceSdk::SendProp* >( static_cast< SourceSdk::SendTable* >( pTable )->GetProp ( prop ) ) );
					if( paths[ depth ].operator==( pProp->GetName () ) )
					{
						*pOffset += pProp->GetOffset ();
						// Register in our cache
						m_cache[ id ] = PropertyCacheS ( *pOffset );
						// Return the offset
						return;
					}
				}
			}
		}
		while( ( pClass = pClass->m_pNext ) != nullptr );
		pOffset = 0;
	}

	template <UsedProps id>
	bool FindInCache ( offset_t * offset )
	{
		if( m_cache[ id ].cache_set == true )
		{
			*offset = m_cache[ id ].prop_offset;
			return true;
		}
		return false;
	}

	template <UsedProps id>
	bool GetDataOffset ( SourceSdk::datamap_t* dt, offset_t* offset )
	{
		CUtlVector<basic_string> paths;
		SplitString<char> ( PropIdToString<id> (), '.', paths );

		basic_string const & data_class ( paths[ 0 ] );
		basic_string const & data_name ( paths[ 1 ] );

		do
		{
			if( data_class.operator==( dt->dataClassName ) )
			{
				for( int i ( 0 ); i < dt->dataNumFields; ++i )
				{
					if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
					{
						SourceSdk::typedescription_t_csgo const & td ( static_cast< SourceSdk::typedescription_t_csgo const * >( dt->dataDesc )[ i ] );

						if( data_name.operator==( td.fieldName ) )
						{
							*offset = td.fieldOffset;

							m_cache[ id ] = PropertyCacheS ( *offset );

							return true;
						}
					}
					else
					{
						SourceSdk::typedescription_t const & td ( static_cast< SourceSdk::typedescription_t const * >( dt->dataDesc )[ i ] );

						if( data_name.operator==( td.fieldName ) )
						{
							*offset = *( td.fieldOffset );

							m_cache[ id ] = PropertyCacheS ( *offset );

							return true;
						}
					}
				}
			}
		}
		while( ( dt = dt->baseMap ) != nullptr );

		return false;
	}

public:
	EntityProps ()
	{};
	virtual ~EntityProps () final
	{};

	template<typename T, UsedProps prop_id>
	T* GetPropValue ( SourceSdk::edict_t * const pEdict, bool type = false )
	{
		SourceSdk::CBaseEntity * const pBase ( pEdict->GetUnknown ()->GetBaseEntity () );
		offset_t offset ( 0 );
		if( !FindInCache<prop_id> ( &offset ) )
		{
			if( !type )
			{
				GetPropOffset<prop_id> ( &offset );
			}
			else
			{
				SourceSdk::datamap_t* const dt ( GetDataDescMap ( pEdict ) );
				GetDataOffset<prop_id> ( dt, &offset );
			}
		}
		LoggerAssert ( offset > 0 );
		return reinterpret_cast< T * >( reinterpret_cast< uint8_t * >( pBase ) + offset );
	};
};

extern EntityProps g_EntityProps;

#endif
