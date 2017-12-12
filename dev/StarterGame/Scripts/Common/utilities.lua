
local PreviousRandomValue = -999.999;

local utilities = 
{
	-- Blend Parameters:
	-- Lumberyard can extract some pieces of information by default (such as 'MoveSpeed',
	-- 'TurnAngle', etc.) and they can be used as blend weight parameters. However there
	-- are times where we want to control them ourselves; hence why we use a few of the
	-- custom parameters for them.
	BP_MoveSpeed	= eMotionParamID_BlendWeight,	-- for moving
	BP_TurnAngle	= eMotionParamID_BlendWeight2,	-- for turning
	BP_SlopeAngle	= eMotionParamID_BlendWeight3,	-- for going up/down slopes
	BP_Aim			= eMotionParamID_BlendWeight4,	-- for aim I.K.
	BP_Hit			= eMotionParamID_BlendWeight5,	-- for when hit (injured)
	
	
	SetBlendParam = function(sm, blendParam, blendParamID)
		CharacterAnimationRequestBus.Event.SetBlendParameter(sm.EntityId, blendParamID, blendParam);
	end,
	
	-- Calculate the SlopeAngle blend parameter
	GetSlopeAngle = function(movementcontroller)
		local tm = TransformBus.Event.GetWorldTM(movementcontroller.entityId);
		local pos = tm:GetTranslation();
		local up = Vector3(0,0,1);
		local dist = 2.0;
		
		local rayCastConfig = RayCastConfiguration();
		rayCastConfig.origin = pos + Vector3(0,0,0.5);
		rayCastConfig.direction =  Vector3(0,0,-1);
		rayCastConfig.maxDistance = dist;
		rayCastConfig.maxHits = 1;
		rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Terrain;		
		local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
		if(#hits > 0) then
			up = hits[1].normal;
			dist = (hits[1].position - pos):GetLength();
		end
		local forward = tm:GetColumn(1):GetNormalized();
		local right = Vector3.Cross(forward, up);
		forward = Vector3.Cross(up, right);
		local slopeAngle = Math.ArcCos(-forward.z) / math.pi;
		local realAngle = Math.ArcCos(up.z);
		return slopeAngle, realAngle, dist;
	end,
	
	GetMoveSpeed = function(moveLocal)
        local moveMag = moveLocal:GetLength();
        if (moveMag > 1.0) then 
            moveMag = 1.0 
        end
		return moveMag;
	end,
	
	GetDistanceMovedDown = function(prev, curr)
		local prevPos = prev:GetTranslation();
		local currPos = curr:GetTranslation();
		return prevPos.z - currPos.z;
	end,
	
	CheckFragmentPlaying = function(entityId, requestId)
    	if (requestId) then
        	local status = MannequinRequestsBus.Event.GetRequestStatus(entityId, requestId);
        	return status == 1 or status == 2
        end	
		return false;
	end,
	
	GetDebugManagerBool = function(name, default)
		local doIt = default;
		local response = DebugManagerComponentRequestsBus.Broadcast.GetDebugBool(name);
		-- If 'response' is nil then it means the DebugManager doesn't exist.
		if (response ~= nil) then
			doIt = response;
		end
		return doIt;
	end,
	
	IsNaN = function(value)
		return value ~= value;
	end,
	
	Abs = function(value)
		return value * Math.Sign(value);
	end,

	Clamp = function(_n, _min, _max)
		if (_n > _max) then _n = _max; end
		if (_n < _min) then _n = _min; end
		return _n;
	end,
	
	Sqr = function(value)
		return value * value;
	end,
	
	-- Calculates a random value between 0 and 'value'.
	-- 'pm' should be between 0 and 1 and is used for shifting the 'mid-point' of the result.
	-- For example:
	--	- a pm of 0 means the result will always be positive;
	--	- a pm of 1 means the result will always be negative;
	--	- a pm of 0.5 means the result will be between -(value/2) and +(value/2)
	RandomPlusMinus = function(value, pm)
		local r = math.random();
		-- For some reason A.I. can sometimes be given the same value when attempting to calculate
		-- a shooting delay. I have no idea why this occurs as they're on different update buses
		-- and it doesn't happen when they get random juke values. As a result, I've put in this
		-- brute-force fix so that this function refuses to return the same value twice in a row.
		if (PreviousRandomValue == r) then
			r = math.random();
		end
		PreviousRandomValue = r;
		--Debug.Log("Random value: " .. tostring(r) .. ", Time : " .. tostring(os.time()));
		return (r * value) - (value * pm);
	end,
	
	Lerp = function(a, b, i)
		return a + ((b - a) * i);
	end,
	
	LogIf = function(object, msg)
		if (object.shouldLog == true) then
			Debug.Log(tostring(object.entityId) .. " " .. msg);
		end
	end,
}

return utilities;
