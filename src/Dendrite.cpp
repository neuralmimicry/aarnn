#include "Dendrite.h"

// Dendrite class
void Dendrite::initialise()
{
    if(!instanceInitialised)
    {
        if(!dendriteBouton)
        {
            dendriteBouton = std::make_shared<DendriteBouton>(
             std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1));
            dendriteBouton->initialise();
            dendriteBouton->updateFromDendrite(shared_from_this());
        }
        instanceInitialised = true;
    }
}