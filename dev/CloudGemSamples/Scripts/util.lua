local utility = {}

local function SetMouseCursorVisible(MouseCursorVisible)    
    if MouseCursorVisible then
        if Platform.Current ~= Platform.Android and Platform.Current ~= Platform.iOS then
            LyShineLua.ShowMouseCursor(MouseCursorVisible)
        end
    end
end

utility.SetMouseCursorVisible = SetMouseCursorVisible

return utility