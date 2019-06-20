
--------------------------------------------------------------------------
-- This is just a stub until the game rules in CryAction is removed.
-- All the real game rules are now in the GameRules prototypes.
--------------------------------------------------------------------------

DummyRules = {
	Client = {},
	Server = {},
}

function DummyRules.Server:OnStartLevel()
	CryAction.SendGameplayEvent(NULL_ENTITY, eGE_GameStarted);
end
