#include "Dendrite.h"

// Dendrite class
void Dendrite::initialise()
{
    if(!instanceInitialised)
    {
        if(!this->dendriteBouton)
        {
            this->dendriteBouton = std::make_shared<DendriteBouton>(
             std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1));
            this->dendriteBouton->initialise();
            this->dendriteBouton->updateFromDendrite(shared_from_this());
        }
        instanceInitialised = true;
    }
}

// Get the dendrite bouton or multiple dendrite boutons
std::shared_ptr<DendriteBouton> Dendrite::getDendriteBoutons()
{
    return this->dendriteBouton;
}

