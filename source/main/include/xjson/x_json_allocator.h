#ifndef __XBASE_JSON_ALLOCATOR_H__
#define __XBASE_JSON_ALLOCATOR_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_debug.h"

namespace xcore
{
    namespace json
    {
		struct allocator_t
		{
			void  Init(s32 max_size, const char* debug_name);
			void  Destroy();
			void  SetOwner(ThreadId thread_id);
			char* Allocate(s32 size, s32 align);

			char* CheckOut(char*& end, s32 align = 4);
			void  Commit(char* ptr);

			void Reset();

			template <typename T> T* Allocate() { return static_cast<T*>((void*)Allocate(sizeof(T), ALIGNOF(T))); }
			template <typename T> T* AllocateArray(s32 count) { return static_cast<T*>((void*)Allocate(sizeof(T) * count, ALIGNOF(T))); }

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			char*       m_Pointer; // allocated pointer
			char*       m_Cursor;  // current pointer
			s32         m_Size;
			ThreadId    m_OwnerThread;
			const char* m_DebugName;
		};

        class allocator_scope_t
        {
        public:
            explicit allocator_scope_t(allocator_t* a);
            ~allocator_scope_t();

        private:
            allocator_scope_t(const allocator_scope_t&);
            allocator_scope_t& operator=(const allocator_scope_t&);
            allocator_t*       m_Allocator;
            char*              m_Pointer;
        };

        allocator_t* CreateAllocator(u32 size, const char* name = "JSON Allocator");
        void         DestroyAllocator(allocator_t* alloc);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__