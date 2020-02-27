//  collect/set.cpp

#include <string.h>
#include "set.h"

#ifdef DEBUG
#include <iostream.h>
#endif

Set::AssocIter::AssocIter(Set& aSet) : set(aSet)
{
    reset();
}

Set::Assoc* Set::AssocIter::next()
{
    currentAssoc = nextAssoc;
    
    if(nextAssoc)
    {
        nextAssoc = (Assoc*)nextAssoc->next;

        if(!nextAssoc)
        {
            for(register unsigned i = nextIndex+1; i < set.size; i++)
            {
                nextAssoc = set.array[i];

                if(nextAssoc)
                {
                    nextIndex = i;

                    break;
                }
            }
        }
    }

    return currentAssoc;
}

Set::Assoc* Set::AssocIter::back()
{
    register int i;

    nextAssoc = currentAssoc;
    
    if (nextIndex > 0) for ( i = nextIndex -1; i >= 0 && !set.array[i]; i--);
    else i = 0;

    if (set.array[i] == currentAssoc) 
    {
        nextIndex = i;

        if (i == 0)  return (currentAssoc = 0);
 
        for (i-- ; i >= 0 && !set.array[i]; i--);

        for (currentAssoc = set.array[i]; currentAssoc && currentAssoc->next; currentAssoc = currentAssoc->next);

        return currentAssoc;
    }

    for (currentAssoc = set.array[i]; currentAssoc->next != nextAssoc; currentAssoc = currentAssoc->next);
 
    return currentAssoc;
}

void Set::AssocIter::setToEnd()
{
	register int i;

    for(i = set.size - 1; i >= 0 && !set.array[i]; i-- );

    if (i < 0)  return;

    for (currentAssoc = set.array[i]; currentAssoc->next; currentAssoc = currentAssoc->next);

    nextAssoc = 0;
    nextIndex = set.size;
}

void Set::AssocIter::reset()
{
	register unsigned i;

    for(i = 0; i < set.size; i++ )
    {
        if( currentAssoc = set.array[i] )
        {
            nextIndex = i;

            break;
        }
    }
    if ( currentAssoc && !(nextAssoc = currentAssoc->next) )
    {
        for(i = nextIndex+1; i < set.size; i++)
        {
            if ( nextAssoc = set.array[i] )
            {
                nextIndex = i;
                
                break;
            }
        }
    }
}

void Set::AssocIter::setTo(Assoc* anAssoc)
{
    // disgrace. we need to search for anAssoc
	register unsigned i;

    for (i = 0; i < set.size; i++)
    {
        if (set.array[i])
        {
            for (currentAssoc = set.array[i]; currentAssoc != anAssoc && currentAssoc != 0; currentAssoc = currentAssoc->next);
            if (currentAssoc)   break;
        }
    }
 
    // now find the next if there is a current

    if (currentAssoc)
    {
        if (currentAssoc->next) nextAssoc = currentAssoc->next;
        else
        {
            for (i++; i < set.size && !set.array[i]; i++);
            if (i < set.size)
            {
                nextAssoc = set.array[i];
                nextIndex = i;
            }
            else nextAssoc = 0;
        }
    }
}

unsigned Set::hashString(const char* key)
{
    register unsigned int h = 0;

    for( ; *key; key++ ) h = (64*h + *key);

    return h;
}

unsigned Set::hashInteger(int index)
{
    return index;
}


static const int primes[]   = { 7, 13, 23, 43, 89, 173, 349, 1423, 2843, 0 };
static const int primeCount = 9;

inline int findNextPrime(int aSize)
{
	int i;

    for(i = 0; i < primeCount; i++ )
    {
        if( aSize < primes[i] ) return primes[i];
    }

    throw "Ich platze...";

	// to disable warning
	return 0;
}

inline void Set::grow()
{
/*
    instance().fill++;

    if( instance().fill > instance().maxFill )
    {
        Assoc** anArray = instance().array;
        unsigned aSize  = instance().size;

        instance().size    = findNextPrime(instance().size);
        instance().array   = (Assoc**)allocate( sizeof(Assoc*) * instance().size );
        instance().fill    = 0;
        instance().maxFill = (int)( 0.8 * instance().size );

        memset( instance().array, 0, instance().size * sizeof(Assoc**) );

        #ifdef DEBUG
        cout << __FUNCTION__ << " from " << aSize << " to " << instance().size << "\n";
        #endif

        for( ; !i.isDone(); i.next() ) add(i.current());

        free( anArray, aSize * sizeof(Assoc*) );
    }
*/
}

Set::Set(int initialSize)
{
	size    = initialSize;
	fill    = 0;
	maxFill = (int)( 0.8 * initialSize );
	array   = (Assoc**)new char[sizeof(Assoc*) * initialSize];
 
	memset(array, 0, size * sizeof(Assoc*));
}

Set::~Set() //  only empty sets may be discarded
{
	// if (fill) throw "verboten"; //PleaseDoNotDeleteNonEmptyContainers(__FILE__,__LINE__,__FUNCTION__);

    delete array;
}

Set::Assoc* Set::basicAdd(Assoc* aNewAssoc)
{
    unsigned index = hashAssoc(aNewAssoc);    
    index %= size;
    Assoc* head  = array[index];

    #if 0
    cout << __FUNCTION__ << " hash: " << index << "\n";
    #endif

	fill++;

    aNewAssoc->next = 0;
    if (!head)
    {
        array[index] = aNewAssoc;

//        grow();

        return 0;
    }
    else if (isEqual(head,aNewAssoc))
    {
        Assoc* old = head;

        head = (Assoc*)head->remove();

        aNewAssoc->addAfter(head);

        array[index] = aNewAssoc;

        return old;
    }
    else if(head->next)   //  at least two elements, head already checked
    {
        Assoc* current;

        List::LinkIter i(head->next);
 
        for( i.previousLink = head; !i.isDone(); i.next() )
        {
            current = (Assoc*)i.current();

            if( isEqual(current,aNewAssoc) )
            {
                return (Assoc*)i.previous()->replaceAfter(aNewAssoc);
            }
        }
    }

    //  not found

    head->addAfter(aNewAssoc);

    // grow();

    return 0;
}

Set::Assoc* Set::basicRemoveAt(Assoc* anAssoc)
{
    unsigned index = hashAssoc(anAssoc) % size;
    Assoc* head  = array[index];

	fill--;

    if (head)
    {
		if (isEqual(head,anAssoc))
		{
			array[index] = (Assoc*)head->remove();

            return head;
        }
        else if(head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for( i.previousLink = head; !i.isDone(); i.next() )
            {
                current = (Assoc*)i.current();

                if( isEqual(current,anAssoc) )
                {
                    return (Assoc*)i.previous()->removeNext();
                }
            }
        }
    }

    return 0;
}

Set::Assoc* Set::basicRemoveAt(const char* aKey)
{
    unsigned index = hashString(aKey) % size;
    Assoc* head  = array[index];

	fill--;

    if (head)
    {
        if (hasKey(head,aKey))
        {
            array[index] = (Assoc*)head->remove();

            return head;
        }
        else if (head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for (i.previousLink = head; !i.isDone(); i.next())
            {
                current = (Assoc*)i.current();

                if(hasKey(current,aKey))
                {
                    return (Assoc*)i.previous()->removeNext();
                }
            }
        }
    }

    return 0;
}

Set::Assoc* Set::basicRemoveAt(int anIndex)
{
    unsigned index = hashInteger(anIndex) % size;
    Assoc* head  = array[index];

	fill--;

    if (head)
    {
        if (hasIndex(head,anIndex))
        {
            array[index] = (Assoc*)head->remove();

            return head;
        }
        else if(head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for( i.previousLink = head; !i.isDone(); i.next() )
            {
                current = (Assoc*)i.current();

                if( hasIndex(current,anIndex) )
                {
                    return (Assoc*)i.previous()->removeNext();
                }
            }
        }
    }

    return 0;
}

Set::Assoc* Set::basicAt(Assoc* anAssoc)
{
    unsigned index = hashAssoc(anAssoc) % size;
    Assoc* head  = array[index];

    if (head)
    {
        if (isEqual(head,anAssoc))
        {
            return head;
        }
        else if (head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for( i.previousLink = head; !i.isDone(); i.next() )
            {
                current = (Assoc*)i.current();

                if( isEqual(current,anAssoc) )
                {
                    return current;
                }
            }
        }
    }

    return 0;
}

Set::Assoc* Set::basicAt(const char* aKey)
{
    unsigned index = hashString(aKey) % size;
    Assoc* head  = array[index];

    if (head)
    {
        if (hasKey(head,aKey))
        {
            return head;
        }
        else if (head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for( i.previousLink = head; !i.isDone(); i.next() )
            {
                current = (Assoc*)i.current();

                if( hasKey(current,aKey) )
                {
                    return current;
                }
            }
        }
    }

    return 0;
}

Set::Assoc* Set::basicAt(int anIndex)
{
    unsigned index = hashInteger(anIndex) % size;
    Assoc* head  = array[index];

    if (head)
    {
        if (hasIndex(head,anIndex))
        {
            return head;
        }
        else if (head->next) // at least two elements
        {
            Assoc* current;

            List::LinkIter i(head->next);

            for (i.previousLink = head; !i.isDone(); i.next())
            {
                current = (Assoc*)i.current();

                if (hasIndex(current,anIndex))
                {
                    return current;
                }
            }
        }
    }

    return 0;
}
