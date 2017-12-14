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

g_timelineCounter = 0
g_animationCallbackCounter = 0
g_animationCallbacks = {}

local ScriptedEntityTweener = {}

function ScriptedEntityTweener:OnActivate()
	if self.tweenerNotificationHandler == nil then
		self.animationParameterShortcuts = 
		{
			--UI Related
			["opacity"] = {"UiFaderComponent", "Fade" },
			["imgColor"] = {"UiImageComponent", "Color" },
			["layoutMinWidth"] = {"UiLayoutCellComponent", "MinWidth" },
			["layoutMinHeight"] = {"UiLayoutCellComponent", "MinHeight" },
			["layoutTargetWidth"] = {"UiLayoutCellComponent", "TargetWidth" },
			["layoutTargetHeight"] = {"UiLayoutCellComponent", "TargetHeight" },
			["layoutExtraWidthRatio"] = {"UiLayoutCellComponent", "ExtraWidthRatio" },
			["layoutExtraHeightRatio"] = {"UiLayoutCellComponent", "ExtraHeightRatio" },
			["layoutColumnPadding"] = {"UiLayoutColumnComponent", "Padding" },
			["layoutColumnSpacing"] = {"UiLayoutColumnComponent", "Spacing" },
			["layoutRowPadding"] = {"UiLayoutRowComponent", "Padding" },
			["layoutRowSpacing"] = {"UiLayoutRowComponent", "Spacing" },
			["scrollHandleSize"] = {"UiScrollBarComponent", "HandleSize" },
			["scrollHandleMinPixelSize"] = {"UiScrollBarComponent", "MinHandlePixelSize" },
			["scrollValue"] = {"UiScrollBarComponent", "Value" },
			["sliderValue"] = {"UiSliderComponent", "Value" },
			["sliderMinValue"] = {"UiSliderComponent", "MinValue" },
			["sliderMaxValue"] = {"UiSliderComponent", "MaxValue" },
			["sliderStepValue"] = {"UiSliderComponent", "StepValue" },
			["textSize"] = {"UiTextComponent", "FontSize" },
			["textColor"] = {"UiTextComponent", "Color" },
			["textCharacterSpace"] = {"UiTextComponent", "CharacterSpacing" },
			["textSpacing"] = {"UiTextComponent", "LineSpacing" },
			["textInputSelectionColor"] = {"UiTextInputComponent", "TextSelectionColor" },
			["textInputCursorColor"] = {"UiTextInputComponent", "TextCursorColor" },
			["textInputCursorBlinkInterval"] = {"UiTextInputComponent", "CursorBlinkInterval" },
			["textInputMaxStringLength"] = {"UiTextInputComponent", "MaxStringLength" },
			["tooltipDelayTime"] = {"UiTooltipDisplayComponent", "DelayTime" },
			["tooltipDisplayTime"] = {"UiTooltipDisplayComponent", "DisplayTime" },
			["scaleX"] = {"UiTransform2dComponent", "ScaleX" },
			["scaleY"] = {"UiTransform2dComponent", "ScaleY" },
			["pivotX"] = {"UiTransform2dComponent", "PivotX" },
			["pivotY"] = {"UiTransform2dComponent", "PivotY" },
			["x"] = {"UiTransform2dComponent", "LocalPositionX" },
			["y"] = {"UiTransform2dComponent", "LocalPositionY" },
			["rotation"] = {"UiTransform2dComponent", "Rotation" },
			["w"] = {"UiTransform2dComponent", "LocalWidth" },
			["h"] = {"UiTransform2dComponent", "LocalHeight" },
			
			--3d transform
			["3dposition"] = {"TransformComponent", "Position" },
			["3drotation"] = {"TransformComponent", "Rotation" },
			["3dscale"] = {"TransformComponent", "Scale" },
			--Camera
			["camFov"] = {"CameraComponent", "FieldOfView" },
			["camNear"] = {"CameraComponent", "NearClipDistance" },
			["camFar"] = {"CameraComponent", "FarClipDistance" },
			--[[
			--Some available virtual properties without shortcuts
			--Lights
			[""] = {"LightComponent", "Visible" },
			[""] = {"LightComponent", "Color" },
			[""] = {"LightComponent", "DiffuseMultiplier" },
			[""] = {"LightComponent", "SpecularMultiplier" },
			[""] = {"LightComponent", "Ambient" },
			[""] = {"LightComponent", "PointMaxDistance" },
			[""] = {"LightComponent", "PointAttenuationBulbSize" },
			[""] = {"LightComponent", "AreaMaxDistance" },
			[""] = {"LightComponent", "AreaWidth" },
			[""] = {"LightComponent", "AreaHeight" },
			[""] = {"LightComponent", "AreaFOV" },
			[""] = {"LightComponent", "ProjectorMaxDistance" },
			[""] = {"LightComponent", "ProjectorAttenuationBulbSize" },
			[""] = {"LightComponent", "ProjectorFOV" },
			[""] = {"LightComponent", "ProjectorNearPlane" },
			[""] = {"LightComponent", "ProbeAreaDimensions" },
			[""] = {"LightComponent", "ProbeSortPriority" },
			[""] = {"LightComponent", "ProbeBoxProjected" },
			[""] = {"LightComponent", "ProbeBoxHeight" },
			[""] = {"LightComponent", "ProbeBoxLength" },
			[""] = {"LightComponent", "ProbeBoxWidth" },
			[""] = {"LightComponent", "ProbeAttenuationFalloff" },
			--Particles
			[""] = {"ParticleComponent", "Visible" },
			[""] = {"ParticleComponent", "Enable" },
			[""] = {"ParticleComponent", "ColorTint" },
			[""] = {"ParticleComponent", "CountScale" },
			[""] = {"ParticleComponent", "TimeScale" },
			[""] = {"ParticleComponent", "SpeedScale" },
			[""] = {"ParticleComponent", "GlobalSizeScale" },
			[""] = {"ParticleComponent", "ParticleSizeScaleX" },
			[""] = {"ParticleComponent", "ParticleSizeScaleY" },
			--Static mesh
			["meshVisibility"] = {"StaticMeshComponent", "Visibility" },
			--]]
			--Add any more custom shortcuts here,  [string] = { ComponentName, VirtualProperty }
		}
		self.tweenerNotificationHandler = ScriptedEntityTweenerNotificationBus.Connect(self)
		self.tickBusHandler = TickBus.Connect(self)
		self.activationCount = 0
		self.timelineRefs = {}
	end
	self.activationCount = self.activationCount + 1
end

function ScriptedEntityTweener:OnDeactivate()
	if self.tweenerNotificationHandler then
		self.activationCount = self.activationCount - 1
		if self.activationCount == 0 then
			self.tweenerNotificationHandler:Disconnect()
			self.tweenerNotificationHandler = nil
			self.tickBusHandler:Disconnect()
			self.tickBusHandler = nil
		end
	end
end

function ScriptedEntityTweener:StartAnimation(args)
	if self.animationParameterShortcuts == nil then
		Debug.Log("ScriptedEntityTweener: Make sure to call OnActivate() and OnDeactivate() for this table when requiring this file")
		return
	end

	if self:ValidateAnimationParameters(args) == false then
		return
	end

	local optionalParams = 
	{
		timeIntoTween = args.timeIntoTween or 0, --Example: If timeIntoTween = 0.5 and duration = 1, the animation will instantly set the animation to 50% complete.
		duration = args.duration or 0,
		easeMethod = args.easeMethod or ScriptedEntityTweenerEasingMethod_Linear,
		easeType =  args.easeType or ScriptedEntityTweenerEasingType_Out,
		delay = args.delay or 0.0,
		timesToPlay = args.timesToPlay or 1,
		isFrom = args.isFrom,
		isPlayingBackward = args.isPlayingBackward,
		uuid = args.uuid or Uuid.CreateNull()
	}
	self:ConvertShortcutsToVirtualProperties(args)
	local virtualPropertiesToUse = args.virtualProperties
	if args.timelineParams ~= nil then
		optionalParams.delay = optionalParams.delay + args.timelineParams.initialStartTime
		optionalParams.timeIntoTween = optionalParams.timeIntoTween + args.timelineParams.timeIntoTween
		optionalParams.timelineId = args.timelineParams.timelineId
		optionalParams.uuid = args.timelineParams.uuidOverride
		
		if args.timelineParams.durationOverride ~= nil then
			optionalParams.duration = args.timelineParams.durationOverride
		end
		
		if args.timelineParams.seekDelayOverride ~= nil then
			optionalParams.delay = args.timelineParams.seekDelayOverride
		end
		
		if args.timelineParams.reversePlaybackOverride then
			if optionalParams.isPlayingBackward == nil then
				optionalParams.isPlayingBackward = false
			end
			optionalParams.isPlayingBackward = not optionalParams.isPlayingBackward
		end
	end
	
	--Create animation callbacks with an id, id sent to C so C can notify this system to trigger the callback.
	if args.onComplete ~= nil then
		optionalParams.onComplete = self:CreateCallback(args.onComplete)
	end
	if args.onUpdate ~= nil then
		optionalParams.onUpdate = self:CreateCallback(args.onUpdate)
	end
	if args.onLoop ~= nil then
		optionalParams.onLoop = self:CreateCallback(args.onLoop)
	end
	
	for componentData, paramTarget in pairs(virtualPropertiesToUse) do
		ScriptedEntityTweenerBus.Broadcast.SetOptionalParams(
			optionalParams.timeIntoTween,
			optionalParams.duration,
			optionalParams.easeMethod, 
			optionalParams.easeType, 
			optionalParams.delay, 
			optionalParams.timesToPlay, 
			optionalParams.isFrom == true,
			optionalParams.isPlayingBackward == true,
			optionalParams.uuid,
			optionalParams.timelineId or 0, 
			optionalParams.onComplete or 0,
			optionalParams.onUpdate or 0,
			optionalParams.onLoop or 0)
			
		ScriptedEntityTweenerBus.Broadcast.AnimateEntity(
			args.id,
			componentData[1],
			componentData[2],
			paramTarget)
	end
end

function ScriptedEntityTweener:ValidateAnimationParameters(args)
	--check for required params
	if args == nil then
		Debug.Log("ScriptedEntityTweener: animation with invalid args, args == nil")
		return false
	end
	if args.id == nil then
		Debug.Log("ScriptedEntityTweener: animation with no id specified" .. args)
		return false
	end
	return true
end

function ScriptedEntityTweener:ConvertShortcutsToVirtualProperties(args)
	for shortcutName, componentData in pairs(self.animationParameterShortcuts) do
		local paramTarget = args[shortcutName]
		if paramTarget ~= nil then
			if args.virtualProperties == nil then
				args.virtualProperties = {}
			end
			local virtualPropertyKey = {componentData[1], componentData[2]}
			args.virtualProperties[virtualPropertyKey] = paramTarget
			args[shortcutName] = nil
		end
	end
end

--Callback related
function ScriptedEntityTweener:CreateCallback(fnCallback)
	--todo: better uuid generation
	g_animationCallbackCounter = g_animationCallbackCounter + 1
	g_animationCallbacks[g_animationCallbackCounter] = fnCallback
	return g_animationCallbackCounter
end

function ScriptedEntityTweener:RemoveCallback(callbackId)
	g_animationCallbacks[callbackId] = nil
end

function ScriptedEntityTweener:CallCallback(callbackId)
	local callbackFn = g_animationCallbacks[callbackId]
	if callbackFn ~= nil then
		callbackFn()
	end
end

function ScriptedEntityTweener:OnComplete(callbackId)
	self:CallCallback(callbackId)
	self:RemoveCallback(callbackId)
end

function ScriptedEntityTweener:OnUpdate(callbackId, currentValue, progressPercent)
	local callbackFn = g_animationCallbacks[callbackId]
	if callbackFn ~= nil then
		callbackFn(currentValue, progressPercent)
	end
end

function ScriptedEntityTweener:OnLoop(callbackId)
	self:CallCallback(callbackId)
end

--Timeline related

function ScriptedEntityTweener:OnTick(deltaTime, timePoint)
	for timelineId, timeline in pairs(self.timelineRefs) do
		timeline.currentSeekTime = timeline.currentSeekTime + deltaTime
	end
end

function ScriptedEntityTweener:GetUniqueTimelineId()
	g_timelineCounter = g_timelineCounter + 1
	return g_timelineCounter
end

function ScriptedEntityTweener:TimelineCreate()
	local timeline = {}
	timeline.animations = {}
	timeline.labels = {}
	timeline.duration = 0
	timeline.isPaused = false
	timeline.timelineId = self:GetUniqueTimelineId()
	timeline.currentSeekTime = 0
	
	--Gets the duration of a specific animation
	timeline.GetDurationOfAnim = function(timelineSelf, animParams)
		return ((animParams.duration or 0) - (animParams.timeIntoTween or 0))+ (animParams.delay or 0)
	end
	
	--Timeline configuration functions
	timeline.Add = function(timelineSelf, animParams, timelineParams)
		--Timeline is a collection of animations
		--Operations like play, pause, reverse, etc
		--    Any animation is automatically added to the end of the animation.
		--		Optional parameters:
		--		timelineParams.offset will add it to the end +time specified, negative offsets allowed. 
		--		timelineParams.initialStartTime will set the timeline to start playing at a certain time within the timeline
		--		timelineParams.label will specify the timelime to start playing at the label.
		if timelineSelf == nil then
			Debug.Log("ScriptedEntityTweener:TimelineAdd no timeline")
			return
		end
		
		if self:ValidateAnimationParameters(animParams) == false then
			Debug.Log("ScriptedEntityTweener:TimelineAdd invalid animation parameters for timline uuid " .. animParams)
			return
		end
	
		local optionalParams = 
		{
			offset = 0,
		}
		if timelineParams ~= nil then
			if timelineParams.label then
				optionalParams.initialStartTime = timelineSelf.labels[timelineParams.label]
			else
				optionalParams.initialStartTime = timelineParams.initialStartTime
			end
			
			optionalParams.offset = timelineParams.offset or 0
		end
		animParams.timelineParams = {}
		--initialStartTime is either specified from user as a number or a label, otherwise append animation to end of timeline
		animParams.timelineParams.initialStartTime = optionalParams.initialStartTime or timelineSelf.duration
		--user can also offset the time the animation is appended to.
		animParams.timelineParams.initialStartTime = animParams.timelineParams.initialStartTime + optionalParams.offset
		--additional time into tween specified from timeline, to allow timeline to seek into the middle of an animation
		
		animParams.timelineParams.timeIntoTween = 0
		--additional delay and duration overrides to allow seeking
		animParams.timelineParams.seekDelayOverride = nil
		animParams.timelineParams.durationOverride = nil
		
		animParams.timelineParams.timelineId = timelineSelf.timelineId
		animParams.timelineParams.uuidOverride = Uuid.Create()
		
		self:ConvertShortcutsToVirtualProperties(animParams)
		animParams.timelineParams.initialValues = {}
		for componentData, paramTarget in pairs(animParams.virtualProperties) do
			--Cache initial values for backwards playback purposes.
			animParams.timelineParams.initialValues[componentData] = ScriptedEntityTweenerBus.Broadcast.GetVirtualPropertyValue(animParams.id, componentData[1], componentData[2])
		end
		
		timelineSelf.animations[#timelineSelf.animations + 1] = animParams
		timelineSelf.duration = timelineSelf.duration + timelineSelf:GetDurationOfAnim(animParams)
		
		table.sort(timelineSelf.animations, 
			function(first, second)
				return first.timelineParams.initialStartTime < second.timelineParams.initialStartTime
			end
		)
	end
	
	timeline.AddLabel = function(timelineSelf, labelId, labelTime)
		if labelId == nil then
			Debug.Log("Warning: TimelineLabel: labelId is nil")
			return
		end
		
		if labelTime == nil then
			Debug.Log("TimelineLabel: label " .. labelId .. " doesn't have a labelTime")
			return
		end
		
		if timelineSelf.labels[labelId] ~= nil then
			Debug.Log("Warning: TimelineLabel: label " .. labelId .. " already exists")
		end
		
		timeline.labels[labelId] = labelTime
	end
	
	--Timeline control functions
	timeline.Play = function(timelineSelf, labelOrTime)
		timelineSelf:Resume()
		local startTime = 0
		if labelOrTime ~= nil then
			local typeInfo = type(labelOrTime)
			if typeInfo == "string" then
				startTime = timelineSelf.labels[labelOrTime] or 0
			elseif typeInfo == "number" then
				startTime = labelOrTime
			end
		end
		
		timelineSelf:Seek(startTime)
	end
	
	
	timeline.Pause = function(timelineSelf)
		timelineSelf.isPaused = true
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			for componentData, paramTarget in pairs(animParams.virtualProperties) do
				ScriptedEntityTweenerBus.Broadcast.Pause(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2])
			end
		end
	end
	
	timeline.Resume = function(timelineSelf)
		timelineSelf.isPaused = false
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			for componentData, paramTarget in pairs(animParams.virtualProperties) do
				ScriptedEntityTweenerBus.Broadcast.Resume(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2])
			end
		end
	end
	
	timeline.ResetRuntimeVars = function(timelineSelf, animParams)
		--Reset all timeline params that might have been set by a previous Seek or Reverse call
		animParams.timelineParams.timeIntoTween = 0
		animParams.timelineParams.reversePlaybackOverride = nil
		animParams.timelineParams.seekDelayOverride = nil
		animParams.timelineParams.durationOverride = nil
	end
	
	timeline.Seek = function(timelineSelf, seekTime)
		local typeInfo = type(seekTime)
		if typeInfo == "string" then
			seekTime = timelineSelf.labels[seekTime] or 0
		end
		
		--Go through each animation in the timeline and configure them to play as appropriate
		timelineSelf.currentSeekTime = seekTime
		
		local runningDuration = 0
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			
			local prevCompletionState = runningDuration < seekTime
			runningDuration = runningDuration + timelineSelf:GetDurationOfAnim(animParams)
			local currentCompletionState = runningDuration < seekTime
			
			timelineSelf:ResetRuntimeVars(animParams)
			
			if runningDuration <= seekTime then
				--Current animation already complete
				animParams.timelineParams.seekDelayOverride = 0
				animParams.timelineParams.durationOverride = 0
			elseif prevCompletionState ~= currentCompletionState then
				--Current animation is partially complete
				local diff = runningDuration - seekTime
				animParams.timelineParams.timeIntoTween = timelineSelf:GetDurationOfAnim(animParams) - diff
				animParams.timelineParams.seekDelayOverride = 0
			elseif runningDuration > seekTime then
				--Current animation still needs to be played
				animParams.timelineParams.seekDelayOverride = (animParams.delay or 0) + (animParams.timelineParams.initialStartTime - seekTime)
			end
		end
		
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			self:StartAnimation(animParams)
		end
	end
	
	timeline.PlayBackwards = function(timelineSelf, specificSeekTime)
		local seekTime = specificSeekTime
		if seekTime == nil then
			seekTime = timelineSelf.currentSeekTime
		end
		
		local animsToPlay = {}
		
		local runningDuration = 0
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			
			runningDuration = runningDuration + timelineSelf:GetDurationOfAnim(animParams)
			
			timelineSelf:ResetRuntimeVars(animParams)
			
			if runningDuration <= seekTime then
				--Current animation has already played, queue it to play again backwards
				animParams.timelineParams.timeIntoTween = timelineSelf:GetDurationOfAnim(animParams)
				animParams.timelineParams.seekDelayOverride = seekTime - runningDuration
				animParams.timelineParams.reversePlaybackOverride = true
				if not timelineSelf.isPaused then
					animsToPlay[#animsToPlay + 1] = animParams
				end
			else
				--Current animation is either partially complete or hasn't been played
				for componentData, paramTarget in pairs(animParams.virtualProperties) do
					ScriptedEntityTweenerBus.Broadcast.SetPlayDirectionReversed(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2], true)
				end
			end
		end
		
		table.sort(animsToPlay, 
			function(first, second)
				return first.timelineParams.initialStartTime < second.timelineParams.initialStartTime
			end
		)
		
		for i=1,#animsToPlay do
			local animParams = animsToPlay[i]
			--Copy last initial values as start animation will overwrite with current initial values
			local initialValues = {}
			for componentData, paramInitial in pairs(animParams.timelineParams.initialValues) do
				initialValues[componentData] = paramInitial
			end
			self:StartAnimation(animParams)
			--Set the newly started animation's initial start position from cached start position.
			for componentData, paramInitial in pairs(initialValues) do
				ScriptedEntityTweenerBus.Broadcast.SetInitialValue(animParams.timelineParams.uuidOverride, animParams.id, componentData[1], componentData[2], paramInitial)
			end
		end
	end
	
	timeline.SetSpeed = function(timelineSelf, multiplier)
		for i=1, #timelineSelf.animations do
			local animParams = timelineSelf.animations[i]
			for componentData, paramTarget in pairs(animParams.virtualProperties) do
				ScriptedEntityTweenerBus.Broadcast.SetSpeed(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2], multiplier)
			end
		end
	end
	
	--SetCompletion - Seek to animation percentage rather than specific time or label
	timeline.SetCompletion = function(timelineSelf, percentage)
		timelineSelf:Seek(timelineSelf.duration * percentage)
	end
	
	self.timelineRefs[timeline.timelineId] = timeline
	
	return timeline
end

function ScriptedEntityTweener:TimelineDestroy(timeline)
	--If a timeline is created, it should eventually be destroyed, otherwise they'll never be removed from self.timelineRef's table and garbage collected.
	self.timelineRefs[timeline.timelineId] = nil
end

function ScriptedEntityTweener:OnTimelineAnimationStart(timelineId, animUuid, addressComponentName, addressPropertyName)
	--Update cached initial value whenever a timeline animation starts, this data is needed if the user ever wants to play the animation backwards
	local timeline = self.timelineRefs[timelineId]
	if timeline == nil then
		return
	end
	
	for i=1,#timeline.animations do
		local animParams = timeline.animations[i]
		if animParams.timelineParams.uuidOverride == animUuid then
			for componentData, paramTarget in pairs(animParams.virtualProperties) do
				if componentData[1] == addressComponentName and componentData[2] == addressPropertyName then
					animParams.timelineParams.initialValues[componentData] = ScriptedEntityTweenerBus.Broadcast.GetVirtualPropertyValue(animParams.id, componentData[1], componentData[2])
					break
				end
			end
		end
	end
	
end 

return ScriptedEntityTweener