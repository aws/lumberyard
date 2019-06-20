
-- This file is (optionally) loaded and executed on C3DEngine::LoadPhysicsData() call.

--------------------------------------
-- Register explosion and crack shapes
--------------------------------------

-- RegisterExposionShape params, in this order:
-- 1) Boolean shape CGF name
-- 2) Characteristic "size" of the shape: Ideally it should roughly represent the linear dimensions of the hole
--    Whenever a carving happens, it requests a desired hole size (set in explosion params or surfacetype params), and the shape is scaled by [desired size/characteristic size]
--    If several shapes are registered with the same id, the one with the size closest to the requested will be selected, and if there are several shapes with this size, one of them will be selected randomly
-- 3) Breakability index (0-based): Used to identify the breakable material
-- 4) Shapes relative probability: When several shapes with the same size appear as candidates for carving, they are selected with these relative probabilities
-- 5) Splinters CGF: Used for trees to add splinters at the place where it broke
-- 6) Splinters Scale: Splinters CGF is scaled by [break radius * splinters scale], i.e. splinters scale should be roughly [1 / most natural radius for the original CGF size]
-- 7) Splinters Particle FX name: Is played when a splinters-based constraint breaks and splinters disappear
-- 
-- RegisterExposionCrack params, in this order:
-- 1) Crack shape CGF name (must have 3 helpers that mark the corners, named "1","2","3")
-- 2) Breakability index (same meaning as for the explosion shapes)

--[[  EXAMPLES
Physics.RegisterExplosionShape("Objects/default/explosion_shape/tree_broken_shape.cgf", 7, 2, 1, "",0,"");
Physics.RegisterExplosionShape("Objects/default/explosion_shape/tree_broken_shape2.cgf", 1.3, 3, 1, "",0,"");
Physics.RegisterExplosionShape("Objects/default/explosion_shape/tree_broken_shape3.cgf", 1.3, 5, 1, 
                               "Objects/default/explosion_shape/trunk_splinters_a.cgf",1.6,"breakable_objects.tree_break.small");
--]]