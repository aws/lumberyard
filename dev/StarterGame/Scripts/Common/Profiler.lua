local profiler = {}

function profiler:OnActivate()
	self.profileEventId = GameplayNotificationId(self.entityId, "Profile", "float");
	self.profileHandler = GameplayNotificationBus.Connect(self, self.profileEventId);
	self.isProfiling = false;
end
local calls, total, this, funcNames, frac, ave = {}, {}, {}, {}, {}, {};
local totalTime
function Hook(event)
  local i = debug.getinfo(2, "Sln")
  if i.what ~= 'Lua' then return end
  local func = i.source..':'..i.linedefined;
  local currTime = StarterGameUtility.GetTimeNowMicroSecond();
	--https://stackoverflow.com/questions/15725744/easy-lua-profiling
  if event == 'call' then
    this[func] = currTime;
	if(i.name) then funcNames[func] = i.name end
  else
	if(this[func] ~= nil) then
    	local time = currTime - this[func]
		totalTime = totalTime + time
    	total[func] = (total[func] or 0) + time
    	calls[func] = (calls[func] or 0) + 1
	else
		Debug.Log(string.format("Profile error %s - %s", func, event));
	end
  end
end

function startProfiling()
	calls = {};
	total = {};
	this = {};
	totalTime = 0;
	funcNames = {};
	debug.sethook(Hook, "cr");
end
function generateResult(func)
	local format = "Function %s (%s) took %d ms (%d us ave) after %d calls"
	local name = funcNames[func];
	if(name == nil) then name = "UNKNOWN" end
	local totalTime = total[func]
	local aveTime = totalTime / calls[func]
	return string.format(format, func, name, total[func] / 1000, aveTime, calls[func])
end
function stopProfiling()
	debug.sethook();
	local keysTotal, keysAve = {}, {};
	frac = {};
	local keyCount = 0
	for f,time in pairs(total) do
		ave[f] = time / calls[f]
		table.insert(keysTotal, f)
		table.insert(keysAve, f)
		keyCount = keyCount + 1;
	end
	table.sort(keysTotal, function(a, b) return total[a] > total[b] end);
	table.sort(keysAve, function(a, b) return ave[a] > ave[b] end);
	local heading = string.format("Highest Total%s\tHighest Average", string.rep(" ", 225));
	Debug.Log(heading)
	Debug.Log(string.rep("-", 350));
	for i = 1, keyCount, 1 do
		local totalKey = keysTotal[i]
		local aveKey = keysAve[i]
		local totalResult = i..": "..generateResult(keysTotal[i])
		local aveResult = generateResult(keysAve[i])
		local padding = string.rep(" ", 170 - string.len(totalResult))
  		Debug.Log(string.format("%s%s\t%s", totalResult, padding, aveResult ));
	end
end
function profiler:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.profileEventId) then
		if(self.isProfiling == false) then
			self.isProfiling = true;
			startProfiling();
		else
			self.isProfiling = false;
			stopProfiling();
		end
	end
end


function profiler:OnDeactivate()
	self.profileHandler:Disconnect();
	self.profileHandler = nil;
	debug.sethook();
end

return profiler;