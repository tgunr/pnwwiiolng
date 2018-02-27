--[[
 StringPool.Add( tableOfStrings )

  Adds the entries in tableOfStrings to the StringPool.
  Format:
   tableOfStrings = { ['stringid'] = { ['en'] = 'string', ['zh'] = 'string' ... }, ... }
   StringPool.Add( tableOfStrings)


 locstr( stringId, ... )

  Looks up a localized string entry from the multi-language string pool,
  StringPool, based on the stringId and the current HostLanguage
  setting (taken from STRAT_LANGUAGE enviroment variable; default: en).

  The lookup is basically: StringPool[stringId][HostLanguage][stringId

  If there is no "stringId" listed under HostLanguage, then it checks
  for StringPool[stringId][stringId].

  If that cannot be found, it will return the string "??HostLanguage/stringId??"

  Also, for non-translatable strings, it checks for an entry:
  StringPool[stringId]

  When called with additional arguments, assumes that the string
  has format options (see http://lua-users.org/wiki/StringLibraryTutorial
  for more details on how to use Lua format options).

  e.g.

  StringPool.Add({ ['en'] = { ['string1'] = "Name = %s, Playerid = %u" } })
  locstr('string1', "Oliver", 3204)
  output:
   Name = Oliver, Playerid = 3204

  Try to use "%u" for numbers you know are unsigned instead of %d (signed)
]]--

StringPool = {}

function StringPool.Add( tableOfStrings )
 for id, translations in pairs(tableOfStrings)
 do
  StringPool[id] = translations
 end
end

function locstr(stringId, arg1, ...)
 -- Look up our language
 local stringEnt = StringPool[stringId]
 if stringEnt == nil then
  return error("locstr: StringPool["..stringId.."] does not have entries for '"..HostLanguage.."'")
 end

 -- If stringEnt is itself a string, it is non-translatable
 local locString
 if type(stringEnt) == 'table' then
  if stringEnt[HostLanguage] ~= nil then
	locString = stringEnt[HostLanguage]
  elseif HostLanguage ~= 'en' and stringEnt['en'] ~= nil then
    warn("locstr: Using default 'en' entry for missing "..stringId.."/"..HostLanguage.." string")
	locString = stringEnt['en']
  else
   error("locstr: No string with id " .. stringId)
   return "??"..stringId.."/"..HostLanguage.."??"
  end
 end

 if arg1 then
  return string.format(locString, arg1, ...)
 end

 return locString
end


--[[ -- Localization Code Testing

StringTableExample =
{
	['test1'] = -- Simple string test
	{
		['en'] = "en: Hello World", ['zh'] = "zh: Hello World"
	}
,	['test2'] = -- Fall thru test
	{
		-- Chinese translation mission to test fall thru
		['en'] = "en: Test 2"
	}
,	['test3'] = -- Single argument format test
	{
		['en'] = "en: Hello, Mr %s", ['zh'] = "zh: Hello, Mr %s"
	}
,	['test4'] = -- Multiple argument format test
	{
		['en'] = "en: Hello, Mr %s, how are the %u kids?"
	,	['zh'] = "en: Hello, Mr %s, how are the %u kids?"
	}
}

function StringTableTest(id, ...)
	print("- "..id.."/"..HostLanguage..": " .. locstr(StringTableExample, id, ...))
end

function StringTableTests(language)
	HostLanguage = language
	StringTableTest("test1")
	StringTableTest("test2", "FAIL")
	StringTableTest("test3", "Tester")
	StringTableTest("test4", "Tested", 5)
	pcall(StringTableTest, "test5", "This should fail")
end

StringTableTests("en")
StringTableTests("zh")
StringTableTests("xx")
]]--

