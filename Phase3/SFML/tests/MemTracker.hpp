#pragma once
// Counts and checks every new and delete of a test program (AddressSanitizer and valgrind
// are not available with this MinGW, so we do it by hand).
//
// BUILD NOTE (MinGW): link the test with -static (CMake does it). The C++ library .dll cannot
// see the replacement new/delete below, so without -static strings are missed and optimised
// builds give wrong counts.
//
// What it detects:
//   - leak:        a pointer that was made with new and never deleted
//   - double free: a delete of a pointer that is not (or no longer) alive
//   - dangling pointer: a deleted block is filled with 0xDD and never given back to the system,
//                  so a pointer that still points at it reads 0xDDDDDDDD instead of its old value
//
// Include this file FIRST in a test, and set tracking = true only around the code to watch.
#include <cstdlib>
#include <cstring>
#include <new>

#pragma GCC diagnostic ignored "-Wmismatched-new-delete"

const int MAX_TRACKED = 60000;
static void* alivePtr[MAX_TRACKED];
static size_t aliveSize[MAX_TRACKED];
static int aliveCount = 0;
static long allocCount = 0;
static long freeCount = 0;
static long badFrees = 0;
static bool tracking = false;

void* operator new(size_t size){
    void* p = malloc(size);
    if(p == nullptr){
        throw std::bad_alloc();
    }
    if(tracking){
        allocCount++;
        if(aliveCount < MAX_TRACKED){
            alivePtr[aliveCount] = p;
            aliveSize[aliveCount] = size;
            aliveCount++;
        }
    }
    return p;
}

static void forgetPointer(void* p){
    if(p == nullptr){
        return;
    }
    if(tracking){
        for(int i = aliveCount - 1; i >= 0; i--){               // newest pointers are the most likely
            if(alivePtr[i] == p){
                memset(p, 0xDD, aliveSize[i]);                  // poison the block, but do NOT free() it
                alivePtr[i] = alivePtr[aliveCount - 1];         // fill the hole with the last entry
                aliveSize[i] = aliveSize[aliveCount - 1];
                aliveCount--;
                freeCount++;
                return;
            }
        }
        badFrees++;                                             // not alive: a double delete
        return;
    }
    free(p);
}
void operator delete(void* p) noexcept{ forgetPointer(p); }
void operator delete(void* p, size_t) noexcept{ forgetPointer(p); }
