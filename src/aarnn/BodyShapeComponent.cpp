#include "BodyShapeComponent.h"

void BodyShapeComponent::setShape(const ShapePtr &newShape)
{
    this->shape = newShape;
}

const BodyShapeComponent::ShapePtr &BodyShapeComponent::getShape() const
{
    return shape;
}
