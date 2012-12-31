-- Script to cancel all active objectives

done = 0
for cp in allCPs() do
  if cp:isAttackObjective(ALLIED) then
    cp:clearObjective(ALLIED)
    done = done + 1
  elseif cp:isAttackObjective(AXIS) then
    cp:clearObjective(AXIS)
    done = done + 1
  end
end

if done > 0 then print("Cancelled "..done.." objective(s)")
else print("No objectives to cancel") end
