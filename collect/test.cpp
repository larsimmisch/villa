//  collect\test.cpp

#include "collect\array.h"
#include "collect\bag.h"
#include "collect\cllctn.h"
#include "collect\error.h"
#include <iostream.h>

main()
{
    try
    {
        Array a;
        Bag b;
        Cllctn c;
    }

    catch( CollectionError& error)
    {
        cerr << error;
    }

    catch(char* s)
    {
        cerr << s;
    }

    catch(...)
    {
        cerr << "Aarrrgh...";
    }

}

