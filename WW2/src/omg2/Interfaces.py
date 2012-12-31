# Interfaces that can be applied when we have a particular function.

difInterfacesByFunc = dict()

class Interface:
#{
	""" Describes an interface and type/accessor dependency it requires. """

	#------------------------------------------------------------
	def __init__(self, strName, strReturnType, bIsRef, strFnName):
	#{
		""" Constructor """
		self.strName, self.strReturnType, self.bIsRef, self.strFnName = strName, strReturnType, bIsRef, strFnName
		global difInterfacesByFunc
		difInterfacesByFunc[strFnName] = self
	#}

	#------------------------------------------------------------
	def qualifiedName(self):
	#{
		""" Return the fully qualified type name """
		return "WWII::Traits::" + self.strName
	#}

	#------------------------------------------------------------
	def Lookup(strName, strReturnType):
	#{
		""" Look up an interface function + type in the interfaces table. """
		global difInterfacesByFunc
		ifResult = difInterfacesByFunc(strName)
		if not ifResult or ifResult.strReturnType != strReturnType:
			return None
		return ifResult
	#}

#}

Interface('ITeulIndex', 'SINT32', False, 'teulIndex')
Interface('IPlayerID', 'SINT32', False, 'playerID')
Interface('IChatHandle', 'ChatHandle', True, 'chatHandle')
Interface('IInstanceID', 'Instance::UniqueID', True, 'instanceID')
Interface('MissionID', 'UINT32', False, 'missionID')
Interface('BrigadeID', 'UINT32', False, 'brigadeID')
Interface('SortieID', 'UINT32', False, 'sortieID')

