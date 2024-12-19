ScriptName EAV_ButtonActiScript Extends ObjectReference

Actor Property PlayerRef Auto

ObjectReference Property FilterCont_Mult_1_5 Auto
ObjectReference Property FilterCont_Mult_0_5 Auto
ObjectReference Property FilterCont_Forced Auto

FormList Property sellRoundsMult_1_5_List Auto
FormList Property sellRoundsMult_0_5_List Auto
FormList Property sellRoundsForcedList Auto

Message Property EAV_MainMessage Auto
Message Property EAV_DespMessage Auto
Message Property EAV_FilterMessage Auto

GlobalVariable Property gAmmoStock Auto
GlobalVariable Property gDespRemove Auto

Event OnActivate(ObjectReference akActionRef)
	bool refeat = true
	while refeat
		int i = EAV_MainMessage.Show(gAmmoStock.GetValue())
		if i == 0
			gAmmoStock.Mod(100)
		elseif i == 1
			if gAmmoStock.GetValue() > 100
				gAmmoStock.Mod(-100)
			Endif
		elseif i == 2
			InputEnableLayer myLayer = InputEnableLayer.Create()
			myLayer.DisablePlayerControls(true, false, false, true, false, true, true, true, true, true, true)

			bool filterRefeat = true
			while filterRefeat
				int c = EAV_FilterMessage.Show()

				objectreference filterCont
				formList filterList

				if c == 0
					filterCont = FilterCont_Mult_1_5
					filterList = sellRoundsMult_1_5_List
				elseif c == 1
					filterCont = FilterCont_Mult_0_5
					filterList = sellRoundsMult_0_5_List
				elseif c == 2
					filterCont = FilterCont_Forced
					filterList = sellRoundsForcedList
				else
					myLayer.reset()
					myLayer.delete()
					filterRefeat = false
					return
				Endif

				filterCont.Activate(PlayerRef)
				Utility.Wait(0.5)

				filterList.revert()
				form[] items = filterCont.GetInventoryItems()
				int w = items.length

				while w > 0
					w -= 1
					form item = items[w]
					if item as ammo
						filterList.AddForm(item as form)
					else
						filterCont.RemoveItem(item, -1, false, playerref)
					endif
				endwhile
			endwhile

			myLayer.reset()
			myLayer.delete()
		elseif i == 3
			int c = EAV_DespMessage.SHow()
			if c == 0
				gDespRemove.SetValue(1)
			Endif
		elseif i == 4
			refeat = false
		Endif
	endwhile
EndEvent
