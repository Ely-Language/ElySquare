#include <cstdint>

#include "exbox_core.h"

namespace ESLowcode {

const uint16_t READONLY = 0x01;  // ...0001 = 1
const uint16_t HIDDEN = 0x02;    // ...0010 = 2
const uint16_t IS_GETTER = 0x04; // ...0100 = 4
const uint16_t IS_SETTER = 0x08; // ...1000 = 8

// Property Descriptor
struct PropertyDescriptor {
    uint32_t nameAtom;    // hash/atom of property
    uint16_t offsetIndex; // index in object
    uint16_t attributes;  // readonly, hidden, isGetter, isSetter
}; // 64 bit

struct Shape;

// Transition
struct TransitionEntry {
    uint32_t propertyAtom; // atom of added field
    Shape* targetShape;    // ptr of shape object will become
};

struct Shape {
    TypeTag baseTypeId;                 // static type of object
    Shape* parentShape;                 // ptr to parent shape

    uint32_t propertyCount;             // count of properties
    PropertyDescriptor* propArray;    // ptr to descriptor array

    uint16_t transitionCount;
    TransitionEntry* transitionTable; // transitions array (table)

    bool isMarkedAlive;
};

Shape* createShape(TypeTag baseTypeId);

}