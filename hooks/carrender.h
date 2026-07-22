bool CarRender_DontRenderPlayer = false;

auto CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))nullptr;
int __thiscall CarGetVisibleStateHooked(eView* a1, const bVector3* a2, const bVector3* a3, bMatrix4* a4) {
	auto carMatrix = (NyaMat4x4*)a4;
	if (CarRender_DontRenderPlayer && TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING && !IsInLoadingScreen()) {
		if (GetClosestActiveVehicle(RenderToWorldCoords(carMatrix->p)) == GetLocalPlayerVehicle()) {
			// hacky solution!! it works but checking some CarRenderInfo ptr against the player and disabling DrawCars would be way better
			carMatrix->p = {0,0,0};
			return CarGetVisibleStateOrig(a1, a2, a3, a4);
		}
	}
	return CarGetVisibleStateOrig(a1, a2, a3, a4);
}

ChloeHook Hook_CarRender([]() {
	CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x74E346, &CarGetVisibleStateHooked);
});