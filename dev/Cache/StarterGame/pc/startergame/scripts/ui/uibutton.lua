local utilities = require "scripts/common/utilities"
local StateMachine = require "scripts/common/StateMachine"

local UIButton =
{
-- this basically sends a message at a newly loaded screen through the UI Asset Ref system so that you can traverse to the thing that loaded it.
	Properties =
	{
		Messages = {
			SetState = { default = "SetState", description = "The event to send to the screen with the parent / loader as the param." },
			SetAction = { default = "SetAction", description = "The event to send to the screen with the parent / loader as the param." },
		},
		
		StartState = {
			State =  { default = "Inactive", description = "The event to send to the screen with the parent / loader as the param."},
			Action = { default = StarterGameUIUtility.ActionType_Invalid, description = "The event to send to the screen with the parent / loader as the param."},
		},
		
		DebugStateMachine = { default = false, description = "The event to send to the screen with the parent / loader as the param."},
		
		ScreenHookup = { 
			Timings = {
				FadeTime = { default = 0.5, description = "Thetime taken to fade between states." },
				HighlightCycleTime  = { default = 1, description = "The time for the active anim cycle" },
				HighlightScaleAmount = { default = 0.75, description = "The amount that the active ring scales down by" },
			},
			States = { 
				Inactive 	= { default = EntityId(), description = "The model that the menu is displayed upon." },
				Idle 		= { default = EntityId(), description = "The model that the menu is displayed upon." },
				Active 		= { default = EntityId(), description = "The model that the menu is displayed upon." },
				Pressed 	= { default = EntityId(), description = "The model that the menu is displayed upon." },
			},
			ButtonImages = { 
				ActiveHighlight = { default = EntityId(), description = "The model that the menu is displayed upon." },
				
				Text = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},
				Blank = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},
				A = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},
				B = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				X = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				Y = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				Circle = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				Square = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				Triangle = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				Cross = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				DUp = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				DDown = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				DLeft = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
				DRight = { 
					Idle = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Active = { default = EntityId(), description = "The model that the menu is displayed upon." },
					Pressed = { default = EntityId(), description = "The model that the menu is displayed upon." },
				},			
			}		
		}
	},
	
	States = 
    {
        Inactive = 		
        {      
            OnEnter = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Inactive, 1, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
            end,
            OnExit = function(self, sm)				
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Inactive, 0, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Idle = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Idle";
                    end	
				},
 				Active = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Active";
                    end	
				},
 				Pressed = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Pressed";
                    end	
				},
 				Hidden = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Hidden";
                    end	
				}
           },
        },
		Idle =
		{      
            OnEnter = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Idle, 1, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
            end,
            OnExit = function(self, sm)				
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Idle, 0, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Inactive";
                    end	
				},
 				Active = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Active";
                    end	
				},
 				Pressed = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Pressed";
                    end	
				},
 				Hidden = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Hidden";
                    end	
				}
           },
        },
		Active =
		{      
            OnEnter = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Active, 1, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
				self.aimTimer = sm.UserData.Properties.ScreenHookup.Timings.HighlightCycleTime;

            end,
            OnExit = function(self, sm)				
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Active, 0, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
			end,            
            OnUpdate = function(self, sm, deltaTime)
				--Debug.Log("UIButton:StateMachine:OnUpdate - Active");
				local bt = sm.UserData.Properties.ScreenHookup.Timings.HighlightCycleTime;
				self.aimTimer = self.aimTimer + deltaTime;
				if (self.aimTimer >= bt) then
					self.aimTimer = self.aimTimer - bt;
				end
				local circleScale = utilities.Clamp((self.aimTimer / bt), 0.0, 1.0);
				local circleFade = 1 - circleScale;
				local itemToManipulate = sm.UserData.Properties.ScreenHookup.ButtonImages.ActiveHighlight;
				
				UiFaderBus.Event.Fade(itemToManipulate, circleFade, 0);
				local scaleVar = utilities.Lerp(1, sm.UserData.Properties.ScreenHookup.Timings.HighlightScaleAmount, circleScale);
				local scaleVec = Vector2(scaleVar, scaleVar);
				UiTransformBus.Event.SetScale(itemToManipulate, scaleVec);
            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Inactive";
                    end	
				},
 				Idle = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Idle";
                    end	
				},
 				Pressed = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Pressed";
                    end	
				},
 				Hidden = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Hidden";
                    end	
				}
           },
        },
		Pressed =
		{      
            OnEnter = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Pressed, 1, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
            end,
            OnExit = function(self, sm)				
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.States.Pressed, 0, 1.0 / sm.UserData.Properties.ScreenHookup.Timings.FadeTime);
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Inactive";
                    end	
				},
 				Idle = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Idle";
                    end	
				},
 				Active = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Active";
                    end	
				},
 				Hidden = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Hidden";
                    end	
				}
           },
        },
		Hidden =
		{      
            OnEnter = function(self, sm)
            end,
            OnExit = function(self, sm)				
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Inactive";
                    end	
				},
 				Idle = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Idle";
                    end	
				},
 				Active = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Active";
                    end	
				},
  				Pressed = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.DesiredState == "Pressed";
                    end	
				}
          },
        },
	}
}

function UIButton:OnActivate()

	--Debug.Log("UIButton:OnActivate");

	self.SetStateEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetState, "float");
	self.SetActionEventId	 = GameplayNotificationId(self.entityId, self.Properties.Messages.SetAction, "float");
		
	self.SetStateHandler = GameplayNotificationBus.Connect(self, self.SetStateEventId);
	self.SetActionHandler	 = GameplayNotificationBus.Connect(self, self.SetActionEventId);
	
	self.DesiredState = "Hidden";
	
	-- hide all the states so that i will reveal properly when i am sintanciated
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Inactive, 0, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Idle, 0, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Active, 0, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Pressed, 0, 0);
		
	self.stateMachine = {}
    setmetatable(self.stateMachine, StateMachine);
	self.stateMachine:Start("UIButtonSM", self.entityId, self, self.States, false, "Hidden", self.Properties.DebugStateMachine)
	
	self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;	
end

function UIButton:OnDeactivate()
	--Debug.Log("UIButton:OnDeactivate");

	self.stateMachine:Stop();

	if (self.SetStateHandler ~= nil) then
		self.SetStateHandler:Disconnect();
		self.SetStateHandler = nil;
	end
	if (self.SetActionHandler ~= nil) then
		self.SetActionHandler:Disconnect();
		self.SetActionHandler = nil;
	end	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function UIButton:OnTick(deltaTime, timePoint)
	--Debug.Log("UIButton:OnTick");
	if (self.performedFirstUpdate == false) then
		self.performedFirstUpdate = true;

		self:SetAction(self.Properties.StartState.Action);

		--Debug.Log("UIButton:OnTick - performedFirstUpdate");
		
		-- hide all the states so that i will reveal properly when i am sintanciated
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Inactive, 0, 0);
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Idle, 0, 0);
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Active, 0, 0);
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.States.Pressed, 0, 0);
	
		self:SetState(self.Properties.StartState.State);
	end
end

function UIButton:GetButtonFromAction(action)
	--local offset = UiTransform2dBus.Event.GetOffsets(self.entityId);
	--Debug.Log("IButton:GetButtonFromAction(" .. tostring(action) .. ") : " .. tostring(self.entityId) .. " : ( " .. tostring(offset.left) .. " , " .. tostring(offset.top) .. ")");
	-- note that this will need to be a proper component if proper button remapping and option screens get added
	-- for now use specifically set information
	if( (Platform.Current == Platform.Windows32) or
		(Platform.Current == Platform.Windows64)) then
		if ((action == StarterGameUIUtility.ActionType_Start) or
			(action == StarterGameUIUtility.ActionType_NavLeft) or
			(action == StarterGameUIUtility.ActionType_NavRight) or
			(action == StarterGameUIUtility.ActionType_NavUp) or
			(action == StarterGameUIUtility.ActionType_NavDown) or
			(action == StarterGameUIUtility.ActionType_CancelBack) or
			(action == StarterGameUIUtility.ActionType_TabLeft) or
			(action == StarterGameUIUtility.ActionType_TabRight) or
			(action == StarterGameUIUtility.ActionType_Pause) or
			(action == StarterGameUIUtility.ActionType_SlideLeft) or
			(action == StarterGameUIUtility.ActionType_SlideRight) )then
			return StarterGameUIUtility.ButtonType_Blank;
		elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
				(action == StarterGameUIUtility.ActionType_Use) or
				(action == StarterGameUIUtility.ActionType_TimeOfDayNext) or
				(action == StarterGameUIUtility.ActionType_TimeOfDayPrev) or
				(action == StarterGameUIUtility.ActionType_AAModeNext) or
				(action == StarterGameUIUtility.ActionType_AAModePrev) ) then
			return StarterGameUIUtility.ButtonType_Text;
		end
	elseif( Platform.Current == Platform.Provo ) then
		if ((action == StarterGameUIUtility.ActionType_Start) or
			(action == StarterGameUIUtility.ActionType_NavLeft) or
			(action == StarterGameUIUtility.ActionType_NavRight) or
			(action == StarterGameUIUtility.ActionType_NavUp) or
			(action == StarterGameUIUtility.ActionType_NavDown) or
			(action == StarterGameUIUtility.ActionType_TabLeft) or
			(action == StarterGameUIUtility.ActionType_TabRight) or
			(action == StarterGameUIUtility.ActionType_Pause) or
			(action == StarterGameUIUtility.ActionType_SlideLeft) or
			(action == StarterGameUIUtility.ActionType_SlideRight) ) then
			return StarterGameUIUtility.ButtonType_Blank;
		elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
				(action == StarterGameUIUtility.ActionType_Use) ) then
			return StarterGameUIUtility.ButtonType_Cross;
		elseif ((action == StarterGameUIUtility.ActionType_CancelBack) ) then
			return StarterGameUIUtility.ButtonType_Triangle;
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayNext ) then
			return StarterGameUIUtility.ButtonType_DDown;
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayPrev ) then
			return StarterGameUIUtility.ButtonType_DDown;
		elseif (action == StarterGameUIUtility.ActionType_AAModeNext ) then
			return StarterGameUIUtility.ButtonType_DUp;
		elseif (action == StarterGameUIUtility.ActionType_AAModePrev ) then
			return StarterGameUIUtility.ButtonType_DUp;			
		end		
	elseif( Platform.Current == Platform.Xenia ) then
		if ((action == StarterGameUIUtility.ActionType_Start) or
			(action == StarterGameUIUtility.ActionType_NavLeft) or
			(action == StarterGameUIUtility.ActionType_NavRight) or
			(action == StarterGameUIUtility.ActionType_NavUp) or
			(action == StarterGameUIUtility.ActionType_NavDown) or
			(action == StarterGameUIUtility.ActionType_TabLeft) or
			(action == StarterGameUIUtility.ActionType_TabRight) or
			(action == StarterGameUIUtility.ActionType_Pause) or
			(action == StarterGameUIUtility.ActionType_SlideLeft) or
			(action == StarterGameUIUtility.ActionType_SlideRight) ) then
			return StarterGameUIUtility.ButtonType_Blank;
		elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
				(action == StarterGameUIUtility.ActionType_Use) ) then
			return StarterGameUIUtility.ButtonType_A;
		elseif ((action == StarterGameUIUtility.ActionType_CancelBack) ) then
			return StarterGameUIUtility.ButtonType_Y;
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayNext ) then
			return StarterGameUIUtility.ButtonType_DDown;
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayPrev ) then
			return StarterGameUIUtility.ButtonType_DDown;
		elseif (action == StarterGameUIUtility.ActionType_AAModeNext ) then
			return StarterGameUIUtility.ButtonType_DUp;
		elseif (action == StarterGameUIUtility.ActionType_AAModePrev ) then
			return StarterGameUIUtility.ButtonType_DUp;	
		end
	elseif( (Platform.Current == Platform.iOS) or
			(Platform.Current == Platform.Android) or
			(Platform.Current == Platform.Android64)) then
			if ((action == StarterGameUIUtility.ActionType_Start) or
				(action == StarterGameUIUtility.ActionType_NavLeft) or
				(action == StarterGameUIUtility.ActionType_NavRight) or
				(action == StarterGameUIUtility.ActionType_NavUp) or
				(action == StarterGameUIUtility.ActionType_NavDown) or
				(action == StarterGameUIUtility.ActionType_CancelBack) or
				(action == StarterGameUIUtility.ActionType_TabLeft) or
				(action == StarterGameUIUtility.ActionType_TabRight) or
				(action == StarterGameUIUtility.ActionType_Pause) or
				(action == StarterGameUIUtility.ActionType_SlideLeft) or
				(action == StarterGameUIUtility.ActionType_SlideRight) )then
				return StarterGameUIUtility.ButtonType_Blank;
		elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
				(action == StarterGameUIUtility.ActionType_Use) or
				(action == StarterGameUIUtility.ActionType_TimeOfDayNext) or
				(action == StarterGameUIUtility.ActionType_TimeOfDayPrev) or
				(action == StarterGameUIUtility.ActionType_AAModeNext) or
				(action == StarterGameUIUtility.ActionType_AAModePrev) ) then
				return StarterGameUIUtility.ButtonType_Text;
		end
	else
		return StarterGameUIUtility.ButtonType_ButtonType_Blank;	
	end
end

function UIButton:GetTextFromAction(action)
	--Debug.Log("IButton:GetTextFromAction(" .. tostring(action) .. ")");
	-- note that this will need to be a proper component if proper button remapping and option screens get added
	-- for now use specifically set information
	if( (Platform.Current == Platform.Windows32) or
		(Platform.Current == Platform.Windows64)) then
		if ((action == StarterGameUIUtility.ActionType_Start) or
			(action == StarterGameUIUtility.ActionType_NavLeft) or
			(action == StarterGameUIUtility.ActionType_NavRight) or
			(action == StarterGameUIUtility.ActionType_NavUp) or
			(action == StarterGameUIUtility.ActionType_NavDown) or
			(action == StarterGameUIUtility.ActionType_CancelBack) or
			(action == StarterGameUIUtility.ActionType_TabLeft) or
			(action == StarterGameUIUtility.ActionType_TabRight) or
			(action == StarterGameUIUtility.ActionType_Pause) or
			(action == StarterGameUIUtility.ActionType_SlideLeft) or
			(action == StarterGameUIUtility.ActionType_SlideRight) ) then
			return StarterGameUIUtility.ButtonType_Blank;
		elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
				(action == StarterGameUIUtility.ActionType_Use) ) then
			return "E";
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayNext ) then
			return "&lt;"; -- <
		elseif (action == StarterGameUIUtility.ActionType_TimeOfDayPrev ) then
			return "&#63;"; -- ?
		elseif (action == StarterGameUIUtility.ActionType_AAModeNext ) then
			return "&gt;"; -- >
		elseif (action == StarterGameUIUtility.ActionType_AAModePrev ) then
			return "&#63;";	 -- ?
		end
	elseif ((Platform.Current == Platform.iOS) or
			(Platform.Current == Platform.Android) or
			(Platform.Current == Platform.Android64)) then
			if ((action == StarterGameUIUtility.ActionType_Start) or
				(action == StarterGameUIUtility.ActionType_NavLeft) or
				(action == StarterGameUIUtility.ActionType_NavRight) or
				(action == StarterGameUIUtility.ActionType_NavUp) or
				(action == StarterGameUIUtility.ActionType_NavDown) or
				(action == StarterGameUIUtility.ActionType_CancelBack) or
				(action == StarterGameUIUtility.ActionType_TabLeft) or
				(action == StarterGameUIUtility.ActionType_TabRight) or
				(action == StarterGameUIUtility.ActionType_Pause) or
				(action == StarterGameUIUtility.ActionType_SlideLeft) or
				(action == StarterGameUIUtility.ActionType_SlideRight) ) then
				return StarterGameUIUtility.ButtonType_Blank;
			elseif ((action == StarterGameUIUtility.ActionType_Confirm) or
					(action == StarterGameUIUtility.ActionType_Use) ) then
				return "B";
			elseif (action == StarterGameUIUtility.ActionType_TimeOfDayNext ) then
				return "Y";
			elseif (action == StarterGameUIUtility.ActionType_TimeOfDayPrev ) then
				return "&#63;"; -- ?
			elseif (action == StarterGameUIUtility.ActionType_AAModeNext ) then
				return "&gt;"; -- >
			elseif (action == StarterGameUIUtility.ActionType_AAModePrev ) then
				return "&#63;";	 -- ?
			end
	else
		return "&#63;";	-- ?
	end
	
end

function UIButton:SetButton(button)
	--Debug.Log("UIButton:SetButton(" .. tostring(button) .. ")");
	local fadeValue = 0;
	if((button == StarterGameUIUtility.ButtonType_Blank) or (button == StarterGameUIUtility.ButtonType_Text)) then
		fadeValue = 1;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Blank.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Blank.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Blank.Pressed, 	fadeValue, 0);

	if((button == StarterGameUIUtility.ButtonType_Text)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Text.Idle, 		fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Text.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Text.Pressed, 	fadeValue, 0);
	
	if((button == StarterGameUIUtility.ButtonType_A)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.A.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.A.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.A.Pressed, 	fadeValue, 0);
                          
	if((button == StarterGameUIUtility.ButtonType_B)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.B.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.B.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.B.Pressed, 	fadeValue, 0);
	
	if((button == StarterGameUIUtility.ButtonType_X)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.X.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.X.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.X.Pressed, 	fadeValue, 0);
	                     
	if((button == StarterGameUIUtility.ButtonType_Y)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Y.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Y.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Y.Pressed, 	fadeValue, 0);
	                     
	if((button == StarterGameUIUtility.ButtonType_Cross)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Cross.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Cross.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Cross.Pressed, 	fadeValue, 0);
	                      
	if((button == StarterGameUIUtility.ButtonType_Circle)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Circle.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Circle.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Circle.Pressed, fadeValue, 0);
	
	if((button == StarterGameUIUtility.ButtonType_Square)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Square.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Square.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Square.Pressed, fadeValue, 0);
	                      
	if((button == StarterGameUIUtility.ButtonType_Triangle)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Triangle.Idle, 		fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Triangle.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.Triangle.Pressed, 	fadeValue, 0);	
	                      
	if((button == StarterGameUIUtility.ButtonType_DUp)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DUp.Idle, 		fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DUp.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DUp.Pressed, 	fadeValue, 0);	
	                      
	if((button == StarterGameUIUtility.ButtonType_DDown)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DDown.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DDown.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DDown.Pressed, 	fadeValue, 0);	
	                      
	if((button == StarterGameUIUtility.ButtonType_DLeft)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DLeft.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DLeft.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DLeft.Pressed, 	fadeValue, 0);	
	                      
	if((button == StarterGameUIUtility.ButtonType_DRight)) then
		fadeValue = 1;
	else
		fadeValue = 0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DRight.Idle, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DRight.Active, 	fadeValue, 0);
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.ButtonImages.DRight.Pressed, fadeValue, 0);	
end

function UIButton:SetButtonText(text)
	--Debug.Log("UIButton:SetButtonText(" .. tostring(text) .. ")");
	UiTextBus.Event.SetText(self.Properties.ScreenHookup.ButtonImages.Text.Idle, text);
	UiTextBus.Event.SetText(self.Properties.ScreenHookup.ButtonImages.Text.Active, text);
	UiTextBus.Event.SetText(self.Properties.ScreenHookup.ButtonImages.Text.Pressed, text);
end

function UIButton:SetState(state)
	--Debug.Log("UIButton:SetState(" .. tostring(state) .. ")");
	self.DesiredState = state;
	
	-- do some validation, valid states are
	-- "Inactive"
	-- "Idle"
	-- "Active"
	-- "Pressed"
end

function UIButton:SetAction(state)
	--Debug.Log("UIButton:SetAction(" .. tostring(state) .. ")");
	self:SetButton(self:GetButtonFromAction(state));
	self:SetButtonText(self:GetTextFromAction(state));
end


function UIButton:OnEventBegin(value)
	--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.SetStateEventId) then
		self:SetState(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetActionEventId) then
		self:SetAction(value);
	end
end

return UIButton;
