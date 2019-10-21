cp = getCP("Kalmthout")

function checkLinks (cp)
 local link
 for link in allLinksFrom(cp) do
  if link.firebase ~= nil then
   print("Checking "..link.name)
   if link.firebase:isOpen() == false and link.endDepot.m_outgoingLink.firebase:isOpen() == false then
    print("Processing "..link.name)
    link:processFirebases()
   end
  end
 end
end

checkLinks(cp)
