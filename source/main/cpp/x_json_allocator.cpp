#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json_allocator.h"

namespace xcore
{
    namespace json
    {
		typedef s32 ThreadId;


		allocator_t* CreateAllocator(u32 size, const char* name)
		{
			allocator_t* alloc = context_t::system_alloc()->construct<allocator_t>();
			alloc->Init(size, name);
			return alloc;
		}

		void DestroyAllocator(allocator_t* alloc)
		{
			alloc->Destroy();
			context_t::system_alloc()->destruct(alloc);
		}

		allocator_scope_t::allocator_scope_t(allocator_t* a)
			: m_Allocator(a)
			, m_Pointer(a->m_Pointer)
		{
		}

		allocator_scope_t::~allocator_scope_t() { m_Allocator->m_Pointer = m_Pointer; }


#define CHECK_THREAD_OWNERSHIP(alloc) ASSERT(context_t::thread_index() == alloc->m_OwnerThread)

		void allocator_t::Init(s32 max_size, const char* debug_name)
		{
			this->m_Pointer     = static_cast<char*>(context_t::system_alloc()->allocate(max_size));
			this->m_Cursor      = this->m_Pointer;
			this->m_Size        = max_size;
			this->m_OwnerThread = context_t::thread_index();
			this->m_DebugName   = debug_name;
		}

		void allocator_t::Destroy()
		{
			context_t::system_alloc()->deallocate(this->m_Pointer);
			this->m_Pointer = nullptr;
			this->m_Cursor  = nullptr;
		}

		void  allocator_t::SetOwner(ThreadId thread_id) { this->m_OwnerThread = thread_id; }
		char* allocator_t::Allocate(s32 size, s32 align)
		{
			CHECK_THREAD_OWNERSHIP(this);

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

		char* allocator_t::CheckOut(char*& end, s32 align)
		{
			CHECK_THREAD_OWNERSHIP(this);

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

		void allocator_t::Commit(char* ptr)
		{
			ASSERT((ptr >= this->m_Cursor) && (ptr <= (this->m_Pointer + this->m_Size)));
			CHECK_THREAD_OWNERSHIP(this);
			this->m_Cursor = (char*)ptr;
		}

		void allocator_t::Reset()
		{
			CHECK_THREAD_OWNERSHIP(this);
			this->m_Cursor = this->m_Pointer;
		}


    } // namespace json
} // namespace xcore