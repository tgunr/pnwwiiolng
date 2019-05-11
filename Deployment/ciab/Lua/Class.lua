require "string"

function Class(defaults, baseclass)
  -- Here lies the hideous nastiness required to
  -- emulate class stuff in Lua.
  return setmetatable(defaults, {
						__call = function(self, init)
							return setmetatable(init or {}, { __index = defaults })
						end
					,	__index = baseclass
					})
end

--[[
	Example usage:

	require "Class"

	Foo = Class({
		['name'] = 'undefined'
	,	['size'] = 0
	,	onClick = function (self, item) print("DON'T CLICK ON " .. item) end
	})

	-- Note use of '{' and '}', *not* '(' and ')'.
	local foo = Foo{}
	foo.name = 'foo'
	
	local bar = Foo{['name'] = 'bar'}

	print(foo.name) -- prints 'foo'
	print(bar.name) -- prints 'bar'
	print(Foo.name) -- prints 'undefined'

	Bar = Class({ ['lettuce'] = 'no' }, Foo)

	a = Foo{}
	b = Bar{}
	c = Bar{['name'] = 'c', ['lettuce'] = 'yes'}

	function report(which)
		local x = _G[which]
		if not x then error("No such " .. which) end
		local n = x.name or "nil"
		local s = x.size
		if s == nil then s = -1 end
		local l = x.lettuce or "nil"
		print(string.format("%s: name = %s, size = %d, lettuce = %q", which, n, s, l))
	end

	report('a')
	report('b')
	report('c')
]]--
