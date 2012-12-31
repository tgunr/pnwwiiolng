----------------------------------------------------------------------
-- CP event triggers
----------------------------------------------------------------------

-- Called when a CP has just left contention

function cpLeftContention(cp)
 return false
end

-- Called when a CP has just entered contention

function cpEnteredContention(cp)
 return false
end

-- Called when a CP starts its periodic run cycle

function cpStartRunCycle(cp)
 return false
end

-- Called when a CP ends its periodic run cycle

function cpEndRunCycle(cp)
 return false
end

-- Called when an attack objective is cleared

function cpClearedAO(cp)
 return false
end

-- Called when a CP changes control
function cpChangedControl(cp)
 CPMonitor:UpdateStatus(cp)
 return false
end

-- Called when a CP changes ownership

function cpChangedOwner(cp)
 -- Update the CPMonitor for this CP and its neighbors.
 CPMonitor:UpdateStatus(cp)
 for neighbor in allNeighbors(cp) do
	CPMonitor:UpdateStatus(neighbor)
 end
 return false
end

