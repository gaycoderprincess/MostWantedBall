void WriteLog(const std::string& str) {
	static auto file = std::ofstream("NFSMWBall_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

float GetSFXVolume() {
	return FEDatabase->CurrentUserProfiles[0]->TheOptionsSettings.TheAudioSettings.SoundEffectsVol;
}

IPlayer* GetLocalPlayer() {
	auto& list = PLAYER_LIST::GetList(PLAYER_LOCAL);
	if (list.empty()) return nullptr;
	return list[0];
}

ISimable* GetLocalPlayerSimable() {
	auto ply = GetLocalPlayer();
	if (!ply) return nullptr;
	return ply->GetSimable();
}

template<typename T>
T* GetLocalPlayerInterface() {
	auto ply = GetLocalPlayerSimable();
	if (!ply) return nullptr;
	T* out;
	if (!ply->QueryInterface<T>(&out)) return nullptr;
	return out;
}

auto GetLocalPlayerVehicle() { return GetLocalPlayerInterface<IVehicle>(); }

void ValueEditorMenu(float& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stof(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void ValueEditorMenu(int& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stoi(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void ValueEditorMenu(char* value, int len) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, false);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		strcpy_s(value, len, inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void QuickValueEditor(const char* name, float& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value); }
}

void QuickValueEditor(const char* name, int& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value); }
}

void QuickValueEditor(const char* name, bool& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { value = !value; }
}

void QuickValueEditor(const char* name, char* value, int len) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value, len); }
}

std::vector<IVehicle*> GetActiveVehicles(int driverClass = -1) {
	auto& list = VEHICLE_LIST::GetList(VEHICLE_ALL);
	std::vector<IVehicle*> cars;
	for (int i = 0; i < list.size(); i++) {
		if (!list[i]->IsActive()) continue;
		if (list[i]->IsLoading()) continue;
		if (driverClass >= 0 && list[i]->GetDriverClass() != driverClass) continue;
		cars.push_back(list[i]);
	}
	return cars;
}

bool IsVehicleValidAndActive(IVehicle* vehicle) {
	auto cars = GetActiveVehicles();
	for (auto& car : cars) {
		if (car == vehicle) return true;
	}
	return false;
}

std::vector<ICollisionBody*> GetActiveObjects() {
	auto list = COLLISIONBODY_LIST::_mTable;
	std::vector<ICollisionBody*> objs;
	for (int i = 0; i < list.size(); i++) {
		auto rb = list[i];
		if (!rb) continue;

		auto simable = rb->mCOMObject->Find<ISimable>();
		if (simable->mCOMObject->Find<IVehicle>()) continue;
		objs.push_back(rb);
	}
	return objs;
}

std::vector<IRigidBody*> GetActiveRigidBodies() {
	std::vector<IRigidBody*> rbs;

	auto cars = GetActiveVehicles();
	auto objs = GetActiveObjects();
	for (auto& car : cars) {
		auto rb = car->mCOMObject->Find<IRigidBody>();
		if (rb) rbs.push_back(rb);
	}
	for (auto& obj : objs) {
		auto rb = obj->mCOMObject->Find<IRigidBody>();
		if (rb) rbs.push_back(rb);
	}

	return rbs;
}

IVehicle* GetClosestActiveVehicle(NyaVec3 toCoords) {
	auto sourcePos = toCoords;
	IVehicle* out = nullptr;
	float distance = 99999;
	auto cars = GetActiveVehicles();
	for (auto& car : cars) {
		auto targetPos = *car->GetPosition();
		if ((sourcePos - targetPos).length() < distance) {
			out = car;
			distance = (sourcePos - targetPos).length();
		}
	}
	return out;
}

class ChloeHook {
public:
	static inline std::vector<void(*)()> aHooks;

	ChloeHook(void(*pFunction)()) {
		aHooks.push_back(pFunction);
	}
};

bool IsInLoadingScreen() {
	if (FadeScreen::IsFadeScreenOn()) return true;
	if (cFEng::mInstance->IsPackagePushed("Loading.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_Loading.fng")) return true;
	return false;
}

bool IsInSplashScreenOrIntros() {
	if (TheGameFlowManager.CurrentGameFlowState < GAMEFLOW_STATE_IN_FRONTEND) return true;
	if (cFEng::mInstance->IsPackagePushed("LS_EA_hidef.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_LS_EA_hidef.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("LS_EALogo.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_LS_EALogo.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("LS_PSA.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_LS_PSA.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MW_LS_AttractFMV.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_MW_LS_AttractFMV.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MW_LS_Splash.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("WS_MW_LS_Splash.fng")) return true;

	// savegame selection
	if (cFEng::mInstance->IsPackagePushed("MC_Main.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MC_Main_GC.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MC_Bootup.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MC_Bootup_GC.fng")) return true;
	if (cFEng::mInstance->IsPackagePushed("MC_List.fng")) return true;
	return false;
}

bool IsInNIS() {
	return INIS::mInstance && INIS::mInstance->IsPlaying();
}

bool IsInMovie() {
	return gMoviePlayer;
}

void TeleportPlayer(UMath::Vector3 pos, UMath::Vector3 fwd) {
	Sim::SetStream(&pos, true);
	GetLocalPlayerVehicle()->SetVehicleOnGround(&pos, &fwd);
}

bool IsCarDestroyed(IVehicle* car, bool tirePopsCount = false) {
	if (auto engine = car->mCOMObject->Find<IEngineDamage>()) {
		if (engine->IsBlown()) return true;
		if (engine->IsSabotaged()) return true;
	}
	if (tirePopsCount) {
		if (auto sus = car->mCOMObject->Find<ISpikeable>()) {
			int count = 0;
			for (int i = 0; i < 4; i++) {
				if (sus->GetTireDamage(i) > TIRE_DAMAGE_NONE) count++;
			}
			if (count >= 2) return true;
		}
	}
	return car->IsDestroyed();
}

void DestroyCar(IVehicle* car) {
	if (auto dam = car->mCOMObject->Find<IEngineDamage>()) {
		if (!dam->IsBlown()) dam->Blow();
	}
	if (auto dam = car->mCOMObject->Find<IDamageable>()) {
		if (!dam->IsDestroyed()) {
			dam->Destroy();
		}
	}
}

Camera* GetLocalPlayerCamera() {
	return eViews[EVIEW_PLAYER1].pCamera;
}

NyaVec3 WorldToRenderCoords(NyaVec3 world) {
	return {world.z, -world.x, world.y};
}

NyaVec3 RenderToWorldCoords(NyaVec3 render) {
	return {-render.y, render.z, render.x};
}

NyaMat4x4 WorldToRenderMatrix(NyaMat4x4 world) {
	NyaMat4x4 out;
	out.x = WorldToRenderCoords(world.x);
	out.y = -WorldToRenderCoords(world.y); // v1, up
	out.z = WorldToRenderCoords(world.z);
	out.p = WorldToRenderCoords(world.p);
	return out;
}

// view to world
NyaMat4x4 PrepareCameraMatrix(Camera* pCamera) {
	return pCamera->CurrentKey.Matrix.Invert();
}

// world to view
void ApplyCameraMatrix(Camera* pCamera, NyaMat4x4 mat) {
	auto inv = mat.Invert();
	pCamera->bClearVelocity = true;
	pCamera->SetCameraMatrix((bMatrix4*)&inv, 0);
}

// SetCurrentDirectoryW causes performance drops so only use when needed
wchar_t gDLLDir[MAX_PATH];
class DLLDirSetter {
public:
	wchar_t backup[MAX_PATH];

	DLLDirSetter() {
		GetCurrentDirectoryW(MAX_PATH, backup);
		if (!wcscmp(backup, gDLLDir)) return;
		SetCurrentDirectoryW(gDLLDir);
	}
	~DLLDirSetter() {
		if (!wcscmp(backup, gDLLDir)) return;
		SetCurrentDirectoryW(backup);
	}
};