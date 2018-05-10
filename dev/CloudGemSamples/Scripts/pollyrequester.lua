pollyrequester =
{
	Properties =
	{
		character = {default = "NewCharacter_0", description = "Character Name"}
	},
}

function pollyrequester:OnActivate()
	self.tickBusHandler = TickBus.Connect(self,self.entityId);

	self.playbackHandler = TextToSpeechPlaybackNotificationBus.Connect(self, self.entityId);
	Debug.Log("Activating")
end

function pollyrequester:initUiCanvas()
	Debug.Log("Loading Ui Canvas")
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/TextToSpeechSample/UI/InputUi.uicanvas")

	self.textInputElement = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Message Input")
	self.characterInputElement = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Character Input")
	self.dropDownEntityId = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Voice Dropdown")
	
	self.uiNotifications = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

	
	local util = require("scripts.util")
	util.SetMouseCursorVisible(true)
	self:PopulateVoiceList()
end

function pollyrequester:OnTick(deltaTime,timePoint)
	self:initUiCanvas();
	self.tickBusHandler:Disconnect();
end

function pollyrequester:AddCharacterSSMLToMessage(language, timbre, tags, ssml, message)
	if #tags == 0 and language == "" and timbre == 100 then
		-- Cached files from the CGP will already have start and end 'speak' ssml tags, even if everything else is default if the character has SSML tags enabled
		if ssml then
			return "<speak>" .. message .. "</speak>";
		end
		return message
	end

	local findTag = string.find(message, "<speak>", 1, true);
	if findTag == 1 and string.find(message, "</speak>", 1, true) == string.len(message) - 7 then
		Debug.Log("already have ssml in this text");
		message = string.sub(message, 8, string.len(message) - 8);
	end

	if language ~= "" then
		ssmlLangTag = "lang=\""..language.."\""
		message = "<lang xml:"..ssmlLangTag..">"..message.."</lang>"
	end

	if timbre ~= 100 then
		message = "<amazon:effect vocal-tract-length=\""..tostring(timbre).."%\">"..message.."</amazon:effect>"
	end

	local finalMessage = "<speak>";
	if #tags ~= 0 then
		finalMessage = finalMessage.."<prosody"
	end

	for index = 1, #tags do
		finalMessage = finalMessage.." "..tags[index];
	end
	if #tags ~= 0 then
		finalMessage = finalMessage .. ">";
	end
	finalMessage = finalMessage .. message
	if #tags ~= 0 then
		finalMessage = finalMessage.."</prosody>"
	end
	
	finalMessage = finalMessage .."</speak>";

	return finalMessage
end

function pollyrequester:OnAction(entityId, actionName)
	if actionName == "speak" then
		local text = UiTextInputBus.Event.GetText(self.textInputElement);
		if string.len(text) > 0 then
			
			local dropDownSelection = UiDropdownBus.Event.GetValue(self.dropDownEntityId)
			if dropDownSelection:IsValid() then
				local selectedText = UiDropdownBus.Event.GetTextElement(self.dropDownEntityId)
				local voiceInput = UiTextBus.Event.GetText(selectedText);
				if voiceInput == '' then
					voiceInput = self.Properties.character;
				end
				-- This optional character input let's the users slect the voice of the Characters for their game without having to
				-- remember what the name of the Polly voice is. This is useful when getting a package of voices generated in the Cloud Gem Portal
				local characterInput = UiTextInputBus.Event.GetText(self.characterInputElement);
				local marks = "VS";
				if characterInput == '' then
					characterInput = self.Properties.character;
				end
				local voice = TextToSpeechRequestBus.Event.GetVoiceFromCharacter(self.entityId, characterInput)

				if voice == '' then
					voice = voiceInput;
				else
					local prosodyTags = TextToSpeechRequestBus.Event.GetProsodyTagsFromCharacter(self.entityId, characterInput);
					local languageOverride = TextToSpeechRequestBus.Event.GetLanguageOverrideFromCharacter(self.entityId, characterInput);
					local timbre = TextToSpeechRequestBus.Event.GetTimbreFromCharacter(self.entityId, characterInput);
					marks = TextToSpeechRequestBus.Event.GetSpeechMarksFromCharacter(self.entityId, characterInput)
					text = self:AddCharacterSSMLToMessage(languageOverride, timbre, prosodyTags, string.match(marks, "T"), text)
				end

				TextToSpeechRequestBus.Event.ConvertTextToSpeechWithMarks(self.entityId, voice, text, marks);
				UiTextInputBus.Event.SetText(self.textInputElement, "");
			end
		end
	end
end

function pollyrequester:OnDeactivate()
	self.playbackHandler:Disconnect();
	self.uiNotifications:Disconnect();
end

function pollyrequester:PlaySpeech(voicePath)
	Debug.Log("Play without lip sync");
end

function pollyrequester:PlayWithLipSync(voicePath, speechMarksPath)
	Debug.Log("Play with lip sync");
end

function pollyrequester:PopulateVoiceList()
	
	local scrollBoxEntityID = UiElementBus.Event.FindChildByName(self.dropDownEntityId, "ScrollBox")
	local maskEntityID = UiElementBus.Event.FindChildByName(scrollBoxEntityID, "Mask")
	local dropDownContentEntityID = UiElementBus.Event.FindChildByName(maskEntityID, "Content")
	local voiceList = {"Joanna",  "Kendra", "Kimberly", "Salli", "Joey", "Justin", "Ivy", "Amy", "Brian", "Emma", "Nicole", "Russell", "Geraint", "Celine", "Chantal", "Mathieu", "Hans", "Marlene", "Vicki", "Dora", "Karl", "Carla", "Giorgio", "Mizuki", "Liv", "Jacek", "Jan", "Ewa", "Maja", "Ricardo", "Vitoria", "Cristiano", "Ines", "Carmen", "Maxim", "Tatyana", "Conchita", "Enrique", "Miguel", "Penelope", "Astrid", "Filiz", "Gwyneth", "Mads", "Naja", "Lotte", "Ruben"}
	local numVoices = table.getn(voiceList)
	UiDynamicLayoutBus.Event.SetNumChildElements(dropDownContentEntityID, numVoices)
	if numVoices > 1 then
		for voiceCount = 1, numVoices do
			local optionElement = UiElementBus.Event.GetChild(dropDownContentEntityID, voiceCount-1)
			local optionText = UiElementBus.Event.FindChildByName(optionElement, "Text")
			UiTextBus.Event.SetText(optionText, tostring(voiceList[voiceCount]))
		end
		local curSelect = UiElementBus.Event.GetChild(dropDownContentEntityID,0)
		UiDropdownBus.Event.SetValue(self.dropDownEntityId, curSelect)
	end
	
	
end

return pollyrequester