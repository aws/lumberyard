local AnimParamUpdateFlags = {};
local AnimParamUpdateFlags_mt = { __index = AnimParamUpdateFlags }

function AnimParamUpdateFlags:create()
	local new_inst = {}
	setmetatable( new_inst, AnimParamUpdateFlags_mt )
	new_inst:reset()
	return new_inst
end

function AnimParamUpdateFlags:reset()
	self.Speed = { update = true, show = false };
	self.TurnSpeed = { update = true, show = false };
	self.StrafeTurnSpeed = { update = false, show = false };
	self.SlopeAngle = { update = true, show = false };
	self.TurnAngle = { update = true, show = false };
	self.PreviousSpeed = { update = true, show = false };
	self.TargetPos = { update = true, show = false };
	self.WantsToJump = { update = true, show = true };
	self.Aiming = { update = true, show = true };
	self.AimAtTarget = { update = true, show = false };
	self.ShouldLand = { update = true, show = false };
	self.LandType = { update = false, show = false };
	self.IsFalling = { update = true, show = false };
	self.PreviousTurning = { update = true, show = false };
	self.WantsToStrafe = { update = true, show = false };
	self.MotionTurnTransitionReached = { update = false, show = false };
	self.MotionTurnEarlyOut = { update = false, show = false };
	self.MotionTurnMotionTurnTag = { update = false, show = false };
	self.WantsToCrouch = { update = true, show = false };
	self.WantsToInteract = { update = true, show = false };
	self.DeathState = { update = true, show = false };
	self.IdleTurnTransition = { update = false, show = false };
	self.WasHit = { update = true, show = true };
	self.HitDirection = { update = true, show = false };
	self.Shot = { update = true, show = false };
	self.FallingDuration = { update = false, show = false };
end

return AnimParamUpdateFlags;