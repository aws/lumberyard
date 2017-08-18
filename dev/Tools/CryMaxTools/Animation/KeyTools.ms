---------- Key-Tools ----------
--- by Marcus Krautwurst ---
--------------------------------

rollout keytools "Keyframe Tools"
(
		group "Transfer Animation"
		(
			--spinner spnRangeStart "Range" type:#integer width:105 range:[-100000,100000,0] across:2 
			--spinner spnRangeEnd "" type:#integer width:55 range:[-100000,100000,0] offset:[-5,0]
			pickbutton pickbtnTransferKeyOrigin "Initial Object" across: 2 width:70 height:15
			pickbutton pickbtnTransferKeyClone "Target Object" height:15
			checkbox chkbtnTransferPos "Pos" checked:true across:3
			checkbox chkbtnTransferRot "Rot" checked:true
			checkbox chkbtnTransferScale "Scale" checked:true
			button btnTransferKeys "Transfer Animation" width: 155 height:20
		)


	
	on pickbtnTransferKeyOrigin picked obj do
	(
		pickbtnTransferKeyOrigin.text = obj.name
	)
	on pickbtnTransferKeyClone picked obj do
	(
		pickbtnTransferKeyClone.text = obj.name
	)

	
	
	on btnTransferKeys pressed do
	(	
		for i = animationrange.start to animationrange.end do
		(
			with undo on
			(
				with animate on
				(
					at time i
					(
						if chkbtnTransferPos.checked == true then
							(
								pickbtnTransferKeyClone.object.pos = pickbtnTransferKeyOrigin.object.pos
							)
						if chkbtnTransferRot.checked == true then
							(
								pickbtnTransferKeyClone.object.rotation = pickbtnTransferKeyOrigin.object.rotation
							)						
						if  chkbtnTransferScale.checked == true then
							(
								pickbtnTransferKeyClone.object.scale = pickbtnTransferKeyOrigin.object.scale
							)
						pickbtnTransferKeyOrigin.text = "Initial Object"
						pickbtnTransferKeyClone.text = "Target Object"
					)
				)
			)
		)
	)
)




--####### Load the Rollout with ROMan ########--
cryMaxTools.basic.ROMan.cryAdd "keytools" keytools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "keytools")