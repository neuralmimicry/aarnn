// BodyShapeComponent.h

#ifndef BODY_SHAPE_COMPONENT_H_INCLUDED
#define BODY_SHAPE_COMPONENT_H_INCLUDED

#include "Shape3D.h"

#include <memory>

class BodyShapeComponent
{
    public:
    using ShapePtr                = std::shared_ptr<Shape3D>;
    BodyShapeComponent()          = default;
    virtual ~BodyShapeComponent() = default;
    virtual void                          setShape(const ShapePtr& newShape);
    [[nodiscard]] virtual const ShapePtr& getShape() const;

    private:
    ShapePtr shape;
};

#endif  // BODY_SHAPE_COMPONENT_H_INCLUDED
