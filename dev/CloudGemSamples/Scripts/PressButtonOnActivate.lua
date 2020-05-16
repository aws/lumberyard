----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
-- PressButtonOnActivate.lua: Presses a ui canvas button when its attached entity is activated.
-- 
-- To use:
-- 1. Create a new entity that starts as inactive
-- 2. Attach the lua script to the entity
-- 3. Pass in the ui canvas button name to press and file path to the canvas ui where the button lives
-- 4. Activating the entity will emulate pressing the button
----------------------------------------------------------------------------------------------------

local PressButtonOnActivate = 
{
    Properties = 
    {
        ButtonName = { default = "" },
        CanvasFilePath = { default = "" }, -- Relative path to project folder
        Delay = { default = 2, suffix = " sec" },
    },
}


-- Presses a UI Canvas button with the given ButtonName in the given CanvasFilePath
function PressButtonOnActivate:OnActivate()
    if self.Properties.ButtonName == "" then
        Debug.Log("PressButtonOnActivate failed: ButtonName cannot be empty")
        return
    end

    if self.Properties.CanvasFilePath == "" then
        Debug.Log("PressButtonOnActivate failed: CanvasFilePath cannot be empty")
        return
    end

    self.timeRemaining = self.Properties.Delay -- seconds
    self.tickBus = TickBus.Connect(self, 0)
end

function PressButtonOnActivate:OnTick(deltaTime, timePoint)
    self.timeRemaining = self.timeRemaining - deltaTime

    if self.timeRemaining <= 0 then		
        local canvasId = UiCanvasManagerBus.Broadcast.FindLoadedCanvasByPathName(self.Properties.CanvasFilePath)
        local buttonId = UiCanvasBus.Event.FindElementByName(canvasId, self.Properties.ButtonName)
        if buttonId == nil then
            Debug.Log("Could not find button id with name '"..self.Properties.ButtonName.."' in canvas with path '"..self.Properties.CanvasFilePath.."'")
            self.tickBus:Disconnect()
            return
        end

        UiCanvasBus.Broadcast.ForceEnterInputEventOnInteractable(buttonId)
        self.tickBus:Disconnect()
    end
end

return PressButtonOnActivate