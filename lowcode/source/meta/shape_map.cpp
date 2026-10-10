#include "include/meta/shape_map.h"

namespace ESLowcode {

Shape* createRootShape(MetaArena* arena) {
    if (!arena) [[unlikely]] return nullptr;

    void* ptr = arena->metalloc(sizeof(Shape), alignof(Shape));

    if (!ptr) {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Error while allocating the memory for root shape",
            __FILE__,
            __LINE__ - 7
        );
    }

    Shape* shape = new (ptr) Shape();

    shape->parentShape = nullptr;
    shape->arena = arena;

    shape->propertyCount = 0;
    shape->propArray = nullptr;

    shape->transitionCount = 0;
    shape->transitionCapacity = 0;
    shape->transitionTable = nullptr;

    shape->shapeFlags = 0x01;

    return shape;
}

TransitionEntry* transiteTo(Shape* shapeTo, atom propertyAtom) {
    void* ptr = shapeTo->arena->metalloc(
        sizeof(TransitionEntry),
        alignof(TransitionEntry)
    );

    if (!ptr) [[unlikely]] ::es::raiseble::panic(
        ::es::ErrorCode::OutOfMemory,
        "Failed to get memory for Shape transition",
        __FILE__,
        __LINE__ - 9
    );

    TransitionEntry* transition = new (ptr) TransitionEntry();
    transition->propertyAtom = propertyAtom;
    transition->targetShape = shapeTo;

    return transition;
}

Shape* makeShape(Shape* parent, uint16_t atomFlags) {
    void* ptr = parent->arena->metalloc(
        sizeof(Shape),
        alignof(Shape)
    );

    if (!ptr) [[unlikely]] ::es::raiseble::panic(
        ::es::ErrorCode::OutOfMemory,
        "Failed to get memory for new Shape",
        __FILE__,
        __LINE__ - 9
    );

    Shape* child = new (ptr) Shape();

    child->parentShape = parent;
    child->arena = parent->arena;

    child->shapeFlags = getShapeFlags(parent, atomFlags);

    child->transitionCount = 0;
    child->transitionCapacity = 0;
    child->transitionTable = nullptr;

    // Creation of properties
    child->propertyCount = parent->propertyCount + 1;

    void* propArrayPtr = child->arena->metalloc(
        sizeof(PropertyDescriptor[child->propertyCount]),
        alignof(PropertyDescriptor[child->propertyCount])
    );

    if (!propArrayPtr) [[unlikely]] ::es::raiseble::panic(
        ::es::ErrorCode::OutOfMemory,
        "Failed to get memory for new Shape properties",
        __FILE__,
        __LINE__ - 9
    );

    PropertyDescriptor* propDescArr = new (propArrayPtr) PropertyDescriptor[child->propertyCount];

    // copying properties
    if (parent->propertyCount) {
        for (uint16_t i = 0; i < parent->propertyCount; i++) {
            propDescArr[i] = parent->propArray[i];
        }
    }

    child->propArray = propDescArr;
    
    return child;
}

PropertyDescriptor makePropertyInShape(
    atom nameAtom,
    uint16_t propertyValueOffsetIndex,
    bool isReadonly,
    bool isHidden,
    bool isGetter,
    bool isSetter
) {
    if (!nameAtom) [[unlikely]] {
        ::es::raiseble::warning(
            ::es::ErrorCode::TypeNotRegistered,
            "Invalid attempt to create Shape property descriptor",
            __FILE__,
            __LINE__ - 5
        );
        return PropertyDescriptor();
    }

    PropertyDescriptor property = PropertyDescriptor();

    property.nameAtom = nameAtom;
    property.offsetIndex = propertyValueOffsetIndex;

    // FLAGS
    property.attributes = 0U;
    if (isReadonly) property.attributes |= stuff::accessFlags::READONLY;
    if (isHidden) property.attributes |= stuff::accessFlags::HIDDEN;
    if (isGetter) property.attributes |= stuff::accessFlags::IS_GETTER;
    if (isSetter) property.attributes |= stuff::accessFlags::IS_SETTER;

    return property;
}

Shape* createTransition(
    Shape* currentShape,
    atom propertyAtom,
    uint16_t offset,
    bool isReadonly,
    bool isHidden,
    bool isGetter,
    bool isSetter
) {
    // Checking the shape
    if (currentShape == nullptr) [[unlikely]] return nullptr;

    uint16_t transitionIdx = currentShape->transitionCount;

    // LOOKING FOR AN INDEX (if there is no space)
    if (currentShape->transitionCount + 1 > currentShape->transitionCapacity) {

        if (currentShape->transitionCount == 0) {
            const sizeT newCapacity = 2;
            void* ptr = currentShape->arena->metalloc(
                sizeof(TransitionEntry*[newCapacity]), 
                alignof(TransitionEntry*[newCapacity])
            );

            if (!ptr) [[unlikely]] ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Failed to get memory for Shape transition",
                __FILE__,
                __LINE__ - 9
            );

            TransitionEntry** newTransitionTable = new (ptr) TransitionEntry*[newCapacity]; 
            currentShape->transitionTable = newTransitionTable;

            currentShape->transitionTable[0] = nullptr;
            currentShape->transitionTable[1] = nullptr;
            
            currentShape->transitionCapacity = newCapacity;
            // transitionIdx = 0; // already
        } else { // if transition count is more than zero
            sizeT newCapacity = stuff::getNextTransitionCapacity(currentShape->transitionCapacity);
            void* ptr = currentShape->arena->metalloc(
                sizeof(TransitionEntry*[newCapacity]),
                alignof(TransitionEntry*[newCapacity])
            );

            if (!ptr) [[unlikely]] ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Failed to get memory for Shape transition",
                __FILE__,
                __LINE__ - 9
            );

            TransitionEntry** newTransitionTable = new (ptr) TransitionEntry*[newCapacity];
            // Copying values
            for (sizeT i = 0; i < currentShape->transitionCount; i++) {
                newTransitionTable[i] = currentShape->transitionTable[i];
            }
            for (sizeT i = currentShape->transitionCount; i < newCapacity; i++) {
                newTransitionTable[i] = nullptr;
            }
            TransitionEntry** oldTransitionTable = currentShape->transitionTable;

            currentShape->transitionTable = newTransitionTable;
            currentShape->arena->metarecycle(oldTransitionTable, sizeof(TransitionEntry*) * currentShape->transitionCapacity);

            currentShape->transitionCapacity = newCapacity;
            transitionIdx = currentShape->transitionCount - 1;
        }
    } else { // if we have some space
        transitionIdx = currentShape->transitionCount;
    }

    // IF WE'RE HERE, idx was found

    // PROPERTY
    PropertyDescriptor property = makePropertyInShape(
        propertyAtom,
        offset,
        isReadonly,
        isHidden,
        isGetter,
        isSetter
    );
    
    Shape* child = makeShape(currentShape, property.attributes);
    
    child->propArray[child->propertyCount - 1] = property;

    TransitionEntry* transition = transiteTo(child, propertyAtom);

    currentShape->transitionTable[transitionIdx] = transition;
    currentShape->transitionCount++;

    return child;
}

Shape* transitionShape(
    Shape* currentShape,
    atom propertyAtom,
    uint16_t offset,
    bool isReadonly,
    bool isHidden,
    bool isGetter,
    bool isSetter
) {
    // Checking the shape
    if (currentShape == nullptr) [[unlikely]] return nullptr;

    uint16_t attributes = 0U;
    if (isReadonly) attributes |= stuff::accessFlags::READONLY;
    if (isHidden) attributes |= stuff::accessFlags::HIDDEN;
    if (isGetter) attributes |= stuff::accessFlags::IS_GETTER;
    if (isSetter) attributes |= stuff::accessFlags::IS_SETTER;

    if (currentShape->transitionTable != nullptr) [[likely]] {
        // Has we shape with same transition?
        for (uint16_t i = 0; i < currentShape->transitionCount; i++) {
            TransitionEntry* transition = currentShape->transitionTable[i];
            if (
                transition->propertyAtom == propertyAtom &&
                transition->targetShape->propArray[transition->targetShape->propertyCount - 1]
                .attributes == attributes
            ) 
                return transition->targetShape;
        }
    }

    return createTransition(
        currentShape,
        propertyAtom,
        offset,
        isReadonly,
        isHidden,
        isGetter,
        isSetter
    );
}

sizeT sweepOrphanedShapes(MetaArena* metaArena) {

}

}