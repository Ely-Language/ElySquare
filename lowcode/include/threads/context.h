#pragma once
#include <include/allocators/tlab.h>

#include <include/meta/meta_arena.h>
#include <include/meta/shape_map.h>

namespace es {

class ThreadContext {
private:
    // Meta Arena
    ::ESLowcode::MetaArena* metaArena;

    // Thread-Local Allocation Buffer
    ::es::TLAB* tlab;

    // Root Shape
    ::ESLowcode::Shape* rootShape;

public:
    ThreadContext();
    ~ThreadContext();

    // returns thread's meta arena
    ::ESLowcode::MetaArena& getMetaArene();

    // returns thread-local allocation buffer of current thread
    ::es::TLAB& getTLAB();

    // returns the root shape of this thread
    ::ESLowcode::Shape& getRootShape();
};

ThreadContext* getThread();

}