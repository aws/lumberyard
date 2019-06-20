local simpleanimationtrigger =
{
	Properties =
	{
		EventPlay = { default = "PlayAnimation", description = "The name of the event that will start playing the animation.", },
		AnimationName = { default = "", description = "Name of animation to play.  Leave blank to play default animation.", },
		EndLoopAnimationName = { default = "", description = "Name of looping animation to play when main anim finishes.", },
	},
}

function simpleanimationtrigger:OnActivate()

	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);

	if (self.Properties.EndLoopAnimationName ~= "") then
		self.animNotifyHandler = SimpleAnimationComponentNotificationBus.Connect(self, self.entityId);
	end
end

function simpleanimationtrigger:OnDeactivate()

	self.playHandler:Disconnect();
	self.playHandler = nil;

	if (self.animNotifyHandler ~= nil) then
		self.animNotifyHandler:Disconnect();
		self.animNotifyHandler = nil;
	end

end

function simpleanimationtrigger:OnAnimationStopped(layer)

	local animInfo = AnimatedLayer(self.Properties.EndLoopAnimationName, 0, true, 1.0, 0.0);
	SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, animInfo);
	
end

function simpleanimationtrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		if (self.Properties.AnimationName ~= nil) then
		    local animInfo = AnimatedLayer(self.Properties.AnimationName, 0, false, 1.0, 0.0);
			SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, animInfo);
		else
			SimpleAnimationComponentRequestBus.Event.StartDefaultAnimations(self.entityId);
		end
	end

end

return simpleanimationtrigger;