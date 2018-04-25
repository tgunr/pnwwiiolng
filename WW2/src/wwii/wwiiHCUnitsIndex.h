#pragma once

#include <limits>

class HCUnit;
class StratHCUnit;


using PackedHCUnitID = uint16_t; // Only need 16 bits for max unit ID
enum class HCUnitID : unsigned { null = 0, max = std::numeric_limits<PackedHCUnitID>::max() };

// This just means the ID *may* point to a valid unit. The unit may still not exist or not be tracked locally.
inline bool IsValid(HCUnitID id) { return id != HCUnitID::null && id <= HCUnitID::max; }
inline bool IsInvalid(HCUnitID id) { return ! IsValid(id); }

using HCUNIT_INDEX = std::map<HCUnitID, HCUnit*>;
using HCUIDX_ITERATOR = HCUNIT_INDEX::iterator;
