local isActive = true

local AWSBehaviorMapTest = {
}

local testData = {
    ["key1"] = "abc",
    ["key2"] = "def",
    ["key3"] = "12345"
}

function AWSBehaviorMapTest:OnActivate()
    local runTestEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestEventId)
end

function AWSBehaviorMapTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorMapTest not active")
        self:NotifyMainEntity("success")
        return
    end

    Debug.Log("Running map test")

    local allPassed = true
    local stringMap = StringMap()

    local tableSize = 0
    for key, value in pairs(testData) do
        stringMap:SetValue(key, value)
        tableSize = tableSize + 1
    end

    local key1Value = stringMap:GetValue("key1")
    if key1Value == testData["key1"] then
        Debug.Log("Lua AWSBehaviorMapTest: Key set and get success, got value " .. key1Value)
    else
        allPassed = false
        Debug.Log("Lua AWSBehaviorMapTest: Key set and get failed. Expected value was " .. testData["key1"] .. " but got " .. key1Value)
    end

    local hasKey = stringMap:HasKey("key3")
    if hasKey then
        Debug.Log("Lua AWSBehaviorMapTest: HasKey success")
    else
        allPassed = false
        Debug.Log("Lua AWSBehaviorMapTest: HasKey failed, unable to find expected key")
    end

    local size = stringMap:GetSize()
    if size == tableSize then
        Debug.Log("Lua AWSBehaviorMapTest: GetSize success, found " .. size .. " entries")
    else
        allPassed = false
        Debug.Log("Lua AWSBehaviorMapTest: GetSize failed, expected " .. tableSize .. " entries, ")
    end

    stringMap:RemoveKey("key2")
    local removedValue = stringMap:GetValue("key2")
    if removedValue == nil or removedValue == "" then
        Debug.Log("Lua AWSBehaviorMapTest: RemoveKey success")
    else
        allPassed = false
        Debug.Log("Lua AWSBehaviorMapTest: RemoveKey failed, expected nil value but got" .. removedValue)
    end

    removedTableSize = 0
    for key, value in pairs(testData) do
        local getVal = stringMap:GetValue(key)
        if getVal ~= nil and getVal ~= "" then
            removedTableSize = removedTableSize + 1
        end
    end

    if removedTableSize == tableSize then
        allPassed = false
        Debug.Log("Lua AWSBehaviorMapTest: Remove key failed, size of table didn't change")
    else
        if removedTableSize == tableSize - 1 then
            Debug.Log("Lua AWSBehaviorMapTest: RemoveKey success, table size of " .. removedTableSize .. " is correct")
        else
            allPassed = false
            Debug.Log("Lua AWSBehaviorMapTest: Remove key failed, expected table size of " .. tableSize - 1 .. "but got table size of " .. removedTableSize)
        end
    end

    if allPassed == true then
        self:OnSuccess()
    else
        self:OnError()
    end
end

function AWSBehaviorMapTest:OnDeactivate()
    self.GamePlayHandler:Disconnect()
end

function AWSBehaviorMapTest:OnSuccess()
    Debug.Log("Lua AWSBehaviorMapTest: All Tests Passed")
    self:NotifyMainEntity("success")
end

function AWSBehaviorMapTest:OnError()
    Debug.Log("Lua AWSBehaviorMapTest: One Or More Tests Failed")
    self:NotifyMainEntity("fail")
end

function AWSBehaviorMapTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end


return AWSBehaviorMapTest