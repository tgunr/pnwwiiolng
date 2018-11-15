local attempts = { "Antwerp", "Schilde", "Oostmalle", "Grobbendonk", "Geel", "Mol", "Lommel", "Leopoldsburg", "Peer", "achel", "Weert", "Maaseik", "Panningen", "Roermond West" }

function run ()
 for i in ipairs(attempts) do
   local name = attempts[i]
   local cp = strtGetCPByName(name)
   if cp == nil then
    print("No such cp: " .. name)
   end
   if cp.country ~= 1 then
     cp:changeOwnershipFromScript(WWII_ENGLAND, true, true)
   else
    print("CP "..cp.uOID.." is already "..cp.country)
   end
 end
end

run()

systemMessage("*Opened UK links to Roermond")

