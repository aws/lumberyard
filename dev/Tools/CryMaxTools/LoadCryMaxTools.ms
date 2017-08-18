-- Loader script to run the CryMaxTools

if csexport != undefined then
(
	if (csexport.get_root_path()) != "" then
	(
		toolsPath = ""
		if globalCryMaxToolsPath != undefined then
			toolsPath = globalCryMaxToolsPath
		else
			toolsPath = csexport.get_root_path() + "\\Tools\\"
		
		toolsPath += "CryMaxTools\\Basic\\Basic.ms"
		
		if doesFileExist toolsPath == true then
			fileIn toolsPath quiet:true
	)
	else
		messageBox "CryENGINE3_Exporter is not linked to any Build!" caption:"Error running CryMaxTools"
)
else
	messageBox "No CryENGINE3_Exporter found!" caption:"Error running CryMaxTools"