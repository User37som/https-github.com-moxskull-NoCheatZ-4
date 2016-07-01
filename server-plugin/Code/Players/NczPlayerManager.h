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

#ifndef NCZPLAYERMANAGER
#define NCZPLAYERMANAGER

#include <limits>
#include <cstring> // memset, memcpy

#include "SdkPreprocessors.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"

#include "Preprocessors.h"
#include "NczPlayer.h"
#include "Players/ProcessFilter.h"
#include "Misc/temp_singleton.h"
#include "Misc/Helpers.h"
#include "Misc/ClassSpecifications.h"

class NczPlayerManager;

class ALIGN16 PlayerHandler :
	protected NoCopy,
	protected NoMove
{
	friend NczPlayerManager;

	typedef NczPlayer* const NczPlayer_ptr;

public:
	class iterator;

	typedef iterator const const_iterator;

	/*
		Wrapper of the PlayerHandler (pointer) type.
	*/
	class ALIGN4 iterator
	{
		friend NczPlayerManager;

	private:
		mutable PlayerHandler const * m_ptr;

		inline PlayerHandler * GetHandler () const; // backdoor for NczPlayerManager

	public:
		iterator () : m_ptr ( invalid.m_ptr )
		{}
		iterator ( PlayerHandler const * const ptr ) : m_ptr ( ptr )
		{}
		iterator ( BaseProcessFilter const * const filter ) : m_ptr ( invalid.m_ptr )
		{
			this->operator+=( filter );
		}
		iterator ( int const slot_index ) : m_ptr ( invalid.m_ptr + slot_index )
		{}
		iterator ( const_iterator & other ) : m_ptr ( other.m_ptr )
		{}
		/*
			Assign by copy
		*/
		const_iterator & operator=( PlayerHandler const * const ptr ) const
		{
			m_ptr = ptr;
			return *this;
		}
		/*
			Assign by copy
		*/
		const_iterator & operator=( const_iterator & other ) const
		{
			m_ptr = other.m_ptr;
			return *this;
		}
		/*
			Assign by index (no check)
		*/
		const_iterator & operator=( int const slot_index ) const
		{
			m_ptr = invalid.m_ptr + slot_index;
			return *this;
		}
		~iterator ()
		{}
		/*
			Advance the iterator to the next handler until the end.
		*/
		inline const_iterator & operator++() const;
		/*
			Advance the iterator to the next handler that matches the conditions in target
		*/
		inline const_iterator & operator+=( BaseProcessFilter const * const target ) const;
		/*
			Returns the index of the handler
		*/
		inline int GetIndex () const;
		/*
			Returns true if the iterator is at the same index than other
		*/
		inline bool operator==( const_iterator & other ) const;
		/*
			Returns false if the iterator is at the same index than other
		*/
		inline bool operator!=( const_iterator & other ) const;
		/*
			Returns true if the status of the handler matches other_status
		*/
		inline bool operator==( SlotStatus const other_status ) const;
		/*
			Returns true if the status of the handler doesn't matches other_status
		*/
		inline bool operator!=( SlotStatus const other_status ) const;
		/*
			Returns true if the status of the handler is greater or equal than other_status
		*/
		inline bool operator>=( SlotStatus const other_status ) const;
		/*
			Returns true if the status of the handler is lower or equal than other_status
		*/
		inline bool operator<=( SlotStatus const other_status ) const;
		/*
			Returns true if the status of the handler is greater than other_status
		*/
		inline bool operator>( SlotStatus const other_status ) const;
		/*
			Returns true if the status of the handler is lower than other_status
		*/
		inline bool operator<( SlotStatus const other_status ) const;
		/*
			Returns true if the handler got a valid status (= NczPlayer class is allocated)
		*/
		inline operator bool () const;
		/*
			Returns false if the handler got a valid status (= NczPlayer class is allocated)
		*/
		inline bool operator!() const;
		/*
			Convert the iterator to the NczPlayer pointer in the handler (might be null)
		*/
		inline operator NczPlayer_ptr() const;
		/*
			Convert the iterator to the handler's status
		*/
		inline operator SlotStatus() const;
		/*
			Convert the iterator to the dereferenced NczPlayer pointer in the handler (might be null)
		*/
		inline NczPlayer_ptr operator*() const;
		/*
			Convert the iterator to the dereferenced NczPlayer pointer in the handler (might be null)
		*/
		inline NczPlayer_ptr operator->() const;
	} ALIGN4_POST;

private:

	static const_iterator first;
	static const_iterator last;
	static const_iterator invalid;

	SlotStatus status;
	NczPlayer* playerClass;
	float in_tests_time;

	PlayerHandler () :
		status ( SlotStatus::INVALID ),
		playerClass ( nullptr ),
		in_tests_time ( std::numeric_limits<float>::max () )
	{};

	PlayerHandler ( const PlayerHandler& other ) = delete;
	PlayerHandler& operator=( const PlayerHandler& other ) = delete;

	void Reset ()
	{
		if( playerClass ) delete playerClass;
		playerClass = nullptr;
		status = SlotStatus::INVALID;
		in_tests_time = std::numeric_limits<float>::max ();
	}

public:
	inline static const_iterator & begin ();

	inline static const_iterator & end ();

} ALIGN16_POST;

inline PlayerHandler * PlayerHandler::iterator::GetHandler () const
{
	return const_cast< PlayerHandler * >( m_ptr );
}

inline PlayerHandler::const_iterator & PlayerHandler::iterator::operator++() const // CONST CHEATTERR
{
	--m_ptr;
	return *this;
}

inline PlayerHandler::const_iterator & PlayerHandler::iterator::operator+=( BaseProcessFilter const * const target ) const
{
	if( this->operator==( PlayerHandler::end () ) ) // We already are at the end ... but our tester wants to work :'(
	{
		this->operator=( begin () );
	}
	else
	{
		this->operator++();
	}
	while( this->operator!=( PlayerHandler::end () ) ) // begin can still be invalid when server is empty
	{
		if( target->CanProcessThisSlot ( this->operator SlotStatus() ) )
		{
			return *this;
		}
		else
		{
			this->operator++();
		}
	}

	return *this;
}

inline PlayerHandler::const_iterator & PlayerHandler::begin ()
{
	return last; // Let's blow your brain ... it's reversed.
}

inline PlayerHandler::const_iterator & PlayerHandler::end ()
{
	return invalid;
}

inline PlayerHandler::iterator::operator bool () const
{
	return m_ptr->status != SlotStatus::INVALID;
}

inline bool PlayerHandler::iterator::operator!() const
{
	return m_ptr->status == SlotStatus::INVALID;
}

inline PlayerHandler::iterator::operator NczPlayer_ptr() const
{
	return m_ptr->playerClass;
}

inline int PlayerHandler::iterator::GetIndex () const
{
	return m_ptr - invalid.m_ptr;
}

inline bool PlayerHandler::iterator::operator==( const_iterator & other ) const
{
	return m_ptr == other.m_ptr;
}

inline bool PlayerHandler::iterator::operator!=( const_iterator & other ) const
{
	return m_ptr != other.m_ptr;
}

inline bool PlayerHandler::iterator::operator==( SlotStatus const other_status ) const
{
	return m_ptr->status == other_status;
}

inline bool PlayerHandler::iterator::operator!=( SlotStatus const other_status ) const
{
	return m_ptr->status != other_status;
}

inline bool PlayerHandler::iterator::operator>=( SlotStatus const other_status ) const
{
	return m_ptr->status >= other_status;
}

inline bool PlayerHandler::iterator::operator<=( SlotStatus const other_status ) const
{
	return m_ptr->status <= other_status;
}

inline bool PlayerHandler::iterator::operator>( SlotStatus const other_status ) const
{
	return m_ptr->status > other_status;
}

inline bool PlayerHandler::iterator::operator<( SlotStatus const other_status ) const
{
	return m_ptr->status < other_status;
}

inline PlayerHandler::iterator::operator SlotStatus() const
{
	return m_ptr->status;
}

inline PlayerHandler::NczPlayer_ptr PlayerHandler::iterator::operator*() const
{
	return m_ptr->playerClass;
}

inline PlayerHandler::NczPlayer_ptr PlayerHandler::iterator::operator->() const
{
	return m_ptr->playerClass;
}

class CCSPlayer;

/* Distribue et met à jour l'état des slots du serveur */
class NczPlayerManager :
	public SourceSdk::IGameEventListener002,
	public HeapMemoryManager::OverrideNew<16>,
	public Singleton<NczPlayerManager>
{
	typedef  Singleton<NczPlayerManager> singleton_class;
	friend PlayerHandler;

private:
	static PlayerHandler FullHandlersList[ MAX_PLAYERS + 1 ];
	int m_max_index;

public:
	NczPlayerManager ();
	virtual ~NczPlayerManager () final;

	/* Force la mise à jour des slots en scannant la mémoire pour EdictList
	   S'inscrit aux événements pour mettre à jour les slots en temps réel */
	void LoadPlayerManager ();
	void OnLevelInit ();

	inline PlayerHandler::const_iterator GetPlayerHandlerByIndex ( int const slot ) const;
	inline PlayerHandler::const_iterator GetPlayerHandlerByUserId ( int const userid ) const;
	PlayerHandler::const_iterator GetPlayerHandlerByBasePlayer ( void* BasePlayer ) const;
	PlayerHandler::const_iterator GetPlayerHandlerBySteamID ( const char * steamid ) const;
	inline PlayerHandler::const_iterator GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const;
	PlayerHandler::const_iterator GetPlayerHandlerByName ( const char * playerName ) const;

	short GetPlayerCount ( BaseProcessFilter const * const filter ) const;

	void ClientConnect ( SourceSdk::edict_t* pEntity ); // Bots don't call this ...
	void ClientActive ( SourceSdk::edict_t* pEntity ); // ... they call this at first
	void ClientDisconnect ( SourceSdk::edict_t *pEntity );
	void FireGameEvent ( SourceSdk::IGameEvent* ev );
	void DeclareKickedPlayer ( int const slot );

	void RT_Think ( float const curtime );

	const int GetMaxIndex () const
	{
		return m_max_index;
	};
};

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByIndex ( int const slot ) const
{
	return slot;
}

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByUserId ( int const userid ) const
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			if( it->m_playerinfo )
			{
				if( it->m_playerinfo->GetUserID () == userid )
					return it;
			}
		}
	}
	return PlayerHandler::end ();
}

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const
{
	return Helpers::IndexOfEdict ( pEdict );
}

/* Utilisé en interne pour initialiser le tableau, des petites fonctions
  Ajout d'une case supplémentaire à FullHandlersList pour pouvoir quitter proprement la boucle PLAYERS_LOOP_RUNTIME */
#define _PLAYERS_LOOP_INIT { \
		int x = 0; \
		PlayerHandler* ph ( &(FullHandlersList[x])); \
		do{

#define _END_PLAYERS_LOOP_INIT  ++x;ph = &(FullHandlersList[x]);}while(x <= MAX_PLAYERS);}

#endif
