// SPDX-License-Identifier: MIT

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

    shape->nextShapeInArena = nullptr;
    shape->prevShapeInArena = nullptr;

    shape->propertyCount = 0;
    shape->propArray = nullptr;

    shape->transitionCount = 0;
    shape->transitionCapacity = 0;
    shape->transitionTable = nullptr;

    shape->shapeFlags = 0x01;

    shape->arena->setLastAllocatedShape(shape);

    return shape;
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

    Shape* last = child->arena->getLastAllocatedShape();
    if (!last) [[unlikely]] ::es::raiseble::panic(
        ::es::ErrorCode::InternalError,
        "makeShape called before createRootShape",
        __FILE__, __LINE__ - 4
    );
    last->nextShapeInArena = child;
    child->prevShapeInArena = last;
    
    child->shapeFlags = getShapeFlags(parent, atomFlags);
    
    child->transitionCount = 0;
    child->transitionCapacity = 0;
    child->transitionTable = nullptr;
    
    // Creation of properties
    child->propertyCount = parent->propertyCount + 1;
    
    void* propArrayPtr = child->arena->metalloc(
        sizeof(PropertyDescriptor) * child->propertyCount,
        alignof(PropertyDescriptor)
    );
    
    if (!propArrayPtr) [[unlikely]] ::es::raiseble::panic(
        ::es::ErrorCode::OutOfMemory,
        "Failed to get memory for new Shape properties",
        __FILE__,
        __LINE__ - 9
    );
    
    PropertyDescriptor* propDescArr = new (propArrayPtr) PropertyDescriptor[child->propertyCount]();
    
    // copying properties
    if (parent->propertyCount) {
        for (uint16_t i = 0; i < parent->propertyCount; i++) {
            propDescArr[i] = parent->propArray[i];
        }
    }
    
    child->propArray = propDescArr;

    child->arena->setLastAllocatedShape(child);
    
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
                sizeof(TransitionEntry) * newCapacity, 
                alignof(TransitionEntry)
            );

            if (!ptr) [[unlikely]] ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Failed to get memory for Shape transition",
                __FILE__,
                __LINE__ - 9
            );

            TransitionEntry* newTransitionTable = new (ptr) TransitionEntry[newCapacity](); 
            currentShape->transitionTable = newTransitionTable;

            // currentShape->transitionTable[0];
            // currentShape->transitionTable[1];
            
            currentShape->transitionCapacity = newCapacity;
            // transitionIdx = 0; // already
        } else { // if transition count is more than zero
            sizeT newCapacity = stuff::getNextTransitionCapacity(currentShape->transitionCapacity);

            if (newCapacity > UINT16_MAX) [[unlikely]]
            ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Shape transition table too large (> UINT16_MAX entries)",
                __FILE__, __LINE__
            );

            void* ptr = currentShape->arena->metalloc(
                sizeof(TransitionEntry) * newCapacity,
                alignof(TransitionEntry)
            );

            if (!ptr) [[unlikely]] ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Failed to get memory for Shape transition",
                __FILE__,
                __LINE__ - 9
            );

            TransitionEntry* newTransitionTable = new (ptr) TransitionEntry[newCapacity]();
            // Copying values
            for (sizeT i = 0; i < currentShape->transitionCount; i++) {
                newTransitionTable[i] = currentShape->transitionTable[i];
            }
            TransitionEntry* oldTransitionTable = currentShape->transitionTable;

            currentShape->transitionTable = newTransitionTable;
            currentShape->arena->metarecycle(oldTransitionTable, sizeof(TransitionEntry) * currentShape->transitionCapacity);

            if (currentShape->transitionCount == UINT16_MAX) [[unlikely]]
            ::es::raiseble::panic(
                ::es::ErrorCode::OutOfMemory,
                "Shape transition table is too large (> UINT16_MAX entries)",
                __FILE__, 
                __LINE__ - 4
            );

            currentShape->transitionCapacity = static_cast<uint16_t>(newCapacity);
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
    
    if (currentShape->propertyCount == UINT16_MAX) [[unlikely]] 
    ::es::raiseble::panic(
        ::es::ErrorCode::OutOfMemory,
        "Shape properties count is too big",
        __FILE__,
        __LINE__ - 4
    );
    
    Shape* child = makeShape(currentShape, property.attributes);

    child->propArray[child->propertyCount - 1] = property;

    TransitionEntry transition = transiteTo(child, propertyAtom);

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

    if (currentShape->transitionTable != nullptr) {
        // Has we shape with same transition?
        for (uint16_t i = 0; i < currentShape->transitionCount; i++) {
            const TransitionEntry &transition = currentShape->transitionTable[i];
            if (!transition.targetShape) [[unlikely]] continue;
            if (
                transition.propertyAtom == propertyAtom &&
                transition.targetShape->propArray[transition.targetShape->propertyCount - 1]
                .attributes == attributes
            ) 
                return transition.targetShape;
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

void killShape(Shape* shape) {
    if (shape == nullptr) [[unlikely]] return;

    if (shape->propertyCount != 0) {
        shape->arena->metarecycle(
            shape->propArray,
            sizeof(PropertyDescriptor) * shape->propertyCount
        );
    }
    if (shape->transitionCapacity != 0) {
        shape->arena->metarecycle(
            shape->transitionTable,
            sizeof(TransitionEntry) * shape->transitionCapacity
        );
    }
    shape->arena->metarecycle(
        shape,
        sizeof(Shape)
    );
}

void forgetShape(Shape* shape) {
    if (shape == nullptr) [[unlikely]] return; // means you're idiot

    if (shape->prevShapeInArena == nullptr && shape->parentShape == nullptr) {
        return;
    } // root shape

    if (shape->prevShapeInArena == nullptr) [[unlikely]] { // NOOOOOO! 
        ::es::raiseble::panic(
            ::es::ErrorCode::InternalError,
            "Non-root shape with null prevShapeInArena (corrupted list)",
            __FILE__, 
            __LINE__ - 5
        );
    }

    if (shape == shape->arena->getLastAllocatedShape()) [[unlikely]] {
            shape->arena->setLastAllocatedShape(shape->prevShapeInArena);
        }

    // arena links
    if (shape->nextShapeInArena != nullptr) {
        shape->nextShapeInArena->prevShapeInArena = shape->prevShapeInArena;
    }
    shape->prevShapeInArena->nextShapeInArena = shape->nextShapeInArena;

    // parentship links
    const uint16_t count = shape->parentShape->transitionCount;
    for (uint16_t i = 0; i < count; ++i) {
        if (shape->parentShape->transitionTable[i].targetShape == shape) {
            shape->parentShape->transitionTable[i] = shape->parentShape->transitionTable[count - 1];
            shape->parentShape->transitionCount = count - 1;
            return;
        }
    }

    ::es::raiseble::panic(
        ::es::ErrorCode::InternalError,
        "Failed to find parent-current transition",
        __FILE__,
        __LINE__ - 12
    );
}

sizeT sweepOrphanedShapes(MetaArena* metaArena) {
    // 1) get start shape
    Shape* last = metaArena->getLastAllocatedShape();
    if (!last) [[unlikely]] return 0;

    // 2) to-head cycle
    sizeT killed = 0;
    Shape* current = last;
    while (current->prevShapeInArena != nullptr) {
        Shape* prev = current->prevShapeInArena;

        // if it's dead
        if (!isShapeAlive(current)) {
            // deleting it
            forgetShape(current);
            killShape(current);
            killed++;
        } else {
            current->shapeFlags &= ~(stuff::shapeFlags::IS_MARKED_ALIVE);
        }

        current = prev;
    }

    return killed;
}

}