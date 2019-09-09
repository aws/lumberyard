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

local function DeserializeString(str, customFieldSchema)
    if customFieldSchema["type"] == "text" or (customFieldSchema["type"] == "predefined" and not customFieldSchema["multipleSelect"]) then
        return str
    elseif customFieldSchema["type"] == "predefined" and customFieldSchema["multipleSelect"] then
        return utility.ConvertStringToArray(str)
    elseif customFieldSchema["type"] == 'object' then
        return utility.ConvertStringToTable(str)
    end
    return response
end

utility.DeserializeString = DeserializeString

local function ConvertStringToArray(str)
    --Remove the square brackets on both sides
    local subStr = str:sub(2, str:len() - 1)

    array = {}
    for value in subStr:gmatch("[^\",]+") do
        array[#array + 1] = value
    end

    return array
end

utility.ConvertStringToArray = ConvertStringToArray

local function convertArrayToString(array)
    local str = "["..table.concat(array, ",").."]"
    return str
end

utility.convertArrayToString = convertArrayToString

local function ConvertStringToTable(str)
    if str == "{}" then
        return {}
    end

    --Remove the braces on both sides
    local subStr = str:sub(2, str:len() - 1)

    --Lua pattern doesn't support negative lookahead, which makes it dificult to split each property by commas that are not winthin square brackets.
    ---Replace all the substrings winthin square brackets with another string which doesn't contain comma
    local arrays = {}
    for part in subStr:gmatch('[\[][^\][]*]') do
        arrays[#arrays] = part
    end
    local subStr = subStr:gsub('[\[][^\][]*]', 'arrayObject')

    response = {}
    local arrayObjectIndex = 0
    for part in subStr:gmatch('[^,]+') do
        key, value = part:match("\"([^\"]+)\":([^:]+)")
        if value ~= 'arrayObject' then
            response[key] = value == "\"\"" and "" or value:sub(2, value:len() - 1)
        else
            response[key] = arrays[arrayObjectIndex]
            arrayObjectIndex = arrayObjectIndex..1
        end 
    end
    return response
end
utility.ConvertStringToTable = ConvertStringToTable

local function converTableToString(obj)
    local response = "{"
    for key, value in pairs(obj) do
        response = response.."\""..key.."\":"
        response = type(value) == "table" and response..utility.convertArrayToString(value) or response.."\""..value.."\""
        response = response..","
    end

    if (response ~= "{") then
        response = response:sub(1, response:len() - 1)
    end
    response = response.."}"
    return response
end
utility.converTableToString = converTableToString

return utility