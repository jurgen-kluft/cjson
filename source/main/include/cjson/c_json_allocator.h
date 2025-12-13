#ifndef __CJSON_JSON_ALLOCATOR_H__
#define __CJSON_JSON_ALLOCATOR_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_allocator.h"
#include "ccore/c_debug.h"

namespace ncore
{
    namespace njson
    {
        struct JsonAllocator
        {
            void  Init(alloc_t* alloc, s64 max_size, const char* debug_name);
            void  Init(void* mem, s64 len, const char* debug_name);
            void  Destroy();
            char* Allocate(s64 size, s16 alignment);

            char* CheckOut(char*& end);
            void  Commit(char* ptr);

            void Reset();

            template <typename T> T* Allocate(s16 alignment = sizeof(void*))
            {
                ASSERT(alignment <= (s16)sizeof(void*));
                void* mem = Allocate(sizeof(T), alignment);
                return static_cast<T*>(mem);
            }
            template <typename T> T* AllocateArray(s32 count) { return static_cast<T*>((void*)Allocate(sizeof(T) * count, sizeof(void*))); }

            DCORE_CLASS_PLACEMENT_NEW_DELETE

            alloc_t*    m_Alloc;     // underlying allocator
            char*       m_Pointer;   // allocated pointer
            s64         m_Size;      // current size
            s64         m_Capacity;  // total capacity
            const char* m_DebugName; // debug name
        };

        class JsonAllocatorScope
        {
        public:
            explicit JsonAllocatorScope(JsonAllocator* a);
            ~JsonAllocatorScope();

        private:
            JsonAllocatorScope(const JsonAllocatorScope& o) {}
            JsonAllocatorScope& operator=(const JsonAllocatorScope&) { return *this; }
            JsonAllocator*      m_Allocator;
            s64                 m_Size;
        };
    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_ALLOCATOR_H__
