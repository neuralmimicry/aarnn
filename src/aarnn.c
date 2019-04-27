#include <stdio.h>
#include <stddef.h>
#include "Set.h"
#include "Object.h"
#include "new.h"

int main (int argc, char* argv[])
{
    void * s = new(Set);
    void * a = add(s, new(Object));
    void * b = add(s, new(Object));
    void * c = new(Object);

    if (contains(s, a) && contains(s, b))
        puts("ok");
    
    if (contains(s, c))
        puts("contains?");

    if (differ(a, add(s, a)))
        puts("differ?");
    
    if (contains(s, drop(s, a)))
        puts("drop?");
    
    delete(drop(s, b));
    delete(drop(s, c));

    printf( "I am alive!  Beware.\n" );
    getchar();
    return 0;
}
