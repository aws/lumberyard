-- A placeholder for a result that hasn't happened yet.  A function can be registered to handle the result when it is available.
local asyncresult = {}

function asyncresult:new(instance)
    instance = instance or {}
    instance.handlers = {}
    self.__index = self
    setmetatable(instance, self)
    return instance
end

-- Set the result and run any registered functions.
function asyncresult:Complete(...)
    self.completed = true
    self.result = arg

    self.threadHandler:RunOnMainThread(function()
        local handler = nil
        repeat
            handler = table.remove(self.handlers)
            if handler then
                handler(unpack(arg))
            end
        until not handler
    end)
end

-- Register a function to be called when the result is available.
function asyncresult:OnComplete(handler, last)
    if self.completed then
        handler(unpack(self.result))
    else
        if last then
            table.insert(self.handlers, 1, handler)
        else
            table.insert(self.handlers, handler)
        end
    end
end

-- Combine two or more asyncresult instances.
function asyncresult:Chain(...)
    local chainResult = asyncresult:new{
        threadHandler = self.threadHandler
    }
    chainResult.remainingResults = arg.n + 1
    chainResult.combinedResults = {}
    
    self:OnComplete(chainResult:CreateChainFunction(1))
    for index,result in ipairs(arg) do
        result:OnComplete(chainResult:CreateChainFunction(index + 1))
    end
    
    return chainResult
end

-- Helper for combining results.
function asyncresult:CreateChainFunction(index)
    local chainResult = self
    return function(...)
        chainResult.combinedResults[index] = arg
        chainResult.remainingResults = chainResult.remainingResults - 1;
        if (chainResult.remainingResults == 0) then
            chainResult:Complete(unpack(chainResult.combinedResults))
        end
    end
end

return asyncresult
