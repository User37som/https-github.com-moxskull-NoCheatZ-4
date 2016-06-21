#ifndef TEMP_BASICSTRING
#define TEMP_BASICSTRING

#include <new>
#include <string.h>
#include <limits>

#include "Containers/utlvector.h"

#define STRING_POOL_SIZE 128
#define AVERAGE_STRING_SIZE 32

/*
Use a pool of pointers to prevent re-allocations.
*/

template <typename pod>
struct ALIGN4 memory_info
{
	size_t m_capacity;
	pod * m_ptr;
	bool m_in_use;

	memory_info() : m_capacity(0), m_ptr(nullptr), m_in_use(false)
	{
	}

	memory_info(memory_info const & other) = delete;
	memory_info & operator= (memory_info const & other) = delete;

} ALIGN4_POST;

template <typename pod>
class string_memory_pool
{
	
private:
	memory_info<pod> m_alloc_pool[STRING_POOL_SIZE]; // list of free memory actually not removed from heap
	size_t m_pool_elements;

public:

	string_memory_pool() : m_alloc_pool(), m_pool_elements(0)
	{
		memset(m_alloc_pool, 0, sizeof(memory_info<pod>) * STRING_POOL_SIZE);
	}

	~string_memory_pool()
	{
		size_t index = 0;
		do
		{
			if (m_alloc_pool[index].m_in_use)
			{
				delete[] m_alloc_pool[index].m_ptr;
			}
		} while (++index < STRING_POOL_SIZE);
	}

public:
	// True if the pool can store a free pointer, so the string must call DeclareFreeMemory instead of delete[]
	inline bool StringShouldNotDealloc() const;

	// Add a free pointer to the pool.
	void DeclareFreeMemory(pod * ptr, size_t capacity);

	// Re-use a free pointer if we have capacity, otherwise return nullptr. The pool will delete element if there is a match.
	pod * GetFreeMemory(size_t target_min_capacity);
};

template <typename pod = char>
class String
{
	friend String<char>;
	friend String<wchar_t>;

public:
	constexpr static size_t const npos = std::numeric_limits<size_t>::max();

private:

	pod * m_alloc;
	size_t m_size;
	size_t m_capacity;

private:
	static string_memory_pool<pod> m_pool;

private:

	inline pod * Alloc(size_t capacity)
	{
		pod * t = m_pool.GetFreeMemory(capacity);
		if(t == nullptr) return new pod[capacity];
		else return t;
	}

	inline void Dealloc()
	{
		if (m_alloc)
		{
			if (m_pool.StringShouldNotDealloc())
			{
				m_pool.DeclareFreeMemory(m_alloc, m_capacity);
			}
			else
			{
				delete[] m_alloc;
			}
			m_alloc = nullptr;
		}
	}

	/*  -need- always contains the zero char. */
	void Grow(size_t need, bool copy = true)
	{
		if (need <= m_capacity)
			return;

		size_t new_capacity = AVERAGE_STRING_SIZE;
		while (new_capacity < need) new_capacity <<= 1;

		pod * n = Alloc(new_capacity);
		
		if (m_alloc)
		{
			if(copy) memcpy(n, m_alloc, m_size * sizeof(pod));
			Dealloc();
		}
		else
		{
			n[0] = 0;
			m_size = 0;
		}

		m_alloc = n;
		m_capacity = new_capacity;
	}

	inline size_t autolen(pod const * string) const
	{
		return strlen(string);
	}

	inline size_t autonlen(pod const * string, size_t max) const
	{
		return strnlen(string, max);
	}

	inline pod const * autoempty() const
	{
		return "";
	}

public:

	String()
	{
		memset(this, 0, sizeof(String));
	}

	~String()
	{
		Dealloc();
#ifdef DEBUG
		memset(this, 0xCCCCCCCC, sizeof(String));
#endif
	}

	const pod *c_str() const
	{
		return m_alloc ? m_alloc : autoempty();
	}

	void assign(const String<pod> &src)
	{
		assign(src.c_str());
	}

	void clear()
	{
		if (m_alloc)
			m_alloc[0] = 0;
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
			size_t const len = autolen(d) + 1;
			Grow(len, false);
			memcpy(m_alloc, d, len * sizeof(pod));
			m_size = autolen(m_alloc);
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
			size_t const len = autonlen(d, count) + 1;
			Grow(len, false);
			memcpy(m_alloc, d, len * sizeof(pod));
			m_alloc[len] = 0;
			m_size = autolen(m_alloc);
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

	String(String<pod> && src) : String()
	{
		memcpy(this, &src, sizeof(String<pod>));
		memset(&src, 0, sizeof(String<pod>));
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

	String & operator = (String<pod> && src)
	{
		if (this != &src)
		{
			Dealloc();
			memcpy(this, &src, sizeof(String<pod>));
			memset(&src, 0, sizeof(String<pod>));
		}
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
		} while (*me++ != 0);

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
		if (m_size != autolen(other)) return false;

		pod const * me = m_alloc;
		do
		{
			if (*me != *other) return false;
			++other;
		} while (*me++ != 0);

		return true;
	}

	inline bool operator !=(pod const * other) const
	{
		return !this->operator==(other);
	}

	String<pod>& append(pod const * t)
	{
		size_t const len = autolen(t) + 1;
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
		m_alloc[pos] = 0;
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

	String<pod>& replace(pod const replace_this, pod const replace_by)
	{
		if (m_size > 0)
		{
			pod * me = m_alloc;
			do
			{
				if (*me == replace_this) *me = replace_by;
			} while (*++me != 0);
		}

		return *this;
	}

	String<pod>& replace(pod const * replace_list, pod const replace_by)
	{
		if (m_size > 0)
		{
			while (*replace_list != 0)
			{
				replace(*replace_list, replace_by);
				++replace_list;
			}
		}

		return *this;
	}

	String<pod>& remove(size_t pos)
	{
		if (pos < m_size)
		{
			do
			{
				m_alloc[pos] = m_alloc[pos + 1];
			} while (m_alloc[++pos] != 0);
			--m_size;
		}
		return *this;
	}

	String<pod>& remove(size_t const start, size_t end)
	{
		if (start > end) return remove(end, start);
		if (end < m_size)
		{
			do
			{
				remove(end);
			} while (end-- - start != 0);
		}
		return *this;
	}

	String<pod>& replace(String<pod> const & replace_this, String<pod> const & replace_by)
	{
		int const diff = replace_by.m_size - replace_this.m_size;
		size_t pos = find(replace_this);
		while (pos != npos)
		{
			if (diff <= 0)
			{
				memcpy(m_alloc + pos, replace_by.m_alloc, sizeof(pod) * replace_by.m_size - 1);
				if(diff < 0) remove(pos + replace_by.m_size, pos + replace_this.m_size - 1);
			}
			else if (diff > 0)
			{
				memcpy(m_alloc + pos, replace_by.m_alloc, sizeof(pod) * replace_this.m_size - 1);
				Grow(m_size + diff + 1);
				size_t move_from_here = m_size + diff;
				size_t const move_until_here = pos + replace_this.m_size - 1;
				do
				{
					m_alloc[move_from_here] = m_alloc[move_from_here - diff];
				} while (--move_from_here != move_until_here);
				memcpy(m_alloc + pos + replace_this.m_size, replace_by.m_alloc + replace_this.m_size, sizeof(pod) * diff);
				m_size += diff;
			}
			pos = find(replace_this, pos);
		}
		return *this;
	}

	size_t find(pod const c, size_t start = 0) const
	{
		size_t a = m_size;
		if (a == 0) return npos;
		if (start >= a) return npos;
		a = start;
		pod const * me = m_alloc + start;
		do
		{
			if (*me == c) return a;
			++a;
		} while (*(++me) != 0);

		return npos;
	}

	size_t find(pod const * const c, size_t start = 0) const
	{
		size_t const csize = autolen(c);
		if (start + csize > m_size) return npos;
		char * me = m_alloc + start;

		while (memcmp(me, c, csize * sizeof(char)) != 0)
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
		while (*me != 0)
		{
			if (isupper(*me)) 
				*me = (pod)tolower(*me);
			++me;
		} 
	}

	pod& operator[] (size_t index) const
	{
		return m_alloc[index];
	}

	static void ConvertToWideChar(String<char> const & in, String<wchar_t> & out)
	{
		out.Grow(in.m_size, false);
		size_t const cpsize = mbstowcs(out.m_alloc, in.m_alloc, out.m_capacity);
		Assert(cpsize < std::numeric_limits<size_t>::max());
		out.m_size = cpsize;
	}
};

template <>
inline size_t String<wchar_t>::autolen(wchar_t const * string) const
{
	return wcslen(string);
}

template <>
inline size_t String<wchar_t>::autonlen(wchar_t const * string, size_t max) const
{
	return wcsnlen(string, max);
}

template <>
inline wchar_t const * String<wchar_t>::autoempty() const
{
	return L"";
}

typedef String<char> basic_string;
typedef String<wchar_t> basic_wstring;

template <typename pod>
inline bool string_memory_pool<pod>::StringShouldNotDealloc() const
{
	return m_pool_elements != STRING_POOL_SIZE;
}

template <typename pod>
void string_memory_pool<pod>::DeclareFreeMemory(pod * ptr, size_t capacity)
{
	size_t index = 0;
	do
	{
		if (!m_alloc_pool[index].m_in_use)
		{
			m_alloc_pool[index].m_capacity = capacity;
			m_alloc_pool[index].m_ptr = ptr;
			m_alloc_pool[index].m_in_use = true;
			++m_pool_elements;
			return;
		}
	} while (++index < STRING_POOL_SIZE);
}

template <typename pod>
pod * string_memory_pool<pod>::GetFreeMemory(size_t target_min_capacity)
{
	if (m_pool_elements > 0)
	{
		size_t index = 0;
		do
		{
			if (m_alloc_pool[index].m_in_use && m_alloc_pool[index].m_capacity >= target_min_capacity)
			{
				m_alloc_pool[index].m_in_use = false;
				--m_pool_elements;
				return m_alloc_pool[index].m_ptr;
			}
		} while (++index < STRING_POOL_SIZE);
	}

	return nullptr;
}

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
			out.AddToTail(String<pod>(string, start));
			break;
		}
		else
		{
			out.AddToTail(String<pod>(string, start, end - 1 - start));
		}
		start = end+1;
	}

}

#endif

