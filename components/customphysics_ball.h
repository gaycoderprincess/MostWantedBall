namespace CustomPhysicsBall {
	bool bEnabled = false;

	bool bDoReset = false;

	float fMoveSpeedLow = 8.0;
	float fMoveSpeed = 15.0;
	float fMoveSpeedHigh = 20.0;
	float fBrakeSpeed = 2.0;
	float fMaxMoveSpeed = 50.0;
	float fBallSize = 2.5;
	float fBallSizeMenu = 1.0;

	float fFwdMoveSpeed = 1.0;
	float fSideMoveSpeed = 1.0;

	b3BodyId BallBody;
	b3BodyId BallBodyMenu;
	b3BodyId GetPlayerBall() {
		if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) return BallBodyMenu;
		return BallBody;
	}

	void EnableBall() {
		bEnabled = true;
		CustomPhysics::bEnabled = true;
		CustomPhysics::bCollectLocalPlayerCar = false;
		CarRender_DontRenderPlayer = true;
	}

	void DisableBall() {
		bEnabled = false;
		CustomPhysics::bEnabled = false;
		CustomPhysics::bCollectLocalPlayerCar = true;
		CarRender_DontRenderPlayer = false;
	}

	float GetBallAcceleration() {
		auto ply = GetLocalPlayerVehicle();
		if (!ply) return fMoveSpeed;
		Physics::Info::Performance perf;
		if (!ply->GetPerformance(&perf)) return fMoveSpeed;
		return std::lerp(fMoveSpeedLow, fMoveSpeedHigh, perf.Acceleration);
	}

	float GetBallTopSpeed() {
		auto ply = GetLocalPlayerVehicle();
		if (!ply) return fMaxMoveSpeed;
		return ply->GetAIVehiclePtr()->GetTopSpeed();
	}

	void OnTick() {
		static bool bOnce = true;
		if (bOnce) {
			NyaAudio::Init(GameWindow);

			{
				b3BodyDef def = b3DefaultBodyDef();
				def.type = b3_dynamicBody;
				def.position = {999,0,0};
				def.enableSleep = false;
				BallBody = b3CreateBody(CustomPhysics::m_worldId, &def);

				b3ShapeDef shapeDef = b3DefaultShapeDef();
				b3Sphere sphere;
				sphere.center = {0,0,0};
				sphere.radius = fBallSize;
				b3CreateSphereShape(BallBody, &shapeDef, &sphere);
			}

			{
				b3BodyDef def = b3DefaultBodyDef();
				def.type = b3_dynamicBody;
				def.position = {0,0,0};
				def.enableSleep = false;
				BallBodyMenu = b3CreateBody(CustomPhysics::m_worldId, &def);

				b3ShapeDef shapeDef = b3DefaultShapeDef();
				b3Sphere sphere;
				sphere.center = {0,0,0};
				sphere.radius = fBallSizeMenu;
				b3CreateSphereShape(BallBodyMenu, &shapeDef, &sphere);
			}

			bOnce = false;
		}

		if (!bEnabled) {
			bDoReset = true;
			return;
		}
		if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING && TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_IN_FRONTEND) {
			bDoReset = true;
			return;
		}
		if (IsInLoadingScreen() || IsInMovie() || IsInSplashScreenOrIntros()) {
			bDoReset = true;
			return;
		}

		auto pos = b3Body_GetPosition(GetPlayerBall());
		if (pos.y < -20) {
			bDoReset = true;
		}
		
		static CNyaTimer gTimer;
		gTimer.Process();

		b3ContactData contactData[8];
		int numCollisions = b3Body_GetContactData(GetPlayerBall(), contactData, 8);

		static int nLastBallCollisions = 0;
		if (numCollisions > nLastBallCollisions) {
			static auto sound = NyaAudio::LoadFile("CwoeeBallin/beachball.wav");
			if (sound) {
				NyaAudio::SetVolume(sound, GetSFXVolume()*0.66);
				NyaAudio::SkipTo(sound, 0, false);
				NyaAudio::Play(sound);
			}
		}
		nLastBallCollisions = numCollisions;

		// destroy cars that touch it
		for (int i = 0; i < numCollisions && i < 8; i++) {
			auto body1 = b3Shape_GetBody(contactData[i].shapeIdA);
			auto body2 = b3Shape_GetBody(contactData[i].shapeIdB);

			auto veh = CustomPhysics::GetVehicleForB3Body(body1);
			if (!veh) veh = CustomPhysics::GetVehicleForB3Body(body2);

			if (!veh) continue;
			if (veh == GetLocalPlayerVehicle()) continue;
			if (veh->GetDriverClass() != DRIVER_COP) continue;
			if (IsCarDestroyed(veh)) continue;
			DestroyCar(veh);
		}

		// ball controls and player car teleport
		if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING) {
			if (bDoReset) {
				if (auto ply = GetLocalPlayerInterface<IRigidBody>()) {
					auto vel = *ply->GetLinearVelocity();
					auto avel = *ply->GetAngularVelocity();
					auto pos = *ply->GetPosition();
					pos.y += 5;
					auto q = *ply->GetOrientation();
					b3Body_SetTransform(GetPlayerBall(), {pos.x, pos.y, pos.z}, {q.x, q.y, q.z, q.w});
					b3Body_SetLinearVelocity(GetPlayerBall(), {vel.x,vel.y,vel.z});
					b3Body_SetAngularVelocity(GetPlayerBall(), {avel.x,avel.y,avel.z});
					bDoReset = false;
				}
			}

			if (FEManager::mPauseRequest || IsInNIS() || GetLocalPlayerVehicle()->IsStaging()) return;

			auto quat = b3Body_GetRotation(GetPlayerBall());
			auto vel = b3Body_GetLinearVelocity(GetPlayerBall());
			auto avel = b3Body_GetAngularVelocity(GetPlayerBall());
			if (auto ply = GetLocalPlayerInterface<IRigidBody>()) {
				UMath::Vector3 v = {pos.x, pos.y, pos.z};
				ply->SetPosition(&v);
				v = {vel.x, vel.y, vel.z};
				ply->SetLinearVelocity(&v);
				ply->SetAngularVelocity(&UMath::Vector3::kZero);

				if (v.length() > 0.01) {
					v.Normalize();
					auto mat = NyaMat4x4::LookAt(v);
					ply->SetOrientation((UMath::Matrix4*)&mat);
				}
			}

			auto mat = PrepareCameraMatrix(GetLocalPlayerCamera());
			auto fwd = RenderToWorldCoords(mat.z);
			auto side = RenderToWorldCoords(mat.x);
			fwd.y = 0;
			side.y = 0;
			fwd.Normalize();
			side.Normalize();

			auto stick = NyaVec3(GetPadKeyState(NYA_PAD_KEY_LSTICK_X) / 32767.0,GetPadKeyState(NYA_PAD_KEY_LSTICK_Y) / -32767.0,0);
			if (IsKeyPressed(VK_LEFT)) {
				stick.x = -1.0;
			}
			if (IsKeyPressed(VK_RIGHT)) {
				stick.x = 1.0;
			}
			if (IsKeyPressed(VK_UP)) {
				stick.y = -1.0;
			}
			if (IsKeyPressed(VK_DOWN)) {
				stick.y = 1.0;
			}

			if (stick.length() > 1.0) {
				stick.Normalize();
			}

			if (IsCarDestroyed(GetLocalPlayerVehicle())) {
				stick = {0,0,0};
			}
			stick.x *= fSideMoveSpeed;
			stick.y *= fFwdMoveSpeed;

			if (GetLocalPlayerInterface<IInput>()->IsLookBackButtonPressed()) {
				stick.x *= -1;
				stick.y *= -1;
			}

			float y = vel.y;
			vel.y = 0;

			auto oldLen = b3Length(vel);

			b3Vec3 velAdd = {0,0,0};

			auto accel = GetBallAcceleration();
			auto maxSpeed = GetBallTopSpeed();
			//AddLogPopup(std::format("accel {:.2f} maxSpeed {:.2f}", accel, maxSpeed));

			velAdd.x += fwd.x * -stick.y * accel * gTimer.fDeltaTime;
			velAdd.z += fwd.z * -stick.y * accel * gTimer.fDeltaTime;
			velAdd.x += side.x * stick.x * accel * gTimer.fDeltaTime;
			velAdd.z += side.z * stick.x * accel * gTimer.fDeltaTime;

			if (b3Length(velAdd) > 0.0) {
				auto velNorm = b3Normalize(vel);
				auto velAddNorm = b3Normalize(velAdd);

				// double force when braking
				auto dot = 1.0 - ((b3Dot(velNorm, velAddNorm) + 1.0) / 2.0);
				velAdd *= (1.0 + (dot * fBrakeSpeed));
			}

			vel += velAdd;

			auto newLen = b3Length(vel);
			if (newLen > maxSpeed) {
				vel.x /= newLen;
				vel.z /= newLen;
				vel.x *= oldLen;
				vel.z *= oldLen;
			}

			// jump button if on ground
			if (numCollisions && (IsKeyJustPressed(VK_SPACE) || IsPadKeyJustPressed(NYA_PAD_KEY_A))) {
				y += 10;
			}
			vel.y = y;

			b3Body_SetLinearVelocity(GetPlayerBall(), vel);
		}
		else if (bDoReset) {
			NyaVec3 pos = {0,0,0};
			pos.y += 5;
			b3Body_SetTransform(GetPlayerBall(), {pos.x, pos.y, pos.z}, {0, 0, 0, 1});
			b3Body_SetLinearVelocity(GetPlayerBall(), {0,0,0});
			b3Body_SetAngularVelocity(GetPlayerBall(), {0,0,0});
			bDoReset = false;
		}

		fFwdMoveSpeed = 1.0;
		fSideMoveSpeed = 1.0;
	}

	void OnTick3D() {
		if (!bEnabled) {
			bDoReset = true;
			return;
		}
		if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING && TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_IN_FRONTEND) {
			bDoReset = true;
			return;
		}
		if (IsInLoadingScreen() || IsInMovie() || IsInSplashScreenOrIntros()) {
			bDoReset = true;
			return;
		}

		static auto mdl = Render3D::CreateModels("beachball.fbx");
		if (!mdl.empty()) {
			UMath::Matrix4 mat;
			auto m = b3MakeMatrixFromQuat(b3Body_GetRotation(CustomPhysicsBall::GetPlayerBall()));

			auto pos = b3Body_GetPosition(CustomPhysicsBall::GetPlayerBall());

			mat.x.x = m.cx.x;
			mat.x.y = m.cx.y;
			mat.x.z = m.cx.z;
			mat.y.x = m.cy.x;
			mat.y.y = m.cy.y;
			mat.y.z = m.cy.z;
			mat.z.x = m.cz.x;
			mat.z.y = m.cz.y;
			mat.z.z = m.cz.z;
			mat.p.x = pos.x;
			mat.p.y = pos.y;
			mat.p.z = pos.z;

			float size = fBallSize;
			if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) size = fBallSizeMenu;

			mat.x *= size;
			mat.y *= size;
			mat.z *= size;

			mdl[0]->RenderAt(WorldToRenderMatrix(mat));
		}
	}

	ChloeHook Init([]{
		aDrawing3DLoopFunctions.push_back(OnTick3D);
		aMainLoopFunctions.push_back(OnTick);
	});
}