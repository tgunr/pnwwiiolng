#pragma once
// Created 2011/9/20 13:24:01 CDT
// Copyright (C) PlayNet Inc 2011 -- All rights reserved.
// Not for redistribution except as part of an API.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapSupercellVislevelXfer.h"
#include "strt/strtSupercell.h"				// For Supercell.
#include "mem/mem.h"
#include <map>

#pragma pack(push, 1)
//! @class HostSupercellVislevelXfer
//! @brief Host extension of Map::SupercellVislevelXfer
class HostSupercellVislevelXfer : public SupercellVislevelXfer
{
private:	void* operator new(size_t);		// Prohibited: See "allocate" instead.

			//! Associate this object with the given supercell.
public:		void associateWith(Supercell& sc, const size_t dataSize_bytes)
			{
				assert(dataSize_bytes < (size_t)std::numeric_limits<UINT16>::max());
				m_size_bytes = static_cast<UINT16>(dataSize_bytes);
				m_scx = sc.x;
				m_scy = sc.y;
				sc.m_xfer = this;
			}

			//! Update our copy of a building's vis level.
public:		bool setBuildingLevel(const UINT32 bid_, const vis_t visLevel_)
			{
				// Get pointer to which vis_t this building is in.
				vis_t* const ptr = getBuildingFlagPtr(bid_);

				// Calculate which bits of the vis_t it occupied.
				const size_t shift = getBitPositionInFlag(bid_);
				const vis_t visValue = visLevel_ << shift;
				const vis_t maskValue = ~(BitMask << shift);

				// Clear those bits and apply our new value.
				const vis_t oldValue = *ptr;
				*ptr = (*ptr & maskValue) | visValue;
				return ( *ptr != oldValue );
			}

			//! Calculate how much memory is required for a given Xfer.
public:		static size_t calculateBytesNeededFor(const Supercell& sc)
			{
				if ( sc.numObjects == 0 )
					return 0;

				static const size_t baseSize_bits = sizeof(decltype(m_size_bytes)) * 8 + SCXYBits;

				const size_t obj_bits = ((sc.numObjects * BitsPerObject) + 7) & ~7;
				const size_t dataSize_bits = baseSize_bits + obj_bits;

				return dataSize_bits / 8;
			}

			//! Size, allocate and populate a complete set of supercell data,
			//! given an iterator range of Supercell's
			//! Assumes an associative sequence.
			template<typename _ItrType>
			static std::pair<void*, size_t>
			allocateFor(_ItrType begin, _ItrType end, UINT32& numBuildings, UINT32& maxBuildings)
			{
				numBuildings = 0, maxBuildings = 0;

				// Calculate total size.
				size_t totalBytes = 0;
				for ( _ItrType it = begin; it != end; ++it )
				{
					const Supercell& sc = it->second;
					const size_t byteSize = calculateBytesNeededFor(sc);
					if ( byteSize != 0 )
						totalBytes += byteSize;
				}

				if ( totalBytes == 0 )
				{
					return std::make_pair<void*, size_t>(nullptr, 0);
				}

				// Allocate the memory.
				void* allocation = memCalloc(1, totalBytes);
				assert( allocation != nullptr );

				// Partition and populate.
				char* curPtr = (char*)allocation;
				for ( auto it = begin; it != end; ++it )
				{
					Supercell& sc = it->second;
					const size_t byteSize = calculateBytesNeededFor(sc);
					if ( byteSize <= 0 )
						continue;

					HostSupercellVislevelXfer* const xfer = (HostSupercellVislevelXfer*)curPtr;
					curPtr += byteSize;

					xfer->associateWith(sc, byteSize);

					const auto& numObjs = sc.numObjects;
					numBuildings += numObjs;
					if ( numObjs > maxBuildings )
						maxBuildings = numObjs;

					for ( size_t objNo = 0; objNo < numObjs; ++objNo )
					{
						ObjectStruct* const obj = &sc.objects[objNo];
						obj->m_bid = objNo;
						xfer->setBuildingLevel(objNo, obj->effectiveVisLevel());
					}
				}

				return std::make_pair(allocation, totalBytes);
			}
};
#pragma pack(pop)

