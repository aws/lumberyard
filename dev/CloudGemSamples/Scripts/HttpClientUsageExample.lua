httpclientusageexample ={
	Properties =
	{
	}
}

function httpclientusageexample:OnActivate()
    self.notificationHandler = HttpClientComponentNotificationBusHandler(self, self.entityId);
    self.requestSender = HttpClientComponentRequestBusSender(self.entityId);

    self.requestSender:MakeHttpRequest("https://jsonplaceholder.typicode.com/users", "GET", "");
end

function httpclientusageexample:OnDeactivate()
    self.notificationHandler:Disconnect();
    self.requestSender = nil;
end

function httpclientusageexample:OnHttpRequestSuccess(responseCode, responseBody)
    Debug.Log("HTTP RESPONSE -- " .. responseCode);
    Debug.Log("HTTP BODY -- " .. responseBody);
end

function httpclientusageexample:OnHttpRequestFailure(errorCode)
    Debug.Log("HTTP Error-- " .. errorCode);
end

return httpclientusageexample
