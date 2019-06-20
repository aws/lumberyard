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
----------------------------------------------------------------------------------------------------


VolumeObject = {
  type = "VolumeObject",
	ENTITY_DETAIL_ID = 1,

	Properties = {
		file_VolumeObjectFile = "Libs/Clouds/Default.xml",
		Movement = 
		{
			bAutoMove = 0,
			vector_Speed = {x=0,y=0,z=0},
			vector_SpaceLoopBox = {x=2000,y=2000,z=2000},
			fFadeDistance = 0,
		}
	},

	Editor = {
		Model = "Editor/Objects/Particles.cgf",
		Icon = "Clouds.bmp",
	},
}

-------------------------------------------------------
function VolumeObject:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function VolumeObject:OnInit()
	self:NetPresent(0);
	self:Create();
end

-------------------------------------------------------
function VolumeObject:SetMovementProperties()
	local moveparams = self.Properties.Movement;
	self:SetVolumeObjectMovementProperties(0, moveparams);
end

-------------------------------------------------------
function VolumeObject:OnShutDown()
end

-------------------------------------------------------
function VolumeObject:Create()
	local props = self.Properties;	
	self:LoadVolumeObject(0, props.file_VolumeObjectFile);
	self:SetMovementProperties();
end

-------------------------------------------------------
function VolumeObject:Delete()
	self:FreeSlot(0);
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function VolumeObject:OnReset()
	local bCurrentlyHasObj = self:IsSlotValid(0);
	if (not bCurrentlyHasObj) then
		self:Create();
	end
end

-------------------------------------------------------
function VolumeObject:OnPropertyChange()
	local props = self.Properties;
	if (props.file_VolumeObjectFile ~= self._VolumeObjectFile) then
		self._VolumeObjectFile = props.file_VolumeObjectFile;
		self:Create();
	else
		self:SetMovementProperties();
	end
end

-------------------------------------------------------
function VolumeObject:OnSave(tbl)
	tbl.bHasObject = self:IsSlotValid(0);
	if (tbl.bHasObject) then
	  -- todo: save pos
	end
end

-------------------------------------------------------
function VolumeObject:OnLoad(tbl)
	local bCurrentlyHasObject = self:IsSlotValid(0);
	local bWantObject = tbl.bHasObject;
	if (bWantObject and not bCurrentlyHasObject) then
		self:Create();
	  -- todo: restore pos
	elseif (not bWantObject and bCurrentlyHasObject) then
		self:Delete();	
	end
end

-------------------------------------------------------
-- Hide Event
-------------------------------------------------------
function VolumeObject:Event_Hide()
	self:Delete();
end

-------------------------------------------------------
-- Show Event
-------------------------------------------------------
function VolumeObject:Event_Show()
	self:Create();
end

VolumeObject.FlowEvents =
{
	Inputs =
	{
		Hide = { VolumeObject.Event_Hide, "bool" },
		Show = { VolumeObject.Event_Show, "bool" },
	},
	Outputs =
	{
		Hide = "bool",
		Show = "bool",
	},
}
