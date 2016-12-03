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

#ifndef PLAYERHANDLER_IMPL_H
#define PLAYERHANDLER_IMPL_H

#include "Misc/ClassSpecifications.h" // NoCopy, NoMove
#include "ProcessFilter.h" // BaseProcessFilter
#include "Systems/Logger.h" // LoggerAssert
#include "Players/NczPlayer.h" // Full definition required L199

class NczPlayerManager;

class alignas( 16 ) PlayerHandler :
	protected NoCopy,
	protected NoMove
{
	friend NczPlayerManager;

	typedef NczPlayer* const NczPlayer_ptr;

public:
	class iterator;

	typedef const iterator const_iterator;

	/*
	Wrapper of the PlayerHandler (pointer) type.
	*/
	class alignas(4) iterator
	{
		friend NczPlayerManager;

	private:
		mutable PlayerHandler const * m_ptr;

		inline PlayerHandler * GetHandler () const; // backdoor for NczPlayerManager

													/*
													Just returns true if m_ptr is in range. Will be only called in debug mode using Assert.
													*/
		inline bool IsIteratorValid () const;

	public:
		iterator () : m_ptr ( invalid.m_ptr )
		{}
		iterator ( PlayerHandler const * const ptr ) : m_ptr ( ptr )
		{}
		iterator ( BaseProcessFilter const * const filter ) : m_ptr ( invalid.m_ptr )
		{
			this->operator+=( filter );
			LoggerAssert ( IsIteratorValid () );
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
			LoggerAssert ( IsIteratorValid () );
			return *this;
		}
		/*
		Assign by copy
		*/
		const_iterator & operator=( const_iterator & other ) const
		{
			LoggerAssert ( other.IsIteratorValid () );
			m_ptr = other.m_ptr;
			return *this;
		}
		/*
		Assign by index (no check)
		*/
		const_iterator & operator=( int const slot_index ) const
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
	};

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
	PlayerHandler ( PlayerHandler&& other ) = delete;
	PlayerHandler& operator=( PlayerHandler&& other ) = delete;

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

inline PlayerHandler::const_iterator & PlayerHandler::iterator::operator++() const // CONST CHEATTERR
{
	LoggerAssert ( IsIteratorValid () );
	--m_ptr;
	return *this;
}

inline PlayerHandler::const_iterator & PlayerHandler::iterator::operator+=( BaseProcessFilter const * const target ) const
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

inline bool PlayerHandler::iterator::operator==( const_iterator & other ) const
{
	LoggerAssert ( IsIteratorValid () );
	LoggerAssert ( other.IsIteratorValid () );
	return m_ptr == other.m_ptr;
}

inline bool PlayerHandler::iterator::operator!=( const_iterator & other ) const
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

inline PlayerHandler::NczPlayer_ptr PlayerHandler::iterator::operator*() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->playerClass;
}

inline PlayerHandler::NczPlayer_ptr PlayerHandler::iterator::operator->() const
{
	LoggerAssert ( IsIteratorValid () );
	return m_ptr->playerClass;
}

#endif // PLAYERHANDLER_IMPL_H
