local scanner =
{
	Properties =
	{
		TriggerBox = { default = EntityId(), description = "The Trigger Box that is responsible for the door." },
		TestKey = { default = "", description = "The thing to test in the persistent data system to signify success / failure." },
		InitiallyEnabled = { default = true, description = "Can i be used." },
		DisableOnSuccess = { default = true, description = "Set to disabled on successful scan." },
		ScanTime = { default = 3.0, description = "The time i spend in the scanning state." },
		
		ScreenEffects = {
			ScanningEffect = { default = EntityId(), description = "The Object with the effect for scanning." },
			SuccessEffect = { default = EntityId(), description = "The Object with the success effect." },
			FailEffect = { default = EntityId(), description = "The Object with the fail effect." },
		},
		ScannerEffects = {
			ScanningModel = { default = EntityId(), description = "The a secondry effect for scanning." },
			MovementDistance = { default = 2, description = "The distance that the scanning effect oscilates." },
			MovementTime = { default = 2, description = "The time taken to complete an ossicilation." },
		},
		Sounds = {
			ScanningStart = { default = "", description = "The a sound for scanning start message." },
			ScanningStop = { default = "", description = "The a sound for scanning stop message." },
			ScanFail = { default = "", description = "The sound for Fail, one hit." },
			ScanSuccess = { default = "", description = "The sound for Success, one hit." },
		},
				
		MessageOut = {
			SuccessMessage = { default = "", description = "Message to send when i have successfuly done my scan." },
			FailMessage = { default = "", description = "Message to send when i have unsuccessfuly done my scan." },
		},		
		MessageIn = {
			EnableMessage = { default = "EnableScanner", description = "Set if i be used." },
		}
	},
}

function scanner:OnActivate()
	--Debug.Log("scanner:OnActivate: " .. tostring(self.entityId));
	self.insideTrigger = false;
	self.insideTimer = 0;
	self.enabled = false;
	self.scannedObject = nil;
	self:SetEnabled(self.Properties.InitiallyEnabled);
	self:SetScanning(false, self.scannedObject);
	
	self.enabledEventId = GameplayNotificationId(self.entityId, self.Properties.MessageIn.EnableMessage, "float");
	self.enabledHandler = GameplayNotificationBus.Connect(self, self.enabledEventId);

	self.successMessageID = GameplayNotificationId(self.entityId, self.Properties.MessageOut.SuccessMessage, "float");
	self.failMessageID = GameplayNotificationId(self.entityId, self.Properties.MessageOut.FailMessage, "float");
end

function scanner:OnDeactivate()
	--Debug.Log("scanner:OnDeactivate: " .. tostring(self.entityId));
	self:SetScanning(false, self.scannedObject);
	self:SetEnabled(false);
	
	if(not self.enabledHandler == nil) then
		self.enabledHandler:Disconnect();
		self.enabledHandler = nil;
	end
end

function scanner:SetEnabled(enabled)
	--Debug.Log("scanner:SetEnabled: " .. tostring(enabled));
	if(enabled ~= self.enabled)then
		self.enabled = enabled;
		if(enabled) then
			self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.Properties.TriggerBox);
		else		
			self:SetScanning(false, self.scannedObject);
			self.triggerAreaHandler:Disconnect();
			self.triggerAreaHandler = nil;
		end
	end
end

function scanner:SetScanning(scanning, entityId)
	--Debug.Log("scanner:SetScanning: " .. tostring(scanning) .. " : " .. tostring(entityId));
	-- do some simple validation to make sure something weird is not happening
	if(self.insideTrigger) then
		-- already scanning
		if(scanning) then
			-- wants to scan
			if(entityId ~= self.scannedObject) then
				-- new object has entered while im scanning another, ignore
				return;
			else
				-- somehow refired on myself, should be impossible
				return;
			end
		else
			-- wants to stop my scan
			if(entityId ~= self.scannedObject) then
				-- an object other than the one i am scanning has left, ignore
				return;
			else
				-- object being scanned is leaving, continue
				self.scannedObject = nil;
				
				self.tickHandler:Disconnect();
				self.tickHandler = nil;
			end
		end
	else
		-- i am not currently scanning
		if(scanning) then
			-- wants to scan a new object, continue
			self.scannedObject = entityId;
			self.tickHandler = TickBus.Connect(self);
		else
			-- wants to stop scanning, but i am not actually scanning?!
			return;
		end
	end
	
	if (scanning) then
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sounds.ScanningStart);
	else
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sounds.ScanningStop);
	end
	
	self.insideTrigger = scanning;
	self.insideTimer = 0;
	ParticleComponentRequestBus.Event.SetVisibility(self.Properties.ScreenEffects.ScanningEffect, scanning);
	MeshComponentRequestBus.Event.SetVisibility(self.Properties.ScannerEffects.ScanningModel, scanning);
end

function scanner:OnTick(deltaTime, timePoint)
	--Debug.Log("scanner:OnTick");
	if(self.insideTrigger) then
		self.insideTimer = self.insideTimer + deltaTime;
		
		local twoPi = 6.283185307179586476925286766559;	
		--Debug.Log("Pickup graphic values:  Rotation: " .. self.rotateTimer .. " Height: " .. self.bobTimer);
		local tm = Transform.CreateIdentity(); --ransform.CreateRotationZ(self.rotateTimer);
		tm:SetTranslation(Vector3(0, 0, (Math.Sin((self.insideTimer / self.Properties.ScannerEffects.MovementTime) * twoPi) + 1) * (self.Properties.ScannerEffects.MovementDistance * 0.5)));
		TransformBus.Event.SetLocalTM(self.Properties.ScannerEffects.ScanningModel, tm);
		
		if(self.insideTimer >= self.Properties.ScanTime) then
			-- whatever happens im stopping the active scan
			self:SetScanning(false, self.scannedObject);
			-- do the test for success / failure
			if(PersistentDataSystemRequestBus.Broadcast.GetData(self.Properties.TestKey))then
				ParticleComponentRequestBus.Event.Enable(self.Properties.ScreenEffects.SuccessEffect, true);
				AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sounds.ScanSuccess);
				
				if(self.Properties.DisableOnSuccess)then
					self:SetEnabled(false);
				end
				
				GameplayNotificationBus.Event.OnEventBegin(self.successMessageID, 1);
			else
				ParticleComponentRequestBus.Event.Enable(self.Properties.ScreenEffects.FailEffect, true);	
				AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sounds.ScanFail);
				
				GameplayNotificationBus.Event.OnEventBegin(self.failMessageID, 0);
			end
		end
	end
end

function scanner:OnEventBegin(value)
	--Debug.Log("scanner:OnEventBegin: " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.enabledEventId) then
		self:SetEnabled(value == true);
	end
end

function scanner:OnTriggerAreaEntered(entityId)
	--Debug.Log("scanner:OnTriggerAreaEntered: " .. tostring(entityId));
	self:SetScanning(true, entityId);
end

function scanner:OnTriggerAreaExited(entityId)
	--Debug.Log("scanner:OnTriggerAreaExited: " .. tostring(entityId));
	self:SetScanning(false, entityId);
end

return scanner;