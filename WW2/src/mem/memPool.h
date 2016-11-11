#pragma once
// Memory Pooling of objects.

#include "mem/mem.h"

namespace Mem
{
	// Creates a managed pool of instances of Type. The free list
	// grows BlockSize units of Type at a time. Provides C-style
	// Alloc/Free members along with C++ New/Delete members.
	template<class Type, size_t BlockSize = 1>
	class Pool
	{
		static_assert(BlockSize > 0, "BlockSize must be at least 1.");

		using Block = std::array<Type, BlockSize>;

		// Pointers to blocks of allocations
		std::list<Block> m_blocks;
		// Pointers to individual elements
		std::vector<Type*> m_freeList;
		// Count of assigned allocations
		size_t m_inUse { 0 };

		// Increase the number of free entries available by BlockSize.
		// Aborts on out of memory.
		void Grow()
		{
			m_blocks.emplace_back();
			auto& newBlock = m_blocks.back();
			m_freeList.reserve(m_freeList.size() + BlockSize);
			for (size_t i = 0; i < newBlock.size(); ++i ) {
				m_freeList.push_back(&newBlock[0] + i);
			}
		}

	public:
		//! Default constructor. Optional argument specifies how many units to reserve at startup.
		Pool(size_t defaultReservation=BlockSize)
		{
			m_freeList.reserve(std::max(defaultReservation, BlockSize * 2));
			Reserve(defaultReservation);	// Pre-allocate the first units.
		}

		//! Free all of our existing allocations.
		void Cleanup() noexcept
		{
			m_freeList.clear();
			m_blocks.clear();
		}

		~Pool() noexcept
		{
#if defined(MEMORY_DEBUG)
			ABORT_IF(m_blocks.empty() == false);
#endif
		}

		//! Ensure at least minAvailable free entries are available.
		void Reserve(size_t minAvailable)
		{
			while (m_freeList.size() < minAvailable)
				Grow();
		}

		//! Return the new available free allocation, uninitialized.
		Type* Alloc()
		{
			if (PN_UNLIKELY(m_freeList.empty()))
				Grow();
			Type* block = m_freeList.back();
			m_freeList.pop_back();
			++m_inUse;
			return block;
		}

		//! Return an Alloc()d allocation to the pool.
		//! @warn Does not verify the allocation came from the pool.
		void Free(Type* block)
		{
			if ( PN_LIKELY( block ) ) {
				m_freeList.push_back(block);
				--m_inUse;
			}
		}

		//! Obtain the next free allocation and in-place default new it.
		Type* New()
		{
			Type* newBlock = Alloc();
			new (newBlock)Type();
			return newBlock;
		}

		//! Obtain the next free allocation and in-place-new it.
		template<typename... Args>
		Type* New(Args&&... args)
		{
			Type* newBlock = Alloc();
			new (newBlock) Type(std::forward<Args>(args)...);
			return newBlock;
		}

		//! Return a New()'d allocation to the pool.
		void Delete(Type* ptr) noexcept
		{
			ptr->~Type();
			Free(ptr);
		}

		// Disabled behaviors.
		Pool(const Pool& rhs) = delete;
		Pool(Pool&& rhs) = delete;
		Pool& operator = (const Pool& rhs) = delete;
		Pool& operator = (Pool&& rhs) = delete;
	};
}
