#ifndef TEMP_BASICLIST
#define TEMP_BASICLIST

#include <limits>
#include <cstdlib>

#include "Preprocessors.h"
#include "HeapMemoryManager.h"

template <typename inner_type>
class basic_slist
{
public:
	struct elem_t :
		HeapMemoryManager::OverrideNew<8>
	{
		elem_t ( inner_type const & newvalue ) : m_value ( newvalue )
		{}

		mutable inner_type m_value;
		elem_t* m_next;
	};

private:

	elem_t* m_first;

public:

	basic_slist () : m_first ( nullptr )
	{}

	~basic_slist ()
	{
		while( m_first != nullptr )
		{
			Remove ( m_first );
		}
	}

	constexpr elem_t* GetFirst () const
	{
		return m_first;
	}

	/*
	Add an element to front.
	*/
	elem_t* Add ( inner_type const & value )
	{
		if( m_first != nullptr )
		{
			elem_t* const old_first ( m_first );
			m_first = new elem_t ( value );
			__assume ( m_first != nullptr );
			m_first->m_next = old_first;
		}
		else
		{
			m_first = new elem_t ( value );
			__assume ( m_first != nullptr );
			m_first->m_next = nullptr;
		}
		return m_first;
	}

	/*
	Find this iterator and remove it from list. Returns the new iterator at this position.
	*/
	elem_t* Remove ( elem_t* it )
	{
		elem_t* iterator ( m_first );
		elem_t* prev ( nullptr );
		elem_t* return_value ( nullptr );
		while( iterator != nullptr )
		{
			if( iterator == it )
			{
				return_value = iterator->m_next;
				if( prev == nullptr )
				{
					m_first = iterator->m_next;
				}
				else
				{
					prev->m_next = iterator->m_next;
				}
				delete it;
				return return_value;
			}
			prev = iterator;
			iterator = iterator->m_next;
		}

		return return_value;
	}

	/*
	Find this value and remove it from list. Returns the new iterator at this position.
	*/
	void Remove ( inner_type const & value )
	{
		elem_t* iterator ( m_first );
		elem_t* prev ( nullptr );
		while( iterator != nullptr )
		{
			if( iterator->m_value == value )
			{
				elem_t* save ( iterator->m_next );
				if( prev == nullptr )
				{
					m_first = iterator->m_next;
				}
				else
				{
					prev->m_next = iterator->m_next;
				}
				delete iterator;
				iterator = save;
				continue;
			}
			prev = iterator;
			iterator = iterator->m_next;
		}
	}

	/*
	Find this value
	*/
	elem_t* const Find ( inner_type const & value ) const
	{
		elem_t* iterator ( m_first );
		while( iterator != nullptr )
		{
			if( iterator->m_value == value )
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}
};

template <class me>
class ListMe
{
	typedef ListMe<me> hClass;

private:
	static me* m_first;
	me* m_next;

	me const * const GetMe ( void ) const
	{
		return static_cast< me const * const >( this );
	}

protected:
	ListMe ( void )
	{
		if( hClass::m_first == nullptr )
		{
			hClass::m_first = const_cast< me* >( GetMe () );
			m_next = nullptr;
		}
		else
		{
			me * const old ( hClass::m_first );
			hClass::m_first = const_cast< me* >( GetMe () );
			m_next = old;
		}
	}

	virtual ~ListMe ( void )
	{
		me* iterator ( hClass::m_first);
		me* prev ( nullptr );
		while( iterator != nullptr )
		{
			if( iterator->GetMe () == GetMe () )
			{
				me* save ( iterator->m_next );
				if( prev == nullptr )
				{
					hClass::m_first = iterator->m_next;
				}
				else
				{
					prev->m_next = iterator->m_next;
				}
				iterator = save;
				continue;
			}
			prev = iterator;
			iterator = iterator->m_next;
		}
	};

public:
	static me * const GetFirst ( void )
	{
		return hClass::m_first;
	}
	me * const GetNext ( void ) const
	{
		return m_next;
	};
	static void GetNext ( me*& ptr )
	{
		ptr = ptr->m_next;
	};
};

template <class me>
me* ListMe<me>::m_first ( nullptr );

#endif

