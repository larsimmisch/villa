//  collect\list.h

#include <string.h>
#include "list.h"

void List::Link::addAfter(Link* aNewLink)
{
    if( aNewLink ) aNewLink->next = next;

    next = aNewLink;
}

List::Link* List::Link::remove()
{
    return next;
}

List::Link* List::Link::removeNext()
{
    Link* r = next;

    if( r ) next = r->next;

    return r;
}

List::Link* List::Link::replaceAfter(Link* aNewLink)
{
    Link* old = removeNext();

    addAfter(aNewLink);
 
    return old;
}

List::Link* List::LinkIter::next()
{
    if( currentLink )
    {
        previousLink = currentLink;
        currentLink  = nextLink;

		if( nextLink ) nextLink = nextLink->next;
    }
 
    return currentLink;
}

List::Link* List::LinkIter::back()
{
    if (previousLink == 0)  return 0;

    nextLink = currentLink;
    currentLink = previousLink;

    for (previousLink = head; previousLink->next != currentLink; previousLink = previousLink->next);

    return currentLink;
}

void List::LinkIter::setToEnd()
{
    if (head == 0)  return;

    if (head->next == 0)
    {
        previousLink = 0;
        currentLink = head;
        nextLink = 0;
 
        return;
    }

    nextLink = 0;

    for (previousLink = head; previousLink->next && previousLink->next->next ; previousLink = previousLink->next);
}

void List::addFirst(Link* aNewLink)
{
    if(!tail) tail = aNewLink;

    aNewLink->next = head;

    head = aNewLink;
    size++;
}

void List::addLast(Link* aNewLink)
{
    if(tail)
    {
        addLinkAfter(tail, aNewLink);
    }
    else
    {
        head = aNewLink;
    }

    tail = aNewLink;
    tail->next = 0;
    size++;
}

void List::addAt(unsigned anIndex, Link* aNewLink)
{
    if	(anIndex == 0)			addFirst(aNewLink);
    else if	(anIndex == size)	addLast(aNewLink);
    else
    {
		LinkIter l(head); 
        for( unsigned i = 0; i < anIndex; i++) l.next();

        addLinkAfter(l.previous(),aNewLink);

        size++;
    }
}

void List::addAfter(Link* aLink, Link* aNewLink)
{
    addLinkAfter(aLink,aNewLink);

    if(aLink == tail)
    {
        tail = aNewLink;
    }

    size++;
}

List::Link* List::removeFirst()
{
    Link* r = head;

    if(r) 
    {
        head = removeLink(head);
        size--;
    }
    
    if(!head) 
		tail = 0;

    return r;
}

List::Link* List::removeAt(unsigned anIndex)
{           
    Link* r = 0;

    if(anIndex == 0)
    {
        r = removeFirst();
    }
    else if(anIndex < size)
    {                                                          
		LinkIter l(head);
        for( unsigned i = 0; i < anIndex; i++) l.next();

        r = removeNextLink(l.previous());
 
        if( tail == r ) tail = l.previous();

        size--;
    }

    return r;
}

List::Link* List::removeLast()
{
    return removeAt(size - 1);
}

List::Link* List::removeAfter(Link* aLink)
{
    Link* r = removeNextLink(aLink);

    if(tail == r) tail = aLink;

	if (r)
		--size;

    return r;
}

void List::empty()
{
    for( LinkIter i(head); !i.isDone(); i.next() ) freeLink( i.current() );

	size = 0;
	head = tail = 0;
}

void DList::addFirst(DLink* aNewLink)
{
    if(!tail) tail = aNewLink;
 
    aNewLink->prev = 0;
    aNewLink->next = head;
    if (aNewLink->next) ((DList::DLink*)aNewLink->next)->prev = aNewLink;

    head = aNewLink;
    size++;
}

void DList::DLink::dAddAfter(DLink* aNewLink)
{
    aNewLink->prev = this;
    aNewLink->next = next;

    if( next ) ((DLink*)next)->prev = aNewLink;

    next = aNewLink;
}

void DList::DLink::dAddBefore(DLink* aNewLink)
{
    aNewLink->prev = prev;
    aNewLink->next = this;

    if(prev) prev->next = this;

    prev = aNewLink;
}

DList::DLink* DList::DLink::dRemove()
{
    if (prev) prev->next = next;
    if (next) ((DLink*)next)->prev = prev;

    return (DLink*)next;
}

DList::DLink* DList::DLink::dRemoveNext()
{
    DLink* r = (DLink*)next;

    if (next) ((DLink*)next)->dRemove();

    return r;
}

DList::DLink* DList::DLink::dRemovePrevious()
{
    DLink* r = prev;

    if(prev) prev->dRemove();

    return r;
}

DList::DLink* DList::DLink::dReplaceAfter(DLink* aNewLink)
{
    DLink* r = dRemoveNext();

    dAddAfter(aNewLink);

    return r;
}

void DList::addBefore(DLink* aLink, DLink* aNewLink)
{
    addLinkBefore(aLink,aNewLink);

    if(aLink == head)
    {
        head = aNewLink;
    }

    size++;
}

DList::DLink* DList::removeBefore(DLink* aLink)
{
    DLink* r = removePreviousLink(aLink);

    if(head == r) 
    {
        aLink->prev = 0;
        head = aLink;
    }

    return r;
}

