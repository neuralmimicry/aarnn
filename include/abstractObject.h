

#include <string.h>
#include "./ThreadSafeQueue.h"

// Header
struct AbstractObject;
AbstractObject* AbstractObject__create(char *objectLabel, char *componentList, char *componentValues);
void AbstractObject__destroy(AbstractObject* self);
char AbstractObject__componentList(AbstractObject* self);
char AbstractObject__componentValues(AbstractObject* self);
