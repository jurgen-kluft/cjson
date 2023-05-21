#ifndef __CJSON_JSON_ALLOCATOR_H__
#define __CJSON_JSON_ALLOCATOR_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "cbase/c_allocator.h"
#include "ccore/c_debug.h"

namespace ncore
{
    namespace json
    {
        struct JsonAllocator
        {
            void  Init(s32 max_size, const char* debug_name);
            void  Init(void* mem, u32 len, const char* debug_name);
            void  Destroy();
            char* Allocate(s32 size, s32 align);

            char* CheckOut(char*& end, s32 align = 4);
            void  Commit(char* ptr);

            void Reset();

            template <typename T> T* Allocate() 
            { 
                void* mem = Allocate(sizeof(T), alignof (T));
                return static_cast<T*>(mem);
            }
            template <typename T> T* AllocateArray(s32 count) 
            { 
                return static_cast<T*>((void*)Allocate(sizeof(T) * count, alignof(T)));
            }

            DCORE_CLASS_PLACEMENT_NEW_DELETE

            char*       m_Pointer; // allocated pointer
            char*       m_Cursor;  // current pointer
            s32         m_Size;
            s32         m_Owner;
            const char* m_DebugName;
        };

        class JsonAllocatorScope
        {
        public:
            explicit JsonAllocatorScope(JsonAllocator* a);
            ~JsonAllocatorScope();

        private:
            JsonAllocatorScope(const JsonAllocatorScope& o) {}
			JsonAllocatorScope& operator=(const JsonAllocatorScope&) { return *this; }
            JsonAllocator*     m_Allocator;
            char*              m_Cursor;
        };

        JsonAllocator* CreateAllocator(u32 size, const char* name = "JSON Allocator");
        JsonAllocator* CreateAllocator(void* mem, u32 len, const char* name = "JSON Allocator");
        void           DestroyAllocator(JsonAllocator* alloc);

    } // namespace json
} // namespace ncore

#endif // __CJSON_JSON_ALLOCATOR_H__