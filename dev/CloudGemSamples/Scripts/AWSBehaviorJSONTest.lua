local isActive = true

local AWSBehaviorJSONTest = {
}

local testJSONString = [[
{
    "glossary": {
        "title": "example glossary",
        "GlossDiv": {
            "title": "S",
            "GlossList": {
                "GlossEntry": {
                    "ID": "SGML",
                    "SortAs": "SGML",
                    "GlossTerm": "Standard Generalized Markup Language",
                    "Acronym": "SGML",
                    "Abbrev": "ISO 8879:1986",
                    "GlossDef": {
                        "para": "A meta-markup language, used to create markup languages such as DocBook.",
                        "GlossSeeAlso": ["GML", "XML"]
                    },
                    "GlossSee": "markup"
                }
            }
        }
    }
}
]]

function AWSBehaviorJSONTest:OnActivate()
    local runTestEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.gamePlayHandler = GameplayNotificationBus.Connect(self, runTestEventId)
end

function AWSBehaviorJSONTest:OnEventBegin(message)
     if isActive == false then
        Debug.Log("AWSBehaviorHTTPTest not active")
        self:NotifyMainEntity("success")
        return
    end

    Debug.Log("Running JSON test")
    local allPassed = true

    local jsonObject = JSON()

    jsonObject:FromString(testJSONString)

    Debug.Log("Test: JSON object correctly parsed")
    Debug.Log(jsonObject:LogToDebugger())

    Debug.Log("Test: Parse JSON example")
    local testObject = jsonObject:IsObject()
    Debug.Log("Test: IsObject should be true - " .. tostring(testObject))
    if testObject ~= true then
        allPassed = false
    end

    jsonObject:EnterObject("glossary")
    jsonObject:EnterObject("title")

    local testBool = jsonObject:IsString()
    Debug.Log("Test: is string should be true - " .. tostring(testBool))
    if testBool ~= true then
        allPassed = false
    end

    local testString = jsonObject:GetString()
    Debug.Log("Test: title, should be 'example glossary' - " .. testString)
    if testString ~= 'example glossary' then
        allPassed = false
    end

    testBool = jsonObject:IsInteger()
    Debug.Log("Test: is integer should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    local testInt = jsonObject:GetInteger()
    Debug.Log("Test: title as integer, should be 0 and report error in console - " .. tostring(testInt))
    if testInt ~= 0 then
        allPassed = false
    end

    testBool = jsonObject:IsBoolean()
    Debug.Log("Test: is boolean should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    testBool = jsonObject:GetBoolean()
    Debug.Log("Test: title as boolean, should be false and report error in console - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    testBool = jsonObject:IsDouble()
    Debug.Log("Test: is double should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    local testDouble = jsonObject:GetDouble()
    Debug.Log("Test: title as double, should be 0 and report error in console - " .. tostring(testDouble))
    if testDouble ~= 0 then
        allPassed = false
    end

    testBool = jsonObject:IsObject()
    Debug.Log("Test: is object should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    testBool = jsonObject:IsArray()
    Debug.Log("Test: is array should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end


    jsonObject:ExitCurrentObject()
    Debug.Log("Test: Exit title object, testing for containing object, should be true - " .. tostring(jsonObject:IsObject()))

    jsonObject:EnterObject("FooBar")
    Debug.Log("Test: Attempt to enter invalid object - passed")

    jsonObject:EnterObject("GlossDiv")
    jsonObject:EnterObject("GlossList")
    jsonObject:EnterObject("GlossEntry")
    jsonObject:EnterObject("GlossDef")
    jsonObject:EnterObject("GlossSeeAlso")

    local testArray = jsonObject:IsArray()
    Debug.Log("Test: Object is array, should be true - " .. tostring(testArray))
    if testArray ~= true then
        allPassed = false
    end

    local testArraySize = jsonObject:EnterArray()
    Debug.Log("Test: Enter array, array size should be 2 - " .. tostring(testArraySize))
    if testArraySize ~= 2 then
        allPassed = false
    end

    testString = jsonObject:GetString()
    Debug.Log("Test: array element one should be 'GML' - " .. testString)
    if testString ~= "GML" then
        allPassed = false
    end

    testBool = jsonObject:NextArrayItem()
    testString = jsonObject:GetString()
    Debug.Log("Test: array element two should be 'XML' - " .. testString)
    if testString ~= "XML" then
        allPassed = false
    end

    testBool = jsonObject:NextArrayItem()
    Debug.Log("Test: should be last element of array, return value should be false - " .. tostring(testBool))
    if testBool ~= false then
        allPassed = false
    end

    jsonObject:ExitArray()
    testBool = jsonObject:IsArray()
    Debug.Log("Test: exit array should take us out of iteration to the array object - " .. tostring(testBool))
    if testBool ~= true then
        allPassed = false
    end

    jsonObject:ExitCurrentObject()
    testBool = jsonObject:IsObject()
    Debug.Log("Test: exit current object should put us back in array parent object object - " .. tostring(testBool))
    if testBool ~= true then
        allPassed = false
    end

    if allPassed == true then
        self:OnSuccess()
    else
        self:OnError()
    end
end

function AWSBehaviorJSONTest:OnDeactivate()
    self.gamePlayHandler:Disconnect()
end

function AWSBehaviorJSONTest:OnSuccess()
    Debug.Log("Lua AWSBehaviorJSONTest: All Tests Passed")
    self:NotifyMainEntity("success")
end

function AWSBehaviorJSONTest:OnError()
    Debug.Log("Lua AWSBehaviorJSONTest: One Or More Tests Failed")
    self:NotifyMainEntity("fail")
end

function AWSBehaviorJSONTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorJSONTest