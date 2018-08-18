local utility = {}

local function SetMouseCursorVisible(MouseCursorVisible)    
    if MouseCursorVisible then
        if Platform.Current ~= Platform.Android and Platform.Current ~= Platform.iOS then
            LyShineLua.ShowMouseCursor(MouseCursorVisible)
        end
    end
end

utility.SetMouseCursorVisible = SetMouseCursorVisible

local function HasValue(table, val)
    for index, value in ipairs(table) do
        if value == val then
            return true
        end
    end

    return false
end

utility.HasValue = HasValue

local function UnserializeStringToArray(serializedString)
    subString = string.sub(serializedString, 2, string.len(serializedString) - 1)..","   
    array = {}

    for value in subString:gmatch("\"(.-)\",") do
        array[#array + 1] = value
    end

    return array
end

utility.UnserializeStringToArray = UnserializeStringToArray

return utility