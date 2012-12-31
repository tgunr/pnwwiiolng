#! /usr/bin/python -OO

from Class import *

# Test case.
c = Class("Vehicle", "Update", strBrief="Example class", strDetail="This is an example OMG test class.")
c.addMember(Member(strName="hostIndex", strType="UINT32", strDefault="0", strBrief="Host's identifier for this vehicle"))

afnP = c.findFunction(strName='packer')
if len(afnP) == 0:
	raise Exception("No packer found in test class.")
elif len(afnP) > 1:
	raise Exception("Found %u packers." % len(afnP))
else:
	print("[./] Found a packer. It takes %s as arguments" % afnP[0].strArgs)

strDeclaration = c.declaration()
if not strDeclaration:
	raise Exception("Declaration generation FAILED.")
print("[./] Declaration generated.")

strDefinition = c.definition()
if strDefinition:
	raise Exception("Definition generated with no definition-requiring members.")
print("[./] Simple class generated no definition.")

c.addFunction(Function(strName="testMe", strArgs='FULL_OBJ_ID& foo', strCode='printf("Hello world\\n") ;', strReturn='void', strAccess='public', bConst=True, bInline=False))
if c.bFinalized == True:
	raise Exception("Adding an extra function didn't clear bFinalized.")
print("[./] We reset Finalized when we added a function.")

info = Class("", "Info", strBrief="Info sub-class", strDetail="This is a sub-class of the Vehicle::Update class.")
if info.clsParent != None:
	raise Exception("Info already appears to have a parent.")
c.addClass(info)
if c.bFinalized == True:
	raise Exception("Adding an extra class didn't clear bFinalized.")
if info.clsParent != c:
	raise Exception("Adding Info to Update didn't assign Update as Info's parent")
if c.clsParent != None:
	raise Exception("Adding Info to Update changed Update's parent!")
print("[./] We added 'info' to 'update'")
info.addMember(Member(strName='handle', strType='ChatHandle', strDefault='0', strBrief="Gamename for this vehicle."))
if not strDeclaration:
	raise Exception("Declaration generation FAILED.")
print("[./] Declaration regenerated.")

c.addMember(Member(strName="info", strType='Info', strBrief='Update Information'))

strDeclaration = c.declaration()
strDefinition = c.definition()
if not strDefinition:
	raise Exception("No definition generated after adding complex function.")
print("[./] Definition generated after adding def-req function.")

print("8x --- snip --- header file --- snip --- x8")
print(strDeclaration)

print("8x --- snip --- source file --- snip --- x8")
print(strDefinition)

