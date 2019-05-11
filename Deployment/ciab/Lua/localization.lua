--[[
	Server-side localized strings.

	The string set used is determined by HostLanguage which is
	taken from the environment variable "STRAT_LANGUAGE".

	Note: You MUST NOT change the order of the "%" arguments, e.g.
		'Hello %s, you have %u minutes remaining'
	The %s and the %u must remain in the same order in translation.
]]--

require 'locstr'

StringPool.Add({

	-- Training Facility Names (or numbers)
	['ukTraining'] = { ['en'] = 'British Training', ['zh'] = 'B军训练' }
,	['frTraining'] = { ['en'] = 'French Training', ['zh'] = 'A军训练' }
,	['deTraining'] = { ['en'] = 'German Training', ['zh'] = 'C军训练' }

	-- The forts at Area 51
,	['ukFort'] = { ['en'] = 'Fort Montgomery', ['zh'] = '孟德利堡垒' }
,	['frFort'] = { ['en'] = 'Fort De Gaulle', ['zh'] = '迪高要塞' }
,	['deFort'] = { ['en'] = 'Fort Rommel', ['zh'] = '罗麦尔要塞' }

	----- auto-ao-placement strings.
	--

,	['aoStats'] =
	{
		-- Generate stats line, e.g. "5 ALLIED AOs available: 3 placed, 1 spare, 1 required"
		['en'] = "%u %s AOs available: %u placed, %u spare, %u required"
	,	['zh'] = "%u 个 %s AO可用: %u 个已部署, %u 个剩余, 必须部署 %u 个。"
	}
,	['aoInfo'] =
	{
		-- Prefix for the stats line on the HC Channel, "INFO: ...stats..."
		-- Sent when no action is required by the HC, just to let them know.
		['en'] = "INFO", ['zh'] = "信息"
	}
,	['aoNotice'] =
	{
		-- Prefix for when the HC need to notice the stats.
		['en'] = "NOTE", ['zh'] = "消息"
	}
,	['aoTimeMinutes'] =
	{
		['en'] = "%u minutes", ['zh'] = "%u 分"
	}
,	['aoTimeSeconds'] =
	{
		['en'] = "%u seconds", ['zh'] = "%u 秒"
	}
,	['aoAttention'] =
	{
		-- Prefix for when the system is planning action.
		['en'] = "ATTENTION", ['zh'] = "注意"
	}
,	['aoNewObjectiveSoon'] =
	{
		-- Warning to the HC that the system is going to place an AO if they don't
		['en'] = "Game Server will use a spare objective automatically in %s"
	,	['zh'] = "系统将在 %s 后自动部署AO"
	}
,	['aoPlacement'] =
	{
		-- To side channel when an auto-ao is being placed
		['en'] = "Automatic AO is being placed at %s"
	,	['zh'] = "系统自动在 %s 部署了AO"
	}
,	['aoCandidates'] =
	{
		-- HC message listing potential candidates.
		['en'] = "Current Auto-AO candidates are: %s (may change)"
	,	['zh'] = "当前自动部署AO的候选地点为: %s （可能会改变）"
	}

	---- bde-strength-check.lua
	--
	-- Next movement time messages for brigades

,	['bdeNextMoveHours'] = { ['en'] = 'Can move in %u hours', ['zh'] = '4 小时之后，才能移动' }
,	['bdeNextMoveHour'] = { ['en'] = 'Can move in 1 hour', ['zh'] = '1 小时之后，才能移动' }
,	['bdeNextMoveMinutes'] = { ['en'] = 'Can move in %u minutes', ['zh'] = '%u 分之后，才能移动' }
,	['bdeNextMoveNow'] = { ['en'] = 'Can be moved now', ['zh'] = '现在可以移动' }

	---- events.lua
	-- Warning that a town is in uprising and will change sides in a certain amount of time.
	-- Displayed as pocketTurnWarnPfx: pocketTurnWarnTime
	-- e.g. Pocket warning for Antwerp: Automatic surrender to enemy in 5 minutes.

,	['pocketTurnWarnPfx'] = { ['en'] = 'Pocket warning for %s' }
,	['pocketTurnWarnTimeH'] = { ['en'] = 'Automatic surrender to enemy in %u hours.' }
,	['pocketTurnWarnTimeM'] = { ['en'] = 'Automatic surrender to enemy in %u minutes.' }
,	['pocketTurnWarnTimeS'] = { ['en'] = 'Automatic surrender to enemy in %u seconds.' }

	---- ewscheck.lua
	--
	-- 'INTEL' prefix (short for "Intelligence report")
,	['ewsIntel'] = { ['en'] = 'INTEL', ['zh'] = '情报' }
,	['ewsSoldiersAt'] =
	{
		-- Soldiers needed at one cp
		['en'] = 'Soldiers needed at %s.'
	,	['zh'] = '%s 需要士兵支援。'
	}
,	['ewsSoldiersAtAnd'] =
	{
		-- Soldiers needed at two CPs
		['en'] = 'Soldiers needed at %s and %s!'
	,	['zh'] = '%s 和 %s 需要士兵支援！'
	}

	--- threat-level.lua
	--
	-- "heavy-infantry" label
,	['threatHeavyInfantry'] = { ['en'] = 'hvy-inf' }
	-- some infantry, but not heavy
,	['threatLightInfantry'] = { ['en'] = 'inf' }
	-- there must always be infantry, so armor is always a
	-- secondary token, e.g. infantry+armor or infantry & heavy-infantry.
	-- character(s) used to separate the infantry from the armor level
,	['threatSeparator'] = { ['en'] = '&' }
	-- "heavy-armor" label
,	['threatHeavyArmor'] = { ['en'] = 'hvy-armor' }
	-- "light-armor" label
,	['threatLightArmor'] = { ['en'] = 'armor' }
	
})

