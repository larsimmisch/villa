//  collect\set.h

#ifndef _SET_H_
#define _SET_H_

#include <string.h>
#include "list.h"

#ifndef _export
#define _export	__declspec( dllexport )
#endif

class _export Set
{
    public:     //  external protocol

    typedef List::Link Assoc;

    struct _export AssocIter
    {
        AssocIter(Set& aSet);
        ~AssocIter() {}

        Set&    set;
        Assoc*  currentAssoc;
        Assoc*  nextAssoc;
        int     nextIndex;

        Assoc* next();
        Assoc* current() { return currentAssoc; }
        int    isDone()  { return currentAssoc ? 0 : 1;}
        Assoc* back();
        void reset();
        void setToEnd();
        void setTo(Assoc* anAssoc);
    };

    public:

    // assert:
    //  key != 0
    //  aNewAssoc != 0
    //  aNewAssoc->key != 0
    //  anIndex >= 0

    virtual Assoc* basicAdd(Assoc* aNewAssoc);

    virtual Assoc* basicRemoveAt(Assoc* aNewAssoc);
    virtual Assoc* basicRemoveAt(const char* aKey);
    virtual Assoc* basicRemoveAt(int anIndex);

    virtual Assoc* basicAt(Assoc* aNewAssoc);
    virtual Assoc* basicAt(const char* aKey);
    virtual Assoc* basicAt(int anIndex);

    unsigned numElements() { return fill; }

    protected:

    Set(int initialSize = 17);
    virtual ~Set();

    virtual int  isEqual(Assoc* anAssoc, Assoc* anotherAssoc) = 0;
    virtual int  hasKey(Assoc* anAssoc, const char* aKey) = 0;
    virtual int  hasIndex(Assoc* anAssoc, int anIndex) = 0;

    virtual unsigned hashAssoc(Assoc* anAssoc) = 0;
    virtual unsigned hashString(const char* aKey);
    virtual unsigned hashInteger(int anInteger);

    void grow();

    void clear()     { memset(array, 0, size * sizeof(Assoc**)); }

	unsigned size;
	unsigned fill;
	unsigned maxFill;
	Assoc** array;

    private:

    friend struct AssocIter;
};

#endif
