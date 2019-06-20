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


Cloud = {
  type = "Cloud",
	ENTITY_DETAIL_ID=1,

	Properties = {
		file_CloudFile = "Libs/Clouds/Default.xml",
		fScale = 1,
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
function Cloud:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function Cloud:OnInit()
	self:NetPresent(0);
	self:CreateCloud();
end

-------------------------------------------------------
function Cloud:SetMovementProperties()
	local moveparams = self.Properties.Movement;
	self:SetCloudMovementProperties(0, moveparams);
end

-------------------------------------------------------
function Cloud:OnShutDown()
end

-------------------------------------------------------
function Cloud:CreateCloud()
	local props = self.Properties;	
	self:LoadCloud( 0,props.file_CloudFile );
	self:SetMovementProperties();
end

-------------------------------------------------------
function Cloud:DeleteCloud()
	self:FreeSlot( 0 );
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function Cloud:OnReset()
	local bCurrentlyHasCloud = self:IsSlotValid(0);
	if (not bCurrentlyHasCloud) then
		self:CreateCloud();
	end
end

-------------------------------------------------------
function Cloud:OnPropertyChange()
	local props = self.Properties;
	if (props.file_CloudFile ~= self._CloudFile) then
		self._CloudFile = props.file_CloudFile;
		self:CreateCloud();
	else
		self:SetMovementProperties();
	end
end

-------------------------------------------------------
function Cloud:DeleteCloud()
	self:FreeSlot( 0 );
end

-------------------------------------------------------
function Cloud:OnSave(tbl)
	tbl.bHasCloud = self:IsSlotValid(0);
	if (tbl.bHasCloud) then
	  -- todo: save pos
	end
end

-------------------------------------------------------
function Cloud:OnLoad(tbl)
	local bCurrentlyHasCloud = self:IsSlotValid(0);
	local bWantCloud = tbl.bHasCloud;
	if (bWantCloud and not bCurrentlyHasCloud) then
		self:CreateCloud();
	  -- todo: restore pos
	elseif (not bWantCloud and bCurrentlyHasCloud) then
		self:DeleteCloud();	
	end
end

-------------------------------------------------------
-- Hide Event
-------------------------------------------------------
function Cloud:Event_Hide()
	self:DeleteCloud();
end

-------------------------------------------------------
-- Show Event
-------------------------------------------------------
function Cloud:Event_Show()
	self:CreateCloud();
end

Cloud.FlowEvents =
{
	Inputs =
	{
		Hide = { Cloud.Event_Hide, "bool" },
		Show = { Cloud.Event_Show, "bool" },
	},
	Outputs =
	{
		Hide = "bool",
		Show = "bool",
	},
}
