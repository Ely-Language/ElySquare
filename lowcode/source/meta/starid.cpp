#include "include/meta/starid.h"

namespace ESLowcode {

ALAR::ALAR(::ESLowcode::MetaArena* arenaCreatingIn, ushort starIDMaxCount) {
    if (arenaCreatingIn == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::InternalError,
            "Couldn't create ALAR for Thread: there is no arena for it",
            __FILE__,
            __LINE__ - 5
        );
    }

    const sizeT arenaStarIDSize = sizeof(ALAR::StarPlace) * starIDMaxCount;

    void* ptr = arenaCreatingIn->metalloc(
        sizeof(::ESLowcode::MetaArena),
        alignof(::ESLowcode::MetaArena)
    );

    if (ptr == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Couldn't create ALAR for Thread: there is no memory",
            __FILE__,
            __LINE__ - 10
        );
    }
    
    this->arena = new(ptr) ::ESLowcode::MetaArena(arenaStarIDSize * 4, false);

    void* placeListPtr = this->arena->metalloc(
        arenaStarIDSize, 
        alignof(StarPlace)
    );

    if (placeListPtr == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Couldn't create ALAR for Thread: there is no memory",
            __FILE__,
            __LINE__ - 10
        );
    }

    this->placelist = new (placeListPtr) StarPlace[starIDMaxCount]();
    
    void* idmapPtr = this->arena->metalloc(
        sizeof(ubyte) * starIDMaxCount, 
        alignof(ubyte)
    );

    if (idmapPtr == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Couldn't create ALAR for Thread: there is no memory",
            __FILE__,
            __LINE__ - 10
        );
    }

    this->idmap = new (idmapPtr) ubyte[starIDMaxCount]();

    this->starIDMaxCount = starIDMaxCount;
}

atom ALAR::numToStarId(ushort num) {
    char buffer[8]; // "*xxxxxx\0"
    buffer[0] = '*';

    ubyte idx = 6;
    atom n = num;

    do {
        buffer[idx--] = stuff::starIDCharset[n % 36];
        n /= 36;
    } while (n > 0 && idx > 0);

    while (idx > 0) {
        buffer[idx--] = '0';
    }

    return charToAtom(buffer);
}

ushort ALAR::starIdToNum(atom starID) {
    return static_cast<ushort>(starID & stuff::stringFlags::STARID_INDEX_MASK);
}

atom ALAR::getNewStarId() {
    if (createdPlacesCount >= starIDMaxCount) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::RuntimeError,
            "String ALAR overflow!",
            __FILE__,
            __LINE__ - 5
        );
    }
    return this->numToStarId(createdPlacesCount++);
}

atom ALAR::getStarIdOf(const char* raw) {
    for (ushort i; i < createdPlacesCount; i++) {
        StarPlace place = placelist[i];
        if (::std::strcmp(*place.ptr, raw) == 0) {
            return numToStarId(i);
        }
    }

#ifdef DEBUG
    ::es::raiseble::warning(
        ::es::ErrorCode::RuntimeError,
        "Failed to find same char in ALAR",
        __FILE__,
        __LINE__ - 12
    );
#endif

    return brokenAtom;
}

atom ALAR::staridize(char* text, sizeT length) {
    atom forFound = getStarIdOf(text);
    
    // fast path
    if (forFound != brokenAtom) return forFound;

    // slow path
    ushort i;
    while (idmap[i] != 0) {
        i++;
    }

    void* textPtr = arena->metalloc((sizeof(char) * length), alignof(char));

    if (textPtr == nullptr) [[unlikely]] {
        ::es::raiseble::panic(
            ::es::ErrorCode::OutOfMemory,
            "Can't write new string",
            __FILE__,
            __LINE__ - 5
        );
    }

    char* refText = new (textPtr) char();
    refText = text;

    if (idmap[i+1] == 1) { // reuse
        placelist[i].ptr = &refText;
        placelist[i].lenAndMark = length | ~stuff::stringFlags::GC_MARK_RAW_MASK;
    } else { // creating
        StarPlace place = StarPlace();
        place.ptr = &refText;
        place.lenAndMark = length | ~stuff::stringFlags::GC_MARK_RAW_MASK;
        placelist[i] = place;
    }

    idmap[i] = 1;

    return numToStarId(i);
}

const char* ALAR::getRaw(atom starId) {
    ushort idx = starIdToNum(starId);
    if (starId == brokenAtom || idx < createdPlacesCount) {
        return *(this->placelist[idx].ptr);
    }
    return "\0";
}

sizeT ALAR::lenOf(atom starID) {
    ushort idx = starIdToNum(starID);
    if (starID == brokenAtom || idx < createdPlacesCount) {
        return this->placelist[idx].len();
    }
    return 0;
}

sizeT ALAR::sweepPlaces() {
    sizeT killed = 0;
    for (ushort i = 0; i < createdPlacesCount; i++) {
        if (placelist[i].isMarkedAlive()) {
            StarPlace place = placelist[i];
            arena->metarecycle(*place.ptr, place.len());
            placelist->lenAndMark = 0;
            idmap[i] = 0;
            killed++;
        }
    }
    return killed;
}

}