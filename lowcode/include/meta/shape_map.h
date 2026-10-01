#pragma once

#include <cstdint>

#include "include/meta/exbox_core.h"
#include "include/meta/meta_arena.h"

namespace ESLowcode {

const uint16_t READONLY = 0x01;  // ...0001 = 1
const uint16_t HIDDEN = 0x02;    // ...0010 = 2
const uint16_t IS_GETTER = 0x04; // ...0100 = 4
const uint16_t IS_SETTER = 0x08; // ...1000 = 8

// Property Descriptor
struct PropertyDescriptor {
    atom nameAtom;    // hash/atom of property
    uint16_t offsetIndex; // index in object
    uint16_t attributes;  // readonly, hidden, isGetter, isSetter
}; // 64 bit

struct Shape;

// Transition
struct TransitionEntry {
    atom propertyAtom; // atom of added field
    Shape* targetShape;    // ptr of shape object will become
};

struct Shape {
    TypeTag baseTypeId;                 // static type of object
    Shape* parentShape;                 // ptr to parent shape

    uint32_t propertyCount;             // count of properties
    PropertyDescriptor* propArray;      // ptr to descriptor array

    uint16_t transitionCount;
    TransitionEntry* transitionTable;   // transitions array (table)

    bool isMarkedAlive;
};

// Creates root Shape for type with zero properties
Shape* createRootShape(TypeTag baseTypeId, MetaArena* arena);

// Executes a shape transition when a new property is dynamicly attached to an object
Shape* transitionShape(Shape* currentShape, atom propertyAtom);

// Finds the array offset idx for a given property atom within the Shape layaot
uint findPropertyOffset(Shape* shape, atom propertyAtom);

// Checks if this property exists in current Shape `shape`
bool hasProperty(Shape* shape, atom propertyAtom);

// Marks the Shape `shape` as active during GC Mark phase
void markShapeAlive(Shape* shape);

// Cleans unused graph branches
sizeT sweepOrphanedShapes(MetaArena* metaArena);

}