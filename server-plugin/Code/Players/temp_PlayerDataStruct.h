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

#ifndef PLAYERDATASTRUCT
#define PLAYERDATASTRUCT

#include "Preprocessors.h"
#include "Misc/temp_BaseDataStruct.h"
#include "NczPlayer.h"
#include "Misc/Helpers.h"

template <typename DataT>
class PlayerDataStructHandler :
	public BaseDataStructHandler<DataT, MAX_PLAYERS>
{
	typedef BaseDataStructHandler<DataT, MAX_PLAYERS> BaseClass;

public:
	inline void ResetPlayerDataStructByIndex ( int const index )
	{
		this->m_dataStruct[ index ] = DataT ();
	};
	inline void ResetPlayerDataStruct ( SourceSdk::edict_t const * const player )
	{
		ResetPlayerDataStructByIndex ( Helpers::IndexOfEdict ( player ) );
	};
	inline void ResetPlayerDataStruct ( NczPlayer const * const player )
	{
		ResetPlayerDataStructByIndex ( player->GetIndex () );
	};

protected:
	PlayerDataStructHandler () :
		BaseClass ()
	{};
	virtual ~PlayerDataStructHandler () override
	{};

	inline DataT* GetPlayerDataStructByIndex ( int const index ) const
	{
		return ( DataT* ) ( &( this->m_dataStruct[ index ] ) );
	};
	inline DataT* GetPlayerDataStruct ( NczPlayer const * const player ) const
	{
		return GetPlayerDataStructByIndex ( player->GetIndex () );
	};
	inline DataT* GetPlayerDataStruct ( SourceSdk::edict_t const * const player ) const
	{
		return GetPlayerDataStructByIndex ( Helpers::IndexOfEdict ( player ) );
	};
};

#endif
