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

#include "NczPlayer.h"

#include "Preprocessors.h"
#include "Players/ProcessFilter.h"
#include "Misc/temp_singleton.h"
#include "Misc/Helpers.h"
#include "Misc/ClassSpecifications.h"
#include "Systems/Logger.h"

class NczPlayerManager;

class PlayerHandler :
	protected NoCopy,
	protected NoMove
{
	friend NczPlayerManager;

public:
	class iterator;

	//typedef iterator const const_iterator;

	/*
		Wrapper of the PlayerHandler (pointer) type.
	*/
	class iterator
	{
		friend NczPlayerManager;

	private:
		PlayerHandler * m_ptr;

		inline PlayerHandler * GetHandler () const; // backdoor for NczPlayerManager

		/*
			Just returns true if m_ptr is in range. Will be only called in debug mode using Assert.
		*/
		inline bool IsIteratorValid () const;

	public:
		iterator () : m_ptr ( invalid.m_ptr )
		{}
		iterator ( PlayerHandler * const ptr ) : m_ptr ( ptr )
		{}
		iterator ( BaseProcessFilter const * const filter ) : m_ptr ( invalid.m_ptr )
		{
			this->operator+=( filter );
			LoggerAssert ( IsIteratorValid () );
		}
		iterator ( int const slot_index ) : m_ptr ( invalid.m_ptr + slot_index )
		{}
		iterator ( iterator const & other ) : m_ptr ( other.m_ptr )
		{}
		/*
			Assign by copy
		*/
		iterator & operator=( PlayerHandler * const ptr )
		{
			m_ptr = ptr;
			LoggerAssert ( IsIteratorValid () );
			return *this;
		}
		/*
			Assign by copy
		*/
		iterator & operator=( iterator const & other )
		{
			LoggerAssert ( other.IsIteratorValid () );
			m_ptr = other.m_ptr;
			return *this;
		}
		/*
			Assign by index (no check)
		*/
		iterator & operator=( int const slot_index )
		{
			m_ptr = invalid.m_ptr + slot_index;
			LoggerAssert ( IsIteratorValid () );
			return *this;
		}
		~iterator ()
		{}
		/*
			Advance the iterator to the next handler until the end.
		*/
		inline iterator & operator++();
		/*
			Advance the iterator to the next handler that matches the conditions in target
		*/
		inline iterator & operator+=( BaseProcessFilter const * const target );
		/*
			Returns the index of the handler
		*/
		inline int GetIndex () const;
		/*
			Returns true if the iterator is at the same index than other
		*/
		inline bool operator==(iterator & other ) const;
		/*
			Returns false if the iterator is at the same index than other
		*/
		inline bool operator!=(iterator & other ) const;
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
			Convert the iterator to the handler's status
		*/
		inline operator SlotStatus() const;
		/*
			Convert the iterator to the dereferenced NczPlayer pointer in the handler (might be null)
		*/
		inline NczPlayer* operator*() const;
		/*
			Convert the iterator to the dereferenced NczPlayer pointer in the handler (might be null)
		*/
		inline NczPlayer* operator->() const;
	};

private:

	static iterator first;
	static iterator last;
	static iterator invalid;

	SlotStatus status;
	NczPlayer* playerClass;
	double in_tests_time;

	PlayerHandler () :
		status ( SlotStatus::INVALID ),
		playerClass ( nullptr ),
		in_tests_time ( std::numeric_limits<float>::max () )
	{};

	PlayerHandler ( const PlayerHandler& other ) = delete;
	PlayerHandler& operator=( const PlayerHandler& other ) = delete;
	PlayerHandler (  PlayerHandler&& other ) = delete;
	PlayerHandler& operator=(  PlayerHandler&& other ) = delete;

	void Reset ()
	{
		if( playerClass ) delete playerClass;
		playerClass = nullptr;
		status = SlotStatus::INVALID;
		in_tests_time = std::numeric_limits<float>::max ();
	}

public:
	inline static iterator & begin ();

	inline static iterator & end ();

};

inline bool PlayerHandler::iterator::IsIteratorValid () const
{
	return m_ptr >= PlayerHandler::invalid.m_ptr && m_ptr < PlayerHandler::invalid.m_ptr + MAX_PLAYERS; // static memory range (fatal)
}

inline PlayerHandler * PlayerHandler::iterator::GetHandler () const
{
	LoggerAssert ( IsIteratorValid () );
	return const_cast< PlayerHandler * >( m_ptr );
}

inline PlayerHandler::iterator & PlayerHandler::iterator::operator++()
{
	LoggerAssert ( IsIteratorValid () );
	--m_ptr;
	return *this;
}

inline PlayerHandler::iterator & PlayerHandler::iterator::operator+=( BaseProcessFilter const * const target )
{
	LoggerAssert ( IsIteratorValid () );
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

inline PlayerHandler::iterator & PlayerHandler::begin ()
{
	return last; // Let's blow your brain ... it's reversed.
}

inline PlayerHandler::iterator & PlayerHandler::end ()
{
	return invalid;
}

inline PlayerHandler::iterator::operator bool () const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status != SlotStatus::INVALID;
}

inline bool PlayerHandler::iterator::operator!() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status == SlotStatus::INVALID;
}

inline int PlayerHandler::iterator::GetIndex () const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr - invalid.m_ptr;
}

inline bool PlayerHandler::iterator::operator==(iterator & other ) const
{
	LoggerAssert ( IsIteratorValid () );
	LoggerAssert ( other.IsIteratorValid () );
	return m_ptr == other.m_ptr;
}

inline bool PlayerHandler::iterator::operator!=(iterator & other ) const
{
	LoggerAssert ( IsIteratorValid () );
	LoggerAssert ( other.IsIteratorValid () );
	return m_ptr != other.m_ptr;
}

inline bool PlayerHandler::iterator::operator==( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status == other_status;
}

inline bool PlayerHandler::iterator::operator!=( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status != other_status;
}

inline bool PlayerHandler::iterator::operator>=( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status >= other_status;
}

inline bool PlayerHandler::iterator::operator<=( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status <= other_status;
}

inline bool PlayerHandler::iterator::operator>( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status > other_status;
}

inline bool PlayerHandler::iterator::operator<( SlotStatus const other_status ) const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status < other_status;
}

inline PlayerHandler::iterator::operator SlotStatus() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->status;
}

inline NczPlayer* PlayerHandler::iterator::operator*() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->playerClass;
}

inline NczPlayer* PlayerHandler::iterator::operator->() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->playerClass;
}

class CCSPlayer;

/* Distribue et met à jour l'état des slots du serveur */
class NczPlayerManager :
	public SourceSdk::IGameEventListener002,
	public Singleton
{
	friend PlayerHandler;

private:
	static PlayerHandler FullHandlersList[ MAX_PLAYERS + 1 ];
	int m_max_index;

	void ResetRange();
	
public:
	NczPlayerManager ();
	virtual ~NczPlayerManager ();

	/* Force la mise à jour des slots en scannant la mémoire pour EdictList
	   S'inscrit aux événements pour mettre à jour les slots en temps réel */
	void LoadPlayerManager ();
	void OnLevelInit ();

	inline PlayerHandler::iterator GetPlayerHandlerByIndex ( int const slot ) const;
	inline PlayerHandler::iterator GetPlayerHandlerByUserId ( int const userid ) const;
	inline PlayerHandler::iterator GetPlayerHandlerByBasePlayer ( void* BasePlayer ) const;
	PlayerHandler::iterator GetPlayerHandlerBySteamID ( const char * steamid ) const;
	inline PlayerHandler::iterator GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const;
	PlayerHandler::iterator GetPlayerHandlerByName ( const char * playerName ) const;

	short GetPlayerCount ( BaseProcessFilter const * const filter ) const;

	void ClientConnect ( SourceSdk::edict_t* pEntity ); // Bots don't call this ...
	void ClientActive ( SourceSdk::edict_t* pEntity ); // ... they call this at first
	void ClientDisconnect ( SourceSdk::edict_t *pEntity );
	void FireGameEvent ( SourceSdk::IGameEvent* ev );
	void DeclareKickedPlayer ( int const slot );

	void RT_Think ( double const & curtime );

	const int GetMaxIndex () const
	{
		return m_max_index;
	};
};

inline PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerByBasePlayer(void * const BasePlayer) const
{
	for (PlayerHandler::iterator it(PlayerHandler::begin()); it != PlayerHandler::end(); ++it)
	{
		if (it && it->GetEdict()->GetUnknown() == BasePlayer)
			return it;
	}

	return PlayerHandler::end();
}

inline PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerByIndex ( int const slot ) const
{
	return slot;
}

inline PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerByUserId ( int const userid ) const
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
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

inline PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const
{
	return Helpers::IndexOfEdict ( pEdict );
}

extern NczPlayerManager g_NczPlayerManager;

/* Utilisé en interne pour initialiser le tableau, des petites fonctions
  Ajout d'une case supplémentaire à FullHandlersList pour pouvoir quitter proprement la boucle PLAYERS_LOOP_RUNTIME */
#define _PLAYERS_LOOP_INIT { \
		int x = 0; \
		PlayerHandler* ph ( &(FullHandlersList[x])); \
		do{

#define _END_PLAYERS_LOOP_INIT  ++x;ph = &(FullHandlersList[x]);}while(x <= MAX_PLAYERS);}

#endif
