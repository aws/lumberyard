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
-- Original file Copyright Crytek GMBH or its affiliates, used under license.
--
----------------------------------------------------------------------------------------------------
Cloth =
{
	Server = {},
	Client = {},

	Properties =
	{
		mass = 5,
		density = 200,
		fileModel="Objects/props/misc/cloth/cloth.cgf",
		gravity={x=0,y=0,z=-9.8},
		damping = 0.3,
		max_time_step = 0.02,
		sleep_speed = 0.01,
		thickness = 0.06,
		friction = 0,
		hardness = 20,
		air_resistance = 1,
		wind = {x=0,y=0,z=0},
		wind_event = {x=0,y=10,z=0},
		wind_variance = 0.2,
		max_iters = 20,
		accuracy = 0.05,
		water_resistance = 600,
		impulse_scale = 0.02,
		explosion_scale = 0.003,
		collision_impulse_scale = 1.0,
		max_collision_impulse = 160,
		mass_decay = 0,
		attach_radius = 0,
		bCollideWithTerrain = 0,
		bCollideWithStatics = 1,
		bCollideWithPhysical = 1,
		bCollideWithPlayers = 1,
		bRigidCore = 0,
		max_safe_step = 0.2,
		MultiplayerOptions = 
		{
			bNetworked = 0,
		},
	},
	id_attach_to = -1,
	id_part_attach_to = -1,

	Editor =
	{
		Icon = "physicsobject.bmp",
	},
}

-------------------------------------------------------------------------------------------------------
function Cloth.Server:OnUnHidden()
	local attachmentInfo = self:LookForEntityToAttachTo();
	self:ReattachSoftEntityVtx( attachmentInfo.entityId, attachmentInfo.partId );
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnReset()
	local PhysParams = {};
	PhysParams.density = self.Properties.density;
	PhysParams.mass = self.Properties.mass;
	PhysParams.bRigidCore = self.Properties.bRigidCore;

	local i = 0;
	local link = self:GetLinkTarget("AttachedTo",i);
	if (link) then
		PhysParams.AttachmentIdEnt = link.id;
		PhysParams.AttachmentPartId = -1;
	else
		local attachmentInfo = self:LookForEntityToAttachTo();
		PhysParams.AttachmentId = attachmentInfo.entityId;
		PhysParams.AttachmentPartId = attachmentInfo.partId;
	end

	self:Physicalize(0, PE_SOFT, PhysParams);
	self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.Properties );
	self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties );
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, self.Properties );

	local CollParams = { collision_mask = 0 };
	if self.Properties.bCollideWithTerrain==1 then CollParams.collision_mask = CollParams.collision_mask+ent_terrain; end
	if self.Properties.bCollideWithStatics==1 then CollParams.collision_mask = CollParams.collision_mask+ent_static; end
	if self.Properties.bCollideWithPhysical==1 then CollParams.collision_mask = CollParams.collision_mask+ent_rigid+ent_sleeping_rigid; end
	if self.Properties.bCollideWithPlayers==1 then CollParams.collision_mask = CollParams.collision_mask+ent_living; end
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, CollParams );
	if self.Properties.air_resistance>0 then
		self:AwakePhysics(1);
	else
		self:AwakePhysics(0);
	end
end

-------------------------------------------------------------------------------------------------------
function Cloth:LookForEntityToAttachTo()
	local attachment =
	{
		entityId = NULL_ENTITY,
		partId = -1,
	};

	if (self.Properties.attach_radius>0) then
	local ents = Physics.SamplePhysEnvironment(self:GetPos(), self.Properties.attach_radius, ent_terrain+ent_static+ent_rigid+ent_sleeping_rigid+ent_independent);
		if (ents[3]) then
			attachment.entityId = ents[3];
			attachment.partId = ents[2];
		end
	end
	return attachment;
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnSpawn()
	if (self.Properties.MultiplayerOptions.bNetworked == 0) then
		self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	end

	self:LoadSubObject(0, self.Properties.fileModel, "cloth");
	self:OnReset();
end

-------------------------------------------------------------------------------------------------------
function Cloth.Server:OnStartGame()
	self:OnReset();
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnPropertyChange()
	self:LoadSubObject(0, self.Properties.fileModel, "cloth");
	self:OnReset();
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnDamage( hit )

--	System:LogToConsole("dir="..hit.dir.x..","..hit.dir.y..","..hit.dir.z);
--	System:LogToConsole("pos="..hit.pos.x..","..hit.pos.y..","..hit.pos.z);
--	System:LogToConsole("impact_force="..hit.impact_force_mul);

	if( hit.ipart ) then
		self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
	else
		self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
	end
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnInit()
	self:OnReset();
end

-------------------------------------------------------------------------------------------------------
function Cloth:OnShutDown()
end

-------------------------------------------------------------------------------------------------------
function Cloth:Event_WindOn(sender)
	local windparam = { wind = {x=0,y=0,z=0} };
	CopyVector(windparam.wind, self.Properties.wind_event);
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, windparam );
end

-------------------------------------------------------------------------------------------------------
function Cloth:Event_WindOff(sender)
	local windparam = { wind = {x=0,y=0,z=0} };
	CopyVector(windparam.wind, self.Properties.wind);
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, windparam );
end

Cloth.FlowEvents =
{
	Inputs =
	{
		WindOn = { Cloth.Event_WindOn, "bool" },
		WindOff = { Cloth.Event_WindOff, "bool" },
	},
	Outputs =
	{
		WindOn = "bool",
		WindOff = "bool",
	},
}
