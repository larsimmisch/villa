// collect\list.h

#if !defined(INCL_COLLECT_LIST)
#define INCL_COLLECT_LIST

/* In hindsight (about ten years later), this is an example
   of bad OO design. I thought it was clever at the time */

class List
{
public:

	struct Link
	{
		Link() : next(0) {}

		Link* next;

		void  addAfter(Link* aNewLink);
		Link* remove();
		Link* removeNext();
		Link* replaceAfter(Link* aNewLink);
	};

	struct LinkIter
	{
		LinkIter(Link* aHead) : head(aHead), currentLink(aHead), previousLink(0), nextLink(0) { if (head) nextLink = head->next; }
		~LinkIter() {}

		Link* next();
		Link* current() 	{ return currentLink; }
		Link* previous()	{ return previousLink; }
		int   isDone()		{ return currentLink ? 0 : 1; }
		Link* back();

		void  reset()		{ currentLink = head; if (head) nextLink = head->next; previousLink = 0; }
		void  setToEnd();
 
		Link* currentLink;
		Link* previousLink;
		Link* nextLink;
		Link* head;
	};

public: //	external protocol

	List() : head(0), tail(0), size(0) {}
	virtual ~List() {}					// it is subclass responsibility to destroy the list

	// assert:
	//	aNewLink != 0 and is not in list!
	//	aLink is in list!
	//	anIndex is >= 0 and < size for removeAt() and <= size for addAt()!

	void addFirst(Link* aNewLink) ;
	void addLast (Link* aNewLink) ;
	void addAt	 (unsigned anIndex, Link* aNewLink) ;

	virtual void addAfter(Link* aLink, Link* aNewLink) ;

	Link* removeFirst() ;
	Link* removeLast () ;
	Link* removeAt	 (unsigned anIndex) ;
	Link* removeAfter(Link* aLink) ;

	unsigned getSize()	   { return size; }
	Link* getHead()   { return head; }
	Link* getTail()   { return tail; }

	void empty();

protected:	//	internal protocol

	//	this method is called when aLink is discarded, e.g. on destruction,
	//	as the list does not know the size of the link, subclasses must supply
	//	this method.

	virtual void  freeLink(Link* aLink) = 0;

	virtual void  addLinkAfter(Link* first, Link* second);
	virtual Link* removeLink(Link* aLink);
	virtual Link* removeNextLink(Link* aLink);

	Link* head;
	Link* tail;
	unsigned   size;

	friend class ListIter;
};

class ListIter
{
	public:
 
	ListIter(List& aList) : list(aList), iter(aList.head) {}
	~ListIter() {}
 
	List::Link* next()		 { return iter.next(); }
	List::Link* current()	 { return iter.current(); }
	List::Link* previous()	 { return iter.previous(); }
	int 		isDone()	 { return iter.isDone(); }
 
	List::Link* removeCurrent()
	{
		return iter.previous() ? list.removeAfter(iter.previous()) : list.removeFirst();
	}
 
	protected:
 
	List&			list;
	List::LinkIter	iter;
};

inline void List::addLinkAfter(Link* first, Link* second)
{
	first->addAfter(second);
}

inline List::Link* List::removeLink(Link* aLink)
{
	return aLink->remove(); //	return next link
}

inline List::Link* List::removeNextLink(Link* aLink)
{
	return aLink->removeNext();
}


class DList : public List
{
public:

	struct DLink : public List::Link
	{
		DLink() : Link(), prev(0) {}

		DLink* prev;

		void	dAddAfter(DLink* aNewLink);
		void	dAddBefore(DLink* aNewLink);
		DLink*	dRemove();
		DLink*	dRemoveNext();
		DLink*	dRemovePrevious();
		DLink*	dReplaceAfter(DLink* aNewLink);
	};

	struct DLinkIter
	{
		DLinkIter(DLink* head) : currentLink(head) {}
		~DLinkIter() {}

		DLink* next()		{ return currentLink ? currentLink = (DLink*)currentLink->next : 0; }
		DLink* prev()		{ return currentLink ? currentLink = currentLink->prev : 0; }
		DLink* current()	{ return currentLink; }
		DLink* previous()	{ return currentLink ? currentLink->prev : 0; }
		int    isDone() 	{ return currentLink ? 1 : 0; }

		DLink* currentLink;
	};

public: //	external protocol

	// assert:
	//	aNewLink != 0 and is not in list!
	//	aLink is in list!

	void   addBefore(DLink* aLink, DLink* aNewLink);
	void   addFirst(DLink* aLink);
	virtual void   addAfter(DLink* aLink, DLink* aNewLink) { aNewLink->prev = 0; List::addAfter(aLink, aNewLink); }
	DLink* removeBefore(DLink* aLink);
	DLink* remove(DLink* aLink) 
	{ if (aLink->prev == 0) return (DLink*)removeFirst(); else return (DLink*)removeAfter(aLink->prev); }

protected:

	DList() : List() {}
	virtual ~DList() {}

	virtual void  addLinkAfter(Link* first, Link* second);	//	read DLink
	virtual void  addLinkBefore(DLink* first, DLink* second);
	virtual Link* removeLink(Link* aLink); //  read DLink
	virtual Link* removeNextLink(Link* aLink); //  read DLink
	virtual DLink* removePreviousLink(DLink* aLink);
};

inline void DList::addLinkAfter(Link* first, Link* second)
{
	((DLink*)first)->dAddAfter((DLink*)second);
}

inline void DList::addLinkBefore(DLink* first, DLink* second)
{
	first->dAddBefore(second);
}

inline List::Link* DList::removeLink(Link* aLink)
{
	return (Link*)((DLink*)aLink)->dRemove();
}

inline List::Link* DList::removeNextLink(Link* aLink)
{
	return (Link*)((DLink*)aLink)->dRemoveNext();
}

inline DList::DLink* DList::removePreviousLink(DLink* aLink)
{
	return aLink->dRemovePrevious();
}

#endif
