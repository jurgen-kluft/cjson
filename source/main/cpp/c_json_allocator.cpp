#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_memory.h"
#include "cjson/c_json_allocator.h"

namespace ncore
{
    namespace json
    {
        JsonAllocator* CreateAllocator(u32 size, const char* name)
        {
            JsonAllocator* alloc = g_construct<JsonAllocator>(g_current_context().system_alloc());
            alloc->Init(size, name);
            return alloc;
        }

        JsonAllocator* CreateAllocator(void* mem, u32 len, const char* name)
        {
            JsonAllocator* alloc = g_construct<JsonAllocator>(g_current_context().system_alloc());
            alloc->Init(mem, len, name);
            return alloc;
        }


        void DestroyAllocator(JsonAllocator* alloc)
        {
            alloc->Destroy();
            g_destruct(g_current_context().system_alloc(), alloc);
        }

        JsonAllocatorScope::JsonAllocatorScope(JsonAllocator* a)
            : m_Allocator(a)
            , m_Cursor(a->m_Cursor)
        {
        }

        JsonAllocatorScope::~JsonAllocatorScope() { m_Allocator->m_Cursor = m_Cursor ; }

        void JsonAllocator::Init(s32 max_size, const char* debug_name)
        {
            this->m_Pointer     = static_cast<char*>(g_allocate_array<char>(g_current_context().system_alloc(), max_size));
            this->m_Cursor      = this->m_Pointer;
            this->m_Size        = max_size;
            this->m_Owner       = 0;
            this->m_DebugName   = debug_name;
            Reset();
        }

        void JsonAllocator::Init(void* mem, u32 len, const char* debug_name)
        {
            this->m_Pointer     = (char*)mem;
            this->m_Cursor      = this->m_Pointer;
            this->m_Size        = len;
            this->m_Owner       = 1;
            this->m_DebugName   = debug_name;
            Reset();
        }

        void JsonAllocator::Destroy()
        {
            if (this->m_Owner != 0)
            {
                g_deallocate_array(g_current_context().system_alloc(), this->m_Pointer);
            }
            this->m_Pointer = nullptr;
            this->m_Cursor  = nullptr;
            this->m_Size    = 0;
            this->m_Owner   = 0;
        }

        char* JsonAllocator::Allocate(s32 size, s32 align)
        {
            ASSERT(0 == (align & (align - 1))); // Alignment must be power of two
            ASSERT(align > 0);                  // Alignment must be non-zero

            // Compute aligned offset.
            s32 const cursor = (s32)(this->m_Cursor - this->m_Pointer);
            s32 const offset = (cursor + align - 1) & ~(align - 1);

            ASSERT(0 == (offset & (align - 1)));

            if ((offset + size) <= this->m_Size) // See if we have space.
            {
                char* ptr      = this->m_Pointer + offset;
                this->m_Cursor = ptr + size;
                return ptr;
            }
            else
            {
                ASSERTS(false, "Out of memory in linear allocator (json parser)");
                return nullptr;
            }
        }

        char* JsonAllocator::CheckOut(char*& end, s32 align)
        {
            ASSERT(0 == (align & (align - 1))); // Alignment must be power of two
            ASSERT(align > 0);                  // Alignment must be non-zero

            // Compute aligned offset.
            s32 const cursor = (s32)(this->m_Cursor - this->m_Pointer);
            s32 const offset = (cursor + align - 1) & ~(align - 1);

            ASSERT(0 == (offset & (align - 1)));

            char* ptr      = this->m_Pointer + offset;
            end            = ptr + (this->m_Size - offset);
            this->m_Cursor = ptr;
            return ptr;
        }

        void JsonAllocator::Commit(char* ptr)
        {
            ASSERT((ptr >= this->m_Cursor) && (ptr <= (this->m_Pointer + this->m_Size)));
            this->m_Cursor = (char*)ptr;
        }

        void JsonAllocator::Reset()
        {
            this->m_Cursor = this->m_Pointer;
			nmem::memset(this->m_Pointer, 0xCD, this->m_Size);
        }

    } // namespace json
} // namespace ncore
