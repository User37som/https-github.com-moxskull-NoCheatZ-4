#ifndef TEMP_BASICLIST
#define TEMP_BASICLIST

#include <new>
#include <limits>
#include <cstdlib>

template <typename inner_type>
class basic_slist
{
public:
	struct elem_t
	{
		elem_t(inner_type const & newvalue) : m_value(newvalue) {}

		inner_type m_value;
		elem_t* m_next;
	};

private:

	elem_t* m_first;

public:

	basic_slist()
	{
		m_first = nullptr;
	}

	~basic_slist()
	{
		while (m_first != nullptr)
		{
			Remove(m_first);
		}
	}

	constexpr elem_t* GetFirst() const
	{
		return m_first;
	}

	/*
	Add an element to front.
	*/
	elem_t* Add(inner_type const & value)
	{
		if (m_first != nullptr)
		{
			elem_t* const old_first = m_first;
			m_first = new elem_t(value);
			m_first->m_next = old_first;
		}
		else
		{
			m_first = new elem_t(value);
			m_first->m_next = nullptr;
		}
		return m_first;
	}

	/*
	Find this iterator and remove it from list. Returns the new iterator at this position.
	*/
	elem_t* Remove(elem_t* it)
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		elem_t* return_value = nullptr;
		while (iterator != nullptr)
		{
			if (iterator == it)
			{
				return_value = iterator->m_next;
				if (prev == nullptr)
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
	void Remove(inner_type const & value)
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		while (iterator != nullptr)
		{
			if (iterator->m_value == value)
			{
				elem_t* save = iterator->m_next;
				if (prev == nullptr)
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
	elem_t* const Find(inner_type const & value) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator->m_value == value)
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}
};

#endif

