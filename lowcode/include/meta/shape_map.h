// SPDX-License-Identifier: MIT

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

// Transition
struct TransitionEntry {
    Shape* targetShape; // ptr of shape object will become
    atom propertyAtom; // atom of added field
    atom _padding; // + 4 byte (speed-up by aligning)
};

struct Shape {
    Shape* parentShape;                 // ptr to parent shape
    MetaArena* arena;                   // Arena of it
    
    Shape* nextShapeInArena;
    Shape* prevShapeInArena;            // GC - next and prev in meta arena
    
    uint16_t shapeFlags;
    
    uint16_t propertyCount;             // count of properties
    PropertyDescriptor* propArray;      // ptr to descriptor array
    
    uint16_t transitionCount;
    uint16_t transitionCapacity;
    TransitionEntry* transitionTable;   // transitions array (table)
    
}; // 64 bytes

static_assert(sizeof(Shape) == 64, "Shape layout changed");
static_assert(sizeof(TransitionEntry) == 16, "TransitionEntry layout changed");
static_assert(sizeof(PropertyDescriptor) == 8, "PropertyDescriptor layout changed");

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
inline TransitionEntry transiteTo(Shape* shapeTo, atom propertyAtom) {
    TransitionEntry transition = TransitionEntry();
    transition.propertyAtom = propertyAtom;
    transition.targetShape = shapeTo;

    return transition;
}

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

// Returns descriptor of property
inline PropertyDescriptor* getProperty(Shape* shape, atom propertyAtom) {
    if (!shape || shape->propertyCount == 0) [[unlikely]] return nullptr;
    for (sizeT i = 0; i < shape->propertyCount; i++) {
        if (shape->propArray[i].nameAtom == propertyAtom) {
            return &shape->propArray[i];
        }
    }
    return nullptr;
}

// Checks if this property exists in current Shape `shape`
inline bool hasProperty(Shape* shape, atom propertyAtom) {
    if (!shape) return false;
    return getProperty(shape, propertyAtom) != nullptr;
}

// Finds the array offset idx for a given property atom within the Shape layaot
inline uint16_t findPropertyOffset(Shape* shape, atom propertyAtom) {
    PropertyDescriptor* property = getProperty(shape, propertyAtom);
    if (property) {
        return property->offsetIndex;
    } else return invalidIndex;
}

// Marks the Shape `shape` as active during GC Mark phase
// NOT RECURSIVE! just marks THIS shape
inline void markShapeAlive(Shape* shape) {
    if (shape == nullptr) [[unlikely]] return;
    shape->shapeFlags |= stuff::shapeFlags::IS_MARKED_ALIVE;
}

// Kills the shape
void killShape(Shape* shape);

// Making shape's neighbors not addictive to it
void forgetShape(Shape* shape);

// Cleans unused graph branches
sizeT sweepOrphanedShapes(MetaArena* metaArena);



// SHAPES ATTRIBUTES

// is given shape alive? 
inline bool isShapeAlive(Shape* shape) {
    if (!shape) return false;
    return (
        (shape->shapeFlags & stuff::shapeFlags::IS_MARKED_ALIVE) != 0
    );
}

// is it root shape?
inline bool isRootShape(Shape* shape) {
    return shape && shape->parentShape == nullptr;
}

// has it accessors?
inline bool hasShapeAccessors(Shape* shape) {
    return shape && (shape->shapeFlags & stuff::shapeFlags::HAS_ACCESSORS) != 0;
}

// is it frozen
inline bool isShapeFrozen(Shape* shape) {
    return shape && (shape->shapeFlags & stuff::shapeFlags::IS_FROZEN) != 0;
}

// makin' it frozen/cooked
inline void setShapeFrozen(Shape* shape, bool isFrozen = true) {
    if (isFrozen) {
        shape->shapeFlags |= stuff::shapeFlags::IS_FROZEN;
    } else {
        shape->shapeFlags &= ~stuff::shapeFlags::IS_FROZEN;
    }
}

// is it in hash table mode
inline bool isShapeHashTableMode(Shape* shape) {
    return shape && (shape->shapeFlags & stuff::shapeFlags::IS_HASH_TABLE_MODE) != 0;
}

// sets hashTable mode
inline void setShapeHashTableMode(Shape* shape, bool isHashTableMode = true) {
    if (isHashTableMode) {
        shape->shapeFlags |= stuff::shapeFlags::IS_HASH_TABLE_MODE;
    } else {
        shape->shapeFlags &= ~stuff::shapeFlags::IS_HASH_TABLE_MODE;
    }
}

// PropertyDescriptor

// Is property readonly
inline bool isPropertyReadonly(const PropertyDescriptor* prop) {
    return prop && (prop->attributes & stuff::accessFlags::READONLY) != 0;
}
// Is property hidden
inline bool isPropertyHidden(const PropertyDescriptor* prop) {
    return prop && (prop->attributes & stuff::accessFlags::HIDDEN) != 0;
}
// Is property getter
inline bool isPropertyGetter(const PropertyDescriptor* prop) {
    return prop && (prop->attributes & stuff::accessFlags::IS_GETTER) != 0;
}
// Is property setter 
inline bool isPropertySetter(const PropertyDescriptor* prop) {
    return prop && (prop->attributes & stuff::accessFlags::IS_SETTER) != 0;
}

// returns property at index of `index`
inline PropertyDescriptor* getPropertyAt(Shape* shape, uint16_t index) {
    if (!shape || index >= shape->propertyCount) return nullptr;
    return &shape->propArray[index];
}

// returns transition at `index`
inline TransitionEntry* getTransitionAt(Shape* shape, uint16_t index) {
    if (!shape || index >= shape->transitionCount) return nullptr;
    return &shape->transitionTable[index];
}

// returns last property
inline PropertyDescriptor* getLastProperty(Shape* shape) {
    if (!shape || shape->propertyCount == 0) return nullptr;
    return &shape->propArray[shape->propertyCount - 1];
}

// Returns the Shape's transition of given atom
inline TransitionEntry* findTransition(
    Shape* shape, 
    atom propertyAtom, 
    uint16_t attributes
) {
    if (!shape || !shape->transitionTable) return nullptr;

    for (uint16_t i = 0; i < shape->transitionCount; i++) {
        TransitionEntry* transition = &shape->transitionTable[i];
        if (!transition->targetShape || transition->propertyAtom != propertyAtom) continue;

        PropertyDescriptor* lastProp = getLastProperty(transition->targetShape);
        if (lastProp && lastProp->attributes == attributes) return transition;
    }
    return nullptr;
}

// Checks has this Shape transition of given atom
inline bool hasTransitionOn(Shape* shape, atom propertyAtom) {
    if (!shape || !shape->transitionTable) return false;

    for (uint16_t i = 0; i < shape->transitionCount; i++) {
        if (shape->transitionTable[i].propertyAtom == propertyAtom) return true;
    }

    return false;
}

// returns shapes tree depth of `shape`
inline uint16_t getShapeChainDepth(Shape* shape) {
    uint16_t depth = 0;
    while (shape && shape->parentShape) { 
        shape = shape->parentShape; 
        depth++; 
    }
    return depth;
}

}