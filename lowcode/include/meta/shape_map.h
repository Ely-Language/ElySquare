#pragma once

#include <cstdint>
#include <new>

#include "include/meta/exbox_core.h"
#include "include/meta/meta_arena.h"
#include "backend/include/typing/typing.hpp"

namespace ESLowcode {

namespace stuff {

namespace accessFlags {
const uint16_t READONLY = 0x01;  // ...0001 = 1
const uint16_t HIDDEN = 0x02;    // ...0010 = 2
const uint16_t IS_GETTER = 0x04; // ...0100 = 4
const uint16_t IS_SETTER = 0x08; // ...1000 = 8
}

namespace shapeFlags {    
const uint16_t IS_MARKED_ALIVE = 0x01;   // ...0001 = 1
const uint16_t HAS_ACCESSORS = 0x02;     // ...0010 = 2
const uint16_t IS_FROZEN = 0x04;         // ...0100 = 4
const uint16_t IS_HASH_TABLE_MODE = 0x08;// ...1000 = 8
}

inline sizeT getNextTransitionCapacity(sizeT currentCapacity) {
    if (currentCapacity == 0) [[unlikely]] return 2;

    if (currentCapacity < 32) return currentCapacity * 2;

    return currentCapacity + 4;
}

}

// Property Descriptor
struct PropertyDescriptor {
    atom nameAtom;    // hash/atom of property
    uint16_t offsetIndex; // index in object
    uint16_t attributes;  // readonly, hidden, isGetter, isSetter
}; // 64 bit (8 bytes)

struct Shape;

// Transition
struct TransitionEntry {
    Shape* targetShape; // ptr of shape object will become
    atom propertyAtom; // atom of added field
    atom _padding; // + 4 byte (speed-up by aligning)
};

struct Shape {
    Shape* parentShape;                 // ptr to parent shape
    MetaArena* arena;                   // Arena of it

    uint16_t shapeFlags;
    
    uint16_t propertyCount;             // count of properties
    PropertyDescriptor* propArray;      // ptr to descriptor array

    uint16_t transitionCount;
    uint16_t transitionCapacity;
    TransitionEntry** transitionTable;   // transitions array (table)

}; // 40 bytes

// Creates root Shape for type with zero properties
Shape* createRootShape(MetaArena* arena);

// Returns flags for shape from atom's flags
inline uint16_t getShapeFlags(Shape* parent, uint16_t atomFlags) {
    if (!parent) [[unlikely]] return 0U;

    uint16_t shapeFlags = 0U;
    if (
        (atomFlags & stuff::accessFlags::IS_GETTER) != 0 || 
        (atomFlags & stuff::accessFlags::IS_SETTER) != 0 ||
        (parent->shapeFlags & stuff::shapeFlags::HAS_ACCESSORS)
    ) {
        shapeFlags |= stuff::shapeFlags::HAS_ACCESSORS;
    }

    return shapeFlags;
}

// Returns transition with atom to specified Shape
TransitionEntry* transiteTo(Shape* shapeTo, atom propertyAtom);

// Creates new Shape from parent Shape
Shape* makeShape(Shape* parent, uint16_t atomFlags);

// Creates Property for Shape 
PropertyDescriptor makePropertyInShape(
    atom nameAtom,
    uint16_t offset,
    bool isReadonly = false,
    bool isHidden = false,
    bool isGetter = false,
    bool isSetter = false
);

// create transiton shape
Shape* createTransition(
    Shape* currentShape,
    atom propertyAtom,
    uint16_t offset,
    bool isReadonly = false,
    bool isHidden = false,
    bool isGetter = false,
    bool isSetter = false
);

// Executes a shape transition when a new property is dynamicly attached to an object
Shape* transitionShape(
    Shape* currentShape,
    atom propertyAtom,
    uint16_t offset,
    bool isReadonly = false,
    bool isHidden = false,
    bool isGetter = false,
    bool isSetter = false
);

// Finds the array offset idx for a given property atom within the Shape layaot
inline uint16_t findPropertyOffset(Shape* shape, atom propertyAtom) {
    PropertyDescriptor* property = getProperty(shape, propertyAtom);
    if (property) {
        return property->offsetIndex;
    } else return invalidIndex;
}

// Returns descriptor of property
inline PropertyDescriptor* getProperty(Shape* shape, atom propertyAtom) {
    for (sizeT i = 0; i < shape->propertyCount; i++) {
        if (shape->propArray[i].nameAtom == propertyAtom) {
            return &shape->propArray[i];
        }
    }
    return nullptr;
}

// Checks if this property exists in current Shape `shape`
inline bool hasProperty(Shape* shape, atom propertyAtom) {
    return getProperty(shape, propertyAtom) != nullptr;
}

// Marks the Shape `shape` as active during GC Mark phase
void markShapeAlive(Shape* shape) {
    shape->shapeFlags |= stuff::shapeFlags::IS_MARKED_ALIVE;
}

// Cleans unused graph branches
sizeT sweepOrphanedShapes(MetaArena* metaArena);



// SHAPES ATTRIBUTES

}