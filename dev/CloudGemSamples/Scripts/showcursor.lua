showcursor = {
}

function showcursor:OnActivate()
	local util = require("scripts.util")
	util.SetMouseCursorVisible(true)
	
end

return showcursor