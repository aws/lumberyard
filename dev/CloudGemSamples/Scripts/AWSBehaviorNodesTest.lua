AWSBehaviorNodesTest = {}

local testButtons = {
    "btnTest01",
    "btnTest02",
    "btnTest03",
    "btnTest04",
    "btnTest05",
    "btnTest06",
    "btnTest07",
    "btnTest08",
    "btnTest09"
}

function AWSBehaviorNodesTest:OnActivate()
    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/AWSBehaviorExamples/UI/AWSBehaviorExamples.uicanvas")
    -- Listen for action strings broadcast by the canvas
    self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    -- Connect our EBus to listen for test entity events
    self.runTestsEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.gamePlayHandler = GameplayNotificationBus.Connect(self, self.runTestsEventId)
    
    -- Display the mouse cursor
    LyShineLua.ShowMouseCursor(true)

    self.tag = "Prerequisite"
    self:InitializeEntityList()

    self.tickBus = TickBus.Connect(self, 0)
end

function AWSBehaviorNodesTest:OnTick(deltaTime, timePoint)
    -- The buttons can't be set up until all of the entities with the tests have been activated
    -- This is a horrible hack to wait one game tick so all are activated
    -- There is a better way to do this with the tag system and waiting for tagged items to
    -- be activated so consider this 
    -- TODO make this use the tag activation system and remove this monstrosity
    self:SetUpSingleTestButtons()
    self.tickBus:Disconnect()
end

function AWSBehaviorNodesTest:OnDeactivate()
    self.buttonHandler:Disconnect()
    self.gamePlayHandler:Disconnect()
end

function AWSBehaviorNodesTest:OnAction(entityId, actionName)
    if actionName == "AWSNodesTest" then
        self.runAllTestsButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "btnAllTests")
        UiInteractableBus.Event.SetIsHandlingEvents(self.runAllTestsButton, false)

        self.isSingleTest = false
        self:InitializeEntityList()
        self:StartNextTest()
    else
        self.singleBehavior[actionName]()
    end
end

function AWSBehaviorNodesTest:OnEventBegin(message)

    if self.isSingleTest == true then
        self.isSingleTest = true;
        if message == "success" then
            Debug.Log("Test successful")
        else
            Debug.Log("Test failed")
        end
    else
        --Get the current test entity name
        local entityName = GameEntityContextRequestBus.Broadcast.GetEntityName(self.entities[self.count])

        if message == "success" then
            Debug.Log(tostring(entityName).." succeeded.")
            self:StartNextTest()        
        else
            Debug.Log(tostring(entityName).." failed.")
            UiInteractableBus.Event.SetIsHandlingEvents(self.runAllTestsButton, true)
        end
    end
end

function AWSBehaviorNodesTest:StartNextTest()
    if self.count < #self.entities then
        self.count = self.count + 1          
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entities[self.count], "Run Tests", typeid("")), "")

        if self.count == #self.entities then
            UiInteractableBus.Event.SetIsHandlingEvents(self.runAllTestsButton, true)
        end
    else
        if self.tag == "Prerequisite" then
            self.tag = "Test Entity"
            self:InitializeEntityList()
            self:StartNextTest()
        end
    end
end

function AWSBehaviorNodesTest:InitializeEntityList()
    self.entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.tag))}
    self.count = 0
end

function AWSBehaviorNodesTest:SetUpSingleTestButtons()
    self.singleBehavior = {}
    self.isSingleTest = false
    local stEnts = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Click Test"))}
    for i,v in ipairs(stEnts) do
        local entityName = GameEntityContextRequestBus.Broadcast.GetEntityName(v)
        Debug.Log("Found entity " .. entityName)
        local testBtn = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, testButtons[i])
        local textElement = UiElementBus.Event.FindChildByName(testBtn, "Text")
        UiTextBus.Event.SetText(textElement, entityName)
        self.singleBehavior[testButtons[i]] = function()
            self.isSingleTest = true
            local entity = v
            local capturedEnityName = entityName
            Debug.Log("Starting test on entity " .. capturedEnityName)
            GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entity, "Run Tests", typeid("")), "")
        end
    end
end

return AWSBehaviorNodesTest
