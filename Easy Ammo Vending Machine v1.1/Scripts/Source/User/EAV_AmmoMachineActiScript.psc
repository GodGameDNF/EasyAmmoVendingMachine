ScriptName EAV_AmmoMachineActiScript Extends ObjectReference

Actor Property PlayerRef Auto
Actor Property AmmoSeller Auto

Activator Property AL_ButtonActi Auto

ObjectReference buttonRef

Auto State firstLoad
	Event OnLoad()
		GotoState("")
		buttonRef = PlaceAtMe(AL_ButtonActi, 1, false, false, false)
		buttonRef.WaitFor3dLoad()
		Utility.WaitMenumode(0.1)
		buttonRef.SetScale(self.getscale())
		Utility.WaitMenumode(0.03)
		buttonRef.AttachTo(Self)
	EndEvent
EndState

Event OnActivate(ObjectReference akActionRef)
	BlockActivation(True, True)
	EAV_Ammo_F4SE.injectAmmo()
	AmmoSeller.ShowBarterMenu()
	Utility.Wait(0.2)
	BlockActivation(False)
EndEvent

Event OnWorkshopObjectDestroyed(ObjectReference akActionRef)
	buttonRef.Disable()
	buttonRef.Delete()
EndEvent