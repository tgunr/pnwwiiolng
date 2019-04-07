print("Finished removing all Garrisons and Brigades from the Map")

for unit in allStratHCUnits() do
  undeploy(unit)
end