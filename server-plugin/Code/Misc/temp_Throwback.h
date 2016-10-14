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

#ifndef THROWBACK_H
#define THROWBACK_H

#include <limits>
#include <type_traits>

/*
	The purpose of this tool is to keep stored a compile-time specified amount of data, like a stack.

	Every time something is wrote in the variable, the stack is popped.
	This offer us the possibility to revert the change made in a variable and also take back the value he got at a certain moment.
*/

template < typename value_type,
	typename time_type,
	size_t count = 2
	/* Number of values to store including the current value. Can remember count-1 values back. */
>
class Throwback
{
	typedef Throwback<value_type, time_type, count> this_type;

	static_assert ( count >= 2, "count is history to keep + current value itself" );

public:
	struct inner_type
	{
		value_type v;
		time_type t;

		inner_type () : v( std::move ( value_type() ) ), t( std::numeric_limits<time_type>::max () )
		{
		}

		inner_type ( value_type value, time_type time ) : v(value), t(time)
		{
		}

		inner_type ( inner_type const &other ) : v(other.v), t(other.t)
		{
		}

		inner_type ( inner_type &&other ) : v ( std::move(other.v) ), t ( other.t )
		{
			other.t = std::numeric_limits<time_type>::max ();
		}

		inner_type& operator= ( inner_type const &other )
		{
			if( this != &other )
			{
				v = other.v;
				t = other.t;
			}

			return *this;
		}

		inner_type& operator= ( inner_type &&other )
		{
			if( this != &other )
			{
				v = std::move(other.v);
				t = other.t;
				other.t = std::numeric_limits<time_type>::max ();
			}

			return *this;
		}
	};

protected:
	inner_type m_stack[ count ];
	inner_type* m_current;

	inner_type * GetForward ()
	{
		if( ( m_current + 1 ) >= ( m_stack + count ) )
		{
			return m_stack;
		}
		else
		{
			return m_current + 1;
		}
	}

	inner_type * GetBackward ()
	{
		if( m_current == m_stack )
		{
			return m_stack + count - 1;
		}
		else
		{
			return m_current - 1;
		}
	}

	void GoForward ()
	{
		m_current = GetForward ();
	}

	void GoBackward ()
	{
		m_current = GetBackward ();
	}

public:
	void Reset ()
	{
		inner_type const * const cp_current ( m_current );
		inner_type default_v;

		do
		{
			*m_current = default_v;
			GoForward ();
		}
		while( m_current != cp_current );
	}

	Throwback () : m_current ( &( m_stack[ 0 ] ) )
	{
		static_assert ( std::is_copy_constructible< value_type>::value, "value_type is not copy constructible" );
		static_assert ( std::is_copy_assignable< value_type>::value, "value_type is not copy assignable" );
		static_assert ( std::is_move_constructible< value_type>::value, "value_type is not move constructible" );
		static_assert ( std::is_move_assignable< value_type>::value, "value_type is not move assignable" );
		static_assert ( std::is_arithmetic< time_type>::value, "time_type is not arithmetic" );
		static_assert ( std::is_signed< time_type >::value, "time_type must be signed" );

		Reset ();
	}

	Throwback ( this_type const & other ) : m_current ( m_stack + ( other.m_current - other.m_stack ) )
	{
		inner_type const * const cp_current ( m_current );

		do
		{
			*m_current = other.m_stack[ m_current - m_stack ];
			GoForward ();
		}
		while( m_current != cp_current );
	}

	Throwback ( this_type && other ) : m_current ( m_stack + ( other.m_current - other.m_stack ) )
	{
		inner_type const * const cp_current ( m_current );

		do
		{
			*m_current = std::move( other.m_stack[ m_current - m_stack ] );
			GoForward ();
		}
		while( m_current != cp_current );
	}

	~Throwback ()
	{
	}

	Throwback& operator=( this_type const & other )
	{
		if( this != &other )
		{
			m_current = m_stack + ( other.m_current - other.m_stack );
			inner_type const * const cp_current ( m_current );

			do
			{
				*m_current = other.m_stack [ m_current-m_stack ];
				GoForward ();
			}
			while( m_current != cp_current );
		}

		return *this;
	}

	Throwback& operator=( this_type && other )
	{
		if( this != &other )
		{
			m_current = m_stack + ( other.m_current - other.m_stack );
			inner_type const * const cp_current ( m_current );

			do
			{
				*m_current = std::move ( other.m_stack[ m_current - m_stack ] );
				GoForward ();
			}
			while( m_current != cp_current );
		}

		return *this;
	}

	void Revert ()
	{
		inner_type const * const past ( GetBackward () );
		GoForward ();
		*m_current = *past;
	}

	void Store ( value_type value, time_type time )
	{
		GoForward ();
		m_current->v = value;
		m_current->t = time;
	}

	void Store ( inner_type &&data )
	{
		GoForward ();
		*m_current = std::move(data);
	}

	void CopyHistory ( inner_type* dest, size_t& amount, size_t max_amount = count, time_type time_range = std::numeric_limits<time_type>::max (), time_type now = std::numeric_limits<time_type>::max () )
	{
		inner_type * const cp_current ( m_current );
		amount = 0;

		do
		{
			if( m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( now - m_current->t > time_range )
			{
				break;
			}
			*dest++ = *m_current;
			if( ++amount >= max_amount )
			{
				break;
			}
			GoBackward ();
		}
		while( m_current != cp_current );

		m_current = cp_current;
	}
};

template < typename value_type,
			typename time_type,
			size_t count = 2 
			/* Number of values to store including the current value. Can remember count-1 values back. */
		>
class Throwback_Arithmetic : public Throwback<value_type, time_type, count>
{
	typedef Throwback_Arithmetic<value_type, time_type, count> this_type;
	
public:
	typedef Throwback<value_type, time_type, count> base_class;
	typedef typename base_class::inner_type inner_type;

	Throwback_Arithmetic () : base_class()
	{
		static_assert ( std::is_arithmetic< value_type>::value, "value_type is not arithmetic" );
		static_assert ( std::is_arithmetic< time_type>::value, "time_type is not arithmetic" );
		static_assert ( std::is_signed< time_type >::value, "time_type must be signed" );
	}

	Throwback_Arithmetic ( this_type const & other ) : base_class(other)
	{
	}

	Throwback_Arithmetic ( this_type && other ) : base_class(other)
	{
	}

	~Throwback_Arithmetic ()
	{
	}

	Throwback_Arithmetic& operator=( this_type const & other )
	{
		base_class::operator=( other );
		return *this;
	}

	Throwback_Arithmetic& operator=( this_type const && other )
	{
		base_class::operator=( other );
		return *this;
	}

	float Average ( value_type range_value_min = std::numeric_limits<value_type>::min(),
					value_type range_value_max = std::numeric_limits<value_type>::max ())
	{
		inner_type * const cp_current ( this->m_current );
		value_type sum ( 0 );

		size_t amount ( 0 );

		do
		{
			if( this->m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( this->m_current->v >= range_value_min && this->m_current->v <= range_value_max )
			{
				++amount;
				sum += this->m_current->v;
			}
			this->GoBackward ();
		}
		while( this->m_current != cp_current );

		this->m_current = cp_current;

		return (float)sum / (float) amount;
	}

	value_type Min ()
	{
		inner_type * const cp_current ( this->m_current );
		value_type min ( this->m_current->v );

		do
		{
			if( this->m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( this->m_current->v < min )
			{
				min = this->m_current->v;
			}
			this->GoBackward ();
		}
		while( this->m_current != cp_current );

		this->m_current = cp_current;

		return min;
	}

	value_type Max ()
	{
		inner_type * const cp_current ( this->m_current );
		value_type max ( this->m_current->v );

		do
		{
			if( this->m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( this->m_current->v > max )
			{
				max = this->m_current->v;
			}
			this->GoBackward ();
		}
		while( this->m_current != cp_current );

		this->m_current = cp_current;

		return max;
	}

	time_type TimeSpan ()
	{
		inner_type * const cp_current ( this->m_current );
		inner_type const * min ( this->m_current );
		time_type const max ( this->m_current->t );

		do
		{
			if( this->m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			min = this->m_current;
			this->GoBackward ();
		}
		while( this->m_current != cp_current );

		this->m_current = cp_current;

		return max - min->t;
	}
};

#endif // THROWBACK_H
