#ifndef TEMP_BASICSTRING
#define TEMP_BASICSTRING

#include <new>
#include <string.h>
#include <limits>

#include "Container/utlvector.h"

template <typename pod = char>
class String
{
public:
	constexpr static size_t const npos = std::numeric_limits<size_t>::max();

private:

	/*  -need- always contains the zero char. */
	void Grow(size_t need, bool copy = true)
	{
		if (need <= m_capacity)
			return;

		size_t new_capacity = 32;
		while (new_capacity < need) new_capacity <<= 1;

		pod *n = new pod[new_capacity];
		
		if (m_alloc)
		{
			if(copy) memcpy(n, m_alloc, m_size * sizeof(pod));
			delete[] m_alloc;
		}
		else
		{
			n[0] = '\0';
			m_size = 0;
		}

		m_alloc = n;
		m_capacity = new_capacity;
	}

	pod * m_alloc;
	size_t m_size;
	size_t m_capacity;

public:

	String()
	{
		memset(this, 0, sizeof(String));
	}

	~String()
	{
		if (m_alloc)
			delete[] m_alloc;
#ifdef DEBUG
		memset(this, 0xCCCCCCCC, sizeof(String));
#endif
	}

	const pod  *c_str() const
	{
		return m_alloc ? m_alloc : "";
	}

	void assign(const String<pod> &src)
	{
		assign(src.c_str());
	}

	void clear()
	{
		if (m_alloc)
			m_alloc[0] = '\0';
		m_size = 0;
	}

	void assign(pod const *d)
	{
		if (!d)
		{
			clear();
		}
		else
		{
			size_t const len = strlen(d) + 1;
			Grow(len, false);
			memcpy(m_alloc, d, len * sizeof(pod));
			m_size = strlen(m_alloc);
		}
	}

	void assign(pod const *d, size_t count)
	{
		if (!d)
		{
			clear();
		}
		else
		{
			size_t const len = strnlen(d, count) + 1;
			Grow(len, false);
			memcpy(m_alloc, d, len * sizeof(pod));
			m_alloc[len] = '\0';
			m_size = strlen(m_alloc);
		}
	}

	String(pod const *src) : String()
	{
		assign(src);
	}

	String(pod const *src, size_t start, size_t count = std::numeric_limits<size_t>::max()) : String()
	{
		assign(src+start, count);
	}

	String(String<pod> const &src) : String()
	{
		assign(src.c_str());
	}

	String(String<pod> const &src, size_t start, size_t count = std::numeric_limits<size_t>::max()) : String()
	{
		assign(src.c_str()+start, count);
	}

	String & operator = (String<pod> const &src)
	{
		assign(src);
		return *this;
	}

	String & operator = (pod const *src)
	{
		assign(src);
		return *this;
	}

	bool operator ==(const String<pod> &other) const
	{
		if (m_size != other.m_size) return false;

		pod const * me = m_alloc;
		pod const * other_c = other.m_alloc;
		do
		{
			if (*me != *other_c) return false;
			++other_c;
		} while (*me++ != '\0');

		return true;
	}

	inline bool operator !=(const String<pod> &other) const
	{
		return !this->operator==(other);
	}

	void reserve(size_t len)
	{
		Grow(len+1);
	}

	bool operator ==(pod const * other) const
	{
		if (m_size != strlen(other)) return false;

		pod const * me = m_alloc;
		do
		{
			if (*me != *other) return false;
			++other;
		} while (*me++ != '\0');

		return true;
	}

	inline bool operator !=(pod const * other) const
	{
		return !this->operator==(other);
	}

	String<pod>& append(pod const * t)
	{
		size_t const len = strlen(t) + 1;
		Grow(m_size + len);
		memcpy(m_alloc + m_size, t, sizeof(pod) * len);
		m_size = strlen(m_alloc);
		return *this;
	}

	String<pod>& append(pod const c)
	{
		size_t pos = m_size;
		Grow(pos + 2); // should be + 1 but when m_alloc is not allocated we also need to add '\0'
		m_alloc[pos] = c;
		++pos;
		m_alloc[pos] = '\0';
		++m_size;
		return *this;
	}

	String<pod>& append(String<pod> const & d)
	{
		append(d.c_str());
		return *this;
	}

	bool isempty() const
	{
		return m_size == 0;
	}

	size_t size() const
	{
		return m_size;
	}

	size_t length() const
	{
		return m_size;
	}

	size_t find(pod const c, size_t start = 0) const
	{
		size_t a = m_size;
		if (a == 0) return npos;
		if (start >= a) return npos;
		a = 0;
		pod const * me = m_alloc + start;
		do
		{
			if (*me == c) return a;
			++a;
		} while (*(++me) != '\0');

		return npos;
	}

	size_t find(pod const * const c, size_t start = 0) const
	{
		size_t const csize = strlen(c);
		if (start + csize > m_size) return npos;
		pod * me = m_alloc + start;

		while (memcmp(me, c, csize * sizeof(pod)) != 0)
		{
			++me;
			if (++start + csize > m_size) return npos;
		}

		return start;
	}

	size_t find(String<pod> const & c, size_t start = 0) const
	{
		return find(c.c_str());
	}

	size_t find_last_of(pod const * c, size_t start = npos) const
	{
		String const temp(c);

		size_t a = m_size;
		if (a == 0) return npos;
		if (start == 0) return npos;

		pod const * me = m_alloc + a - 1;
		do
		{
			if (temp.find(*me) != npos) return a;
			--a;
		} while (me-- != m_alloc);

		return npos;
	}

	size_t find_last_of(pod const c, size_t start = npos) const
	{
		size_t a = m_size;
		if (a == 0) return npos;
		if (start == 0) return npos;

		pod const * me = m_alloc + a - 1;
		do
		{
			if (*me == c) return a;
			--a;
		} while (me-- != m_alloc);

		return npos;
	}

	void lower()
	{
		if (!m_alloc)
			return;
		pod * me = m_alloc;
		while (*me != '\0')
		{
			if (*me >= 'A' && *me <= 'Z') 
				*me += 0x20;
			++me;
		} 
	}

	pod& operator[] (size_t index) const
	{
		return m_alloc[index];
	}
};

typedef String<char> basic_string;
typedef String<wchar_t> basic_wstring;

template <typename pod>
void SplitString(String<pod> const & string, pod const delim, CUtlVector < String<pod> > & out)
{
	out.RemoveAll();
	out.EnsureCapacity(8);

	size_t start = 0;
	size_t end;
	for(;;)
	{
		end = string.find(delim, start);
		if (end == String<pod>::npos)
		{
			out.AddToTail(String<pod>(name, start));
			break;
		}
		else
		{
			out.AddToTail(String<pod>(name, start, end - 1));
		}
		start = end+1;
	}

}

#endif

