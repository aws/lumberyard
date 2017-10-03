closedcaption =
{
    Properties =
    {
		
    },
}

function closedcaption:OnActivate()
	self.currentSentence = {}
	self.currentSentenceStartIndex = 0
	self.currentSentenceEndIndex = 0
	
	self.currentWord = {}
	self.currentWordStartIndex = 0
	self.currentWordEndIndex = 0
	
	-- Let's load the canvas
   self.canvasEntityId = UiCanvasManagerBus.Broadcast.FindLoadedCanvasByPathName("Levels/TextToSpeechSample/UI/InputUi.uicanvas")
	
	-- SHYNE --
	local ccEntityID = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ClosedCaption")
	self.closedCaptionOut = UiElementBus.Event.FindChildByName(ccEntityID, "CaptionOut")
   	self.notificationHandler = TextToSpeechClosedCaptionNotificationBus.Connect(self, self.entityId)
end


function closedcaption:OnNewClosedCaptionSentence(sentence, startIndex, endIndex)
	self.currentSentence = sentence
	self.currentSentenceStartIndex = startIndex
	self.currentSentenceEndIndex = endIndex
end

function closedcaption:OnNewClosedCaptionWord(word, startIndex, endIndex)
	self.currentWord = word
	self.currentWordStartIndex = startIndex
	self.currentWordEndIndex = endIndex
	
	-- Although it may seem like I forgot to make this 1 based, it is because we want to stop at the character before the word
	local relativeStartIndex = startIndex - self.currentSentenceStartIndex
	-- This is 1 based
	local relativeEndIndex = endIndex - self.currentSentenceStartIndex + 1
	local firstPart = string.sub(self.currentSentence, 1, relativeStartIndex)
	local lastPart = string.sub(self.currentSentence, relativeEndIndex, #self.currentSentence)
	
	local karaokeStyleString = firstPart.."<font color=\"#FFFF00\">"..word.."</font>"..lastPart
	UiTextBus.Event.SetText(self.closedCaptionOut, karaokeStyleString)
end

function closedcaption:OnDeactivate()
    self.notificationHandler:Disconnect();
end

return closedcaption