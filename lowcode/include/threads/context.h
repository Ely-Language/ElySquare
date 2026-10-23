#pragma once
#include <include/allocators/tlab.h>

#include <include/meta/meta_arena.h>
#include <include/meta/shape_map.h>
#include <include/meta/starid.h>

namespace es {

class ThreadContext {
private:
    sizeT maxStarIdCount;

    // Meta Arena
    ::ESLowcode::MetaArena* metaArena;

    // Thread-Local Allocation Buffer
    ::es::TLAB* tlab;

    // Root Shape
    ::ESLowcode::Shape* rootShape;

    // Arena-Local Atom Registry
    ::ESLowcode::ALAR* alar;

public:
    ThreadContext();
    ~ThreadContext();

    // returns thread's meta arena
    ::ESLowcode::MetaArena* getMetaArene();

    // returns thread-local allocation buffer of current thread
    ::es::TLAB* getTLAB();

    // returns the root shape of this thread
    ::ESLowcode::Shape* getRootShape();

    // returns the alar of the tread
    ::ESLowcode::ALAR* getALAR();
};

ThreadContext* getThread();

}