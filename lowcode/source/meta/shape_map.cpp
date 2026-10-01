#include "include/meta/shape_map.h"

namespace ESLowcode {

Shape* createRootShape(TypeTag baseTypeId, MetaArena* arena) {
    void* ptr = arena->metalloc(sizeof(Shape), alignof(Shape));
    Shape* shape = new (ptr) Shape();
    shape->propertyCount = 0;
    shape->parentShape = nullptr;
    return shape;
}

Shape* transitionShape(Shape* currentShape, atom propertyAtom) {
    
}

}