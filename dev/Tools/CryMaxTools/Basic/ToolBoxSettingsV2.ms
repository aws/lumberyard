------------------------
-- CryMaxTools v2.0
-- Toolbox Settings Rollout v2.0
-- by Sascha Herfort
------------------------

--COMMENTS ABOUT V2.0
--THE TOOLBOX WILL BE REDESIGNED TO SUPPORT THE FOLLOWING THINGS:
--DYNAMIC TAB CREATION TO ENABLE CUSTOMIZABLE TAB LAYOUT
--PLUG-IN FOLDER TO ENABLE EASY INSTALLATION OF NEW ROLLOUTS
--SAVE FEATURE FOR TAB LAYOUT AND PER-ROLLOUT SETTINGS
--
--EACH ROLLOUT FILE NEEDS THE FOLLOWING SETUP TO BE RECOGNIZED AND LOADED BY THE TOOLBOX:



--define the cryMaxToolboxRollout instance first - this obviously requires the toolbox to be loaded first - so to test your rollouts, save this file and reload the toolbox
--the toolbox will execute this code, carefully make sure that it does not break the toolbox!
--the below line marks the beginning of the cryMaxToolboxRollout definition - moving or reusing it somewhere else can make the plugin unusable
--### ROLLOUT CONTAINER DEFINITION BELOW ###
cryMaxTools.toolBox.cryMaxToolboxRollout sName:"Toolbox Settings" sPreferredTab:"Settings" bStatic:true

--define rollout second - this will be assigned to the cryMaxToolboxRollout instance later and deleted afterwards
--the toolbox will execute this code, carefully make sure that it does not break the toolbox!
--the below line marks the beginning of the rollout definition - moving or reusing it somewhere else can make the plugin unusable
--### ROLLOUT DEFINITION BEGIN ###
rollout rltTemporary "Toolbox Settings"
(	
	label lblTest "this is a rollout - woooop!"
	--add more UI elements here
	
--########################################################################################
--TOOLBOX SETTINGS GLOBAL VARIABLES
--########################################################################################
	
--########################################################################################
--TOOLBOX SETTINGS GLOBAL FUNCTIONS
--########################################################################################
	
--########################################################################################
--TOOLBOX SETTINGS UI EVENT HANDLERS
--########################################################################################
	
--### BUTTONS BEGIN ###

--### BUTTONS END ###
)
--### ROLLOUT DEFINITION END ###
--the above line marks the end of the rollout definition - moving or reusing it somewhere else can make the plugin unusable
--adding code after the rollout definition can make the plugin unusable