#pragma once
// Memory Pooling of objects.

namespace Mem
{
	// Creates a managed pool of instances of Type. The free list
	// grows BlockSize units of Type at a time. Provides C-style
	// Alloc and Free members along with C++ New members.
	template<class Type, size_t BlockSize = 1>
	class Pool
	{
		static_assert(BlockSize > 0, "BlockSize must be at least 1.");

		std::vector<Type*> m_blocks;
		std::vector<Type*> m_freeList;
		size_t m_pruneCandidate;

		// Increase the number of free entries available by BlockSize.
		// Aborts on out of memory.
		void Grow()
		{
			Type* const newBlock = static_cast<Type*>(memAlloc(sizeof(Type)* BlockSize));
			ABORT_IF(newBlock == nullptr);
			for (Type* node = newBlock + BlockSize - 1; node >= newBlock; --node)
				m_freeList.push_back(node);
			m_pruneCandidate = 0;
		}

		Pool(const Pool& rhs);				// Copy construction prohibited.
		Pool& operator=(const Pool& rhs);	// Copy operator prohibited.

	public:
		//! Default constructor. Optional argument specifies how many units to reserve at startup.
		Pool(size_t defaultReservation=BlockSize)
		{
			m_blocks.reserve(128);			// Boot-strap the allocations tracker.
			m_freeList.reserve(std::max(defaultReservation, BlockSize * 2));
			Reserve(defaultReservation);	// Pre-allocate the first units.
			m_pruneCandidate = 0;
		}

		//! Free all of our existing allocations.
		void Cleanup()
		{
			m_freeList.clear();
			std::for_each(m_blocks.begin(), m_blocks.end(), [](Type* block) { memFree(block); });
			m_blocks.clear();
		}

		~Pool()
		{
			ABORT_IF(m_blocks.empty() == false);
		}

		void Reserve(size_t minAvailable)
		{
			while (m_freeList.size() < minAvailable)
				Grow();
		}

		//! Return the new available free node, uninitialized.
		Type* Alloc()
		{
			if (PN_UNLIKELY(m_freeList.empty()))
				Grow();
			Type* node = m_freeList.back();
			m_freeList.pop_back();
			return node;
		}

		void Free(Type* node)
		{
			ABORT_IF(node == nullptr);
			m_freeList.push_back(node);
		}

		Type* New()
		{
			Type* newNode = Alloc();
			new (newNode)Type();
			return newNode;
		}

		Type* New(const Type& copy)
		{
			Type* newNode = Alloc();
			new (newNode)Type(copy);
			return newNode;
		}

		Type* New(Type&& rvalue)
		{
			Type* newNode = Alloc();
			new (newNode)Type(rvalue);
			return newNode;
		}

		void Delete(Type* ptr)
		{
			Free(ptr);
			ptr->~Type();
		}

	};
}
