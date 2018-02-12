---------------------------------------------------------------------
---------------------------------------------------------------------
-- Test Harness functions
---------------------------------------------------------------------
---------------------------------------------------------------------

---------------------------------------------------------------------
-- The WWII test class structure
wwiiTest = {}
function wwiiTest:new (name)
 o = {}
 setmetatable(o, self)
 self.__index = self
 self.name = name
 self.tests = 0
 self.oks = 0
 self.fails = 0
 self.result = "FAIL"
 self.results = {}
 return self
end

---------------------------------------------------------------------
function wwiiTest:run(name, expect, func, ...)
 local got,message = func(unpack(arg))
 if got == nil then got = "FAIL" end
 if message == nil then message = "[unexpected null message]" end
 local description = self.name..":"..name.."("..join(',',arg).."):Want="..expect..":Got="..got..":"..message
 self.tests = self.tests + 1
 local result
 if got == expect then
  self.oks = self.oks + 1
  result = "OK"
 else
  self.fails = self.fails + 1
  result = "FAIL"
 end
 table.insert(self.results, result..":"..description)
 return result, description
end

---------------------------------------------------------------------
function wwiiTest:finish ()
 local retval = false
 local desc = self.name..":"..self.oks.."/"..self.tests..":"..math.floor(self.oks/self.tests*100).."%"
 if self.fails == 0 then
  self.result = "OK" ; retval = true
 else
  for i=1,table.getn(self.results) do
   print(self.results[i])
  end
  print(self.result..":"..desc)
 end
 self.results = {}
 lastTestResult = result
 return self.result,desc
end

