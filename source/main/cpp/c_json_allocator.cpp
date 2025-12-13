#include "cbase/c_allocator.h"
#include "cbase/c_memory.h"
#include "cjson/c_json_allocator.h"

namespace ncore
{
    namespace njson
    {
        JsonAllocatorScope::JsonAllocatorScope(JsonAllocator* a)
            : m_Allocator(a)
            , m_Size(a->m_Size)
        {
        }

        JsonAllocatorScope::~JsonAllocatorScope() { m_Allocator->m_Size = m_Size; }

        void JsonAllocator::Init(alloc_t* alloc, s64 max_size, const char* debug_name)
        {
            this->m_Alloc     = alloc;
            this->m_Pointer   = static_cast<char*>(g_allocate_array<char>(alloc, max_size));
            this->m_Size      = 0;
            this->m_Capacity  = max_size;
            this->m_DebugName = debug_name;
            Reset();
        }

        void JsonAllocator::Init(void* mem, s64 len, const char* debug_name)
        {
            this->m_Alloc     = nullptr;
            this->m_Pointer   = (char*)mem;
            this->m_Size      = 0;
            this->m_Capacity  = len;
            this->m_DebugName = debug_name;
            Reset();
        }

        void JsonAllocator::Destroy()
        {
            if (this->m_Alloc != nullptr)
            {
                g_deallocate_array(this->m_Alloc, this->m_Pointer);
                this->m_Alloc = nullptr;
            }
            this->m_Pointer   = nullptr;
            this->m_Size      = 0;
            this->m_Capacity  = 0;
            this->m_DebugName = nullptr;
        }

        char* JsonAllocator::Allocate(s64 size, s16 alignment)
        {
            ASSERT(alignment <= (s16)sizeof(void*));
            // Compute aligned offset.
            const s64 align  = sizeof(void*); // Pointer size alignment
            const s64 cursor = this->m_Size;
            const s64 offset = (cursor + align - 1) & ~(align - 1);
            ASSERT(0 == (offset & (align - 1)));

            if ((offset + size) <= this->m_Capacity) // See if we have space.
            {
                char* ptr    = this->m_Pointer + offset;
                this->m_Size = offset + size;
                return ptr;
            }
            else
            {
                ASSERTS(false, "Out of memory in linear allocator (json parser)");
                return nullptr;
            }
        }

        char* JsonAllocator::CheckOut(char*& end)
        {
            // Compute aligned offset.
            const s64 align  = sizeof(void*); // Pointer size alignment
            const s64 cursor = this->m_Size;
            const s64 offset = (cursor + align - 1) & ~(align - 1);
            ASSERT(0 == (offset & (align - 1)));
            char* ptr    = this->m_Pointer + offset;
            end          = this->m_Pointer + this->m_Capacity;
            this->m_Size = offset;
            return ptr;
        }

        void JsonAllocator::Commit(char* ptr)
        {
            ASSERT((ptr >= this->m_Pointer) && (ptr <= (this->m_Pointer + this->m_Capacity)));
            this->m_Size = (s64)(ptr - this->m_Pointer);
        }

        void JsonAllocator::Reset()
        {
            this->m_Size = 0;
            nmem::memset(this->m_Pointer, 0xCD, this->m_Capacity);
        }

    } // namespace njson
} // namespace ncore
