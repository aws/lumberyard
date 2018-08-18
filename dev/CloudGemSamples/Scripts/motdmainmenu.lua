motdmainmenu =
{
    Properties =
    {
    },
}

function motdmainmenu:OnActivate()
	-- For handling tick events.
    self.tickBusHandler = TickBus.Connect(self,self.entityId);

	self.messageQueue = {}
	self.displayTimer = 0.0

    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/MsgOfTheDaySample/UI/MOTDMenu.uicanvas")

    -- Listen for action strings broadcast by the canvas
	self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)

    self.notificationHandler = CloudGemMessageOfTheDayNotificationBus.Connect(self, self.entityId)

end

function motdmainmenu:OnTick(deltaTime,timePoint)

	local queueSize = table.getn(self.messageQueue)
	if(queueSize > 0) then
		self.displayTimer = self.displayTimer - deltaTime
		if(self.displayTimer <= 0.0) then
			highestPriorityIndex = 1
			if(queueSize > 1) then
				for i = 2, table.getn(self.messageQueue) do
					if(self.messageQueue[i].priority < self.messageQueue[highestPriorityIndex].priority)then
						highestPriorityIndex = i
					end
    			end
			end
			Debug.Log("Highest prio:".. highestPriorityIndex)
			Debug.Log("Message: " .. tostring(self.messageQueue[highestPriorityIndex].priority));
			self:UpdateMessage(self.messageQueue[highestPriorityIndex].message)
			table.remove(self.messageQueue, highestPriorityIndex)
			self.displayTimer = 3.0
		end
	end
end

function motdmainmenu:OnDeactivate()
    self.notificationHandler:Disconnect();
    self.tickBusHandler:Disconnect();
	self.buttonHandler:Disconnect()
end

function motdmainmenu:OnGetPlayerMessagesRequestSuccess(response)
    Debug.Log("GetPlayerMessages succeeded")
    self.messageQueue = {}
    self.displayTimer = 0.0
    Debug.Log("Response messages: "..tostring(#response.list));
    --This is a callback from C++ with an object containing a vector called list.
    --Therefore we cannot treat it as a regular Lua table and must rely on the reflected methods and operators of the reflected vector class
    --for msgCount = 1, table.getn(response) do
    --    Debug.Log(response[msgCount])
    --    table.insert(self.messageQueue, response[msgCount])
    for msgCount = 1, #response.list do
        Debug.Log(tostring(response.list[msgCount].message))
		table.insert(self.messageQueue, response.list[msgCount])

    end
end

function motdmainmenu:OnGetPlayerMessagesRequestError(errorMsg)
    Debug.Log("GetPlayerMessages Error")
end

function motdmainmenu:OnAction(entityId, actionName)
    Debug.Log(tostring(entityId) .. ": " .. actionName)

    if actionName == "GetMessages" then
        if CloudGemMessageOfTheDayRequestBus.Event == nil then
            Debug.Log("No Message Request Event found")
            return
        end

        local timeVal = os.date("!%b %d %Y %H:%M")
        local lang = "Eng"
        Debug.Log(timeVal);
        CloudGemMessageOfTheDayRequestBus.Event.GetPlayerMessages(self.entityId, timeVal, lang, nil)
    end
end

function motdmainmenu:UpdateMessage(msgText)
    local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "MessageText")
    UiTextBus.Event.SetText(displayEntity, msgText)
end

return motdmainmenu