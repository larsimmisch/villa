// ringbuffer.h
//
// $Revision: 1.2 $
// $Author: claudia $
//
// $Date: 2003/01/28 11:22:03 $

#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <vector>

// Use:
// ringbuffer r;
// r.push(); r.back() = new_element;
// oldest_element = r.front(); r.pop();

template<class T>
class ringbuffer
{
public:
	typedef T value_type;
	typedef unsigned int size_type;

	ringbuffer(size_type max_size) :
		m_buffer(new value_type[max_size]),
		m_max_size(max_size)
	{ clear(); }

	~ringbuffer()
	{ delete[] m_buffer; }

	size_t size()     const		{ return m_size; }
	size_t max_size() const		{ return m_max_size; }

	bool empty() const	{ return m_size == 0; }
	bool full()  const	{ return m_size == m_max_size; }

	value_type& front() const	{ return m_buffer[m_front]; }
	value_type& back()  const	{ return m_buffer[m_back]; }

	void clear() {
		m_size = 0;
		m_front = 0;
		m_back  = m_max_size - 1;
	}

	void push()	{
		inc(m_back);
		if( size() == max_size() )
			inc(m_front);
		else
			m_size++;
	}

	void push(const value_type& x) {
		push();
		back() = x;
	}

	void pop() {
		if( m_size > 0  ) {
			m_size--;
			inc(m_front);
		}
	}

	void back_erase(const size_t n) {
		if( n >= m_size )
			clear();
		else {
			m_size -= n;
			m_back = (m_front + m_size - 1) % m_max_size;
		}
	}

	void front_erase(const size_t n) {
		if( n >= m_size )
			clear();
		else {
			m_size -= n;
			m_front = (m_front + n) % m_max_size;
		}
	}


protected:

	size_type _inc(size_type x) const{
		if( ++x == m_max_size )
			return 0;
		else 
			return x;
	}

	size_type _dec(size_type x) const {
		if( x == 0 )
			return m_max_size - 1 ;
		else
			return x - 1;
	}

	void inc(size_type& x) const {
		if( ++x == m_max_size )
			x = 0;
	}

	void dec(size_type& x) const {
		if( x == 0 )
			x = m_max_size;
		x--;
	}

	value_type*	m_buffer;

	size_type		m_size;
	size_type		m_max_size;

	size_type		m_front;
	size_type		m_back;

	class general_iterator;
	friend class general_iterator;

	class general_iterator
	{
	public:

		general_iterator(): m_base(NULL) {}

		general_iterator(const ringbuffer* base, size_type position, bool begin = false) :
		m_base(base),
		m_position(position),
		m_begin(begin)
		{}

		bool operator==(const general_iterator& a) const
		{ return m_position == a.m_position && m_begin == a.m_begin && m_base == a.m_base; }
		bool operator!=(const general_iterator& a) const
		{ return !(*this == a); }

		value_type& operator*() const
		{ return m_base->m_buffer[m_position]; }

	protected:

		const ringbuffer* m_base;
		size_type		m_position;
		bool			m_begin;
		// If the buffer is full, iterators begin() and end() 
		// (or rbegin() and rend()) point to the same position in
		// the buffer. m_begin is used to distinguish between them.
	};

public:

	class iterator;
	friend class iterator;

	class iterator : public general_iterator
	{
	public:

		iterator() : general_iterator() {}
		
		iterator(const ringbuffer* base, size_type position, bool begin = false) :
			general_iterator(base, position, begin) {}

		iterator& operator++() {	
			m_base->inc(m_position);			
			m_begin = false;
			return (*this); 
		}
		iterator& operator++(int){
			iterator _Tmp = *this;
			m_base->inc(m_position);			
			m_begin = false;
			return (_Tmp); 
		}
	};

	class reverse_iterator;
	friend class reverse_iterator;

	class reverse_iterator : public general_iterator
	{
	public:

		reverse_iterator() : general_iterator() {}
		
		reverse_iterator(const ringbuffer* base, size_type position, bool begin = false) :
		general_iterator(base, position, begin) {}

		reverse_iterator& operator++() {	
			m_base->dec(m_position);			
			m_begin = false;
			return (*this); 
		}
		reverse_iterator& operator++(int){
			reverse_iterator _Tmp = *this;
			m_base->dec(m_position);
			m_begin = false;
			return (_Tmp); 
		}
	};

	iterator begin()  const
	{ return iterator(this, m_front, m_size > 0); }
	iterator end()	  const
	{ return iterator(this, _inc(m_back)); }
	
	reverse_iterator rbegin() const
	{ return reverse_iterator(this, m_back, m_size > 0); }
	reverse_iterator rend()   const
	{ return reverse_iterator(this, _dec(m_front)); }
};


#endif // __RINGBUFFER_H__
