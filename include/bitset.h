
#ifndef __BITSET_H__
#define __BITSET_H__

#pragma warning(push)
#pragma warning(disable : 4511 4146 4244)
#include <ostream>
#pragma warning(pop)

template<unsigned size> class bitset
{
public:

	enum { not_set = 0xffffffff };
	
	bitset()
	{
		memset(m_Data, 0, sizeof(m_Data));
	}

	bitset(const bitset& b)
	{
		memcpy(m_Data, b.m_Data, sizeof(m_Data));
	}
	
	~bitset() {}

	bitset& operator=(const bitset& b)
	{
		memcpy(m_Data, b.m_Data, sizeof(m_Data));

		return *this;
	}
	
	unsigned bit_at(unsigned pos) const
	{		
		unsigned value = m_Data[pos / (8 * sizeof(int))];
		
		unsigned bit = 1 << (pos % (8 * sizeof(int)));
		
		return (bit & value) ? 1 : 0;
	}
	
	void set_bit(unsigned pos, bool b)
	{
		unsigned bit = 1 << pos % (8 * sizeof(int));
		
		if (b)	
			m_Data[pos / (8 * sizeof(int))] |= bit;
		else 
			m_Data[pos / (8 * sizeof(int))] &= ~bit;
	}	
	
	void set_bits(bool b, unsigned start = 0, unsigned end = size -1)
	{
		for (unsigned i = 0; i <= end; ++i)
		{
			unsigned bit = 1 << i % (8 * sizeof(int));
			
			if (b)	
				m_Data[i / (8 * sizeof(int))] |= bit;
			else 
				m_Data[i / (8 * sizeof(int))] &= ~bit;
		}
	}	

	unsigned highest_bit(unsigned start = 0, unsigned end = size -1) const
	{
		for (unsigned index = end; index != start -1; --index)
		{
			if (bit_at(index))	
				return index;
		}
		
		return (unsigned)not_set;
	}

	unsigned lowest_bit(unsigned start = 0, unsigned end = size -1) const
	{
		for (unsigned index = start; index <= end; index++)
		{
			if (bit_at(index))
				return index;
		}
		
		return (unsigned)not_set;
	}

	bitset& operator|=(bitset& b)
	{
		int l = size_in_ints();

		for (int i = 0; i < l; ++i)
			m_Data[i] |= b.m_Data[i];

		return *this;
	}

	bitset& operator&=(bitset& b)
	{
		int l = size_in_ints();

		for (unsigned i = 0; i < l; ++i)
			m_Data[i] &= b.m_Data[i];

		return *this;
	}

	bitset& operator^=(bitset& b)
	{
		int l = size_in_ints();

		for (unsigned i = 0; i < l; ++i)
			m_Data[i] ^= b.m_Data[i];

		return *this;
	}

	static unsigned size_in_ints() 
	{ 
		return size / (8 * sizeof(int)) + ((size % (8 * sizeof(int))) ? 1 : 0);
	}

protected:
	
	unsigned m_Data[size / (8 * sizeof(int)) + ((size % (8 * sizeof(int))) ? 1 : 0)];
};

template<unsigned s, class ch, class tr>
std::basic_ostream<ch,tr>& operator<<(std::basic_ostream<ch,tr>& out, const bitset<s>& b)
{
	// print the msb left
	for (unsigned index = s; index >= 0; --index)
	{
		if (b.bit_at(index))
			out << '1';
		else 
			out << '0';
	}
	
	return out;
}

#endif

