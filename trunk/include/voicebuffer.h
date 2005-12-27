// ringbuffer.h
//
// $Revision: 1.2 $
// $Author: claudia $
//
// $Date: 2003/01/28 11:22:03 $

#ifndef __VOICEBUFFER_H__
#define __VOICEBUFFER_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <vector>

// Use:
// ringbuffer r;
// r.push(); r.back() = new_element;
// oldest_element = r.front(); r.pop();

#define VBUF_DATASIZE kSMMaxReplayDataBufferSize

struct vbuf
{
    char data[VBUF_DATASIZE];
    int size;
};

class voicebuffer
{
public:

	voicebuffer(size_t max_size) :
		m_buffer(new vbuf[max_size]),
		m_max_size(max_size)
	{ clear(); }

	~voicebuffer()
	{ delete[] m_buffer; }

	size_t size()     const		{ return m_size; }
	size_t max_size() const		{ return m_max_size; }

	bool empty() const	{ return m_size == 0; }
	bool full()  const	{ return m_size == m_max_size; }

	vbuf& front() const	{ return m_buffer[m_front]; }
	vbuf& back()  const	{ return m_buffer[m_back]; }

	void clear() {
		m_size = 0;
		m_front = 0;
		m_back  = m_max_size - 1;

        for (int i = 0; i < m_max_size; ++i)
        {
            m_buffer[i].size = 0;
        }
	}

	void push()	{
		inc(m_back);
		if( size() == max_size() )
			inc(m_front);
		else
			m_size++;
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

	size_t _inc(size_t x) const{
		if( ++x == m_max_size )
			return 0;
		else 
			return x;
	}

	size_t _dec(size_t x) const {
		if( x == 0 )
			return m_max_size - 1 ;
		else
			return x - 1;
	}

	void inc(size_t& x) const {
		if( ++x == m_max_size )
			x = 0;
	}

	void dec(size_t& x) const {
		if( x == 0 )
			x = m_max_size;
		x--;
	}

	vbuf *m_buffer;

	size_t m_size;
	size_t m_max_size;

	size_t m_front;
	size_t m_back;
};


#endif // __VOICEBUFFER_H__
