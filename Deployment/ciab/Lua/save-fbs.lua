for cp in allCPs() do
 for link in allLinksFrom(cp) do
  linkback = link.endDepot.m_outgoingLink
  if link and link.firebase and linkback and linkback.firebase then
   if link.firebase:isOpen() then print("blowFirebase("..linkback.firebase.uOID..")")
   elseif linkback.firebase:isOpen() then print("blowFirebase("..link.firebase.uOID..")")
   end
  end
 end
end
