namespace CustomPhysics {
	void ModifyCustomCollisionInstance(WCollisionInstance* inst, const std::vector<WCollisionTri>& tris, NyaVec3 centerPos, Attrib::Collection* surfaceRef = nullptr) {
		float width = 0.0;
		float length = 0.0;
		float height = 0.0;

		for (auto& tri : tris) {
			width = std::max(std::abs(tri.fPt0.x-centerPos.x), width);
			width = std::max(std::abs(tri.fPt1.x-centerPos.x), width);
			width = std::max(std::abs(tri.fPt2.x-centerPos.x), width);
			height = std::max(std::abs(tri.fPt0.y-centerPos.y), height);
			height = std::max(std::abs(tri.fPt1.y-centerPos.y), height);
			height = std::max(std::abs(tri.fPt2.y-centerPos.y), height);
			length = std::max(std::abs(tri.fPt0.z-centerPos.z), length);
			length = std::max(std::abs(tri.fPt1.z-centerPos.z), length);
			length = std::max(std::abs(tri.fPt2.z-centerPos.z), length);
		}

		NyaVec3 tmp = {width,length,0};

		inst->fInvMatRow0Width.x = 1.0;
		inst->fInvMatRow0Width.y = 0.0;
		inst->fInvMatRow0Width.z = 0.0;
		inst->fInvMatRow0Width.w = width;
		inst->fInvMatRow2Length.x = 0.0;
		inst->fInvMatRow2Length.y = 0.0;
		inst->fInvMatRow2Length.z = 1.0;
		inst->fInvMatRow2Length.w = length;
		inst->fInvPosRadius.x = -centerPos.x;
		inst->fInvPosRadius.y = -centerPos.y;
		inst->fInvPosRadius.z = -centerPos.z;
		inst->fInvPosRadius.w = tmp.length();
		inst->fHeight = height;

		auto article = inst->fCollisionArticle;
		auto data = (uint8_t*)inst->fCollisionArticle;

		if (tris.size() != article->fNumStrips) {
			MessageBoxA(nullptr, std::format("Attempted to modify collision instance with {} tris to have {} tris", article->fNumStrips, tris.size()).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}

		auto numStrips = article->fNumStrips;
		auto numVerts = numStrips*3;

		size_t stripSphere_begin = sizeof(WCollisionArticle);
		size_t strips_begin = stripSphere_begin+(sizeof(WCollisionStripSphere)*numStrips);
		size_t surfaces_begin = strips_begin+(sizeof(WCollisionStrip)*numVerts);

		auto stripSphere = (WCollisionStripSphere*)(data+stripSphere_begin);

		auto stripList = (WCollisionStrip*)(data+strips_begin);

		float stripMult = 128.0; // 0.0078125
		float stripSphereMult = 16.0; // 0.0625

		for (int i = 0; i < tris.size(); i++) {
			auto tri = tris[i];
			tri.fPt0.x += inst->fInvPosRadius.x;
			tri.fPt0.y += inst->fInvPosRadius.y;
			tri.fPt0.z += inst->fInvPosRadius.z;
			tri.fPt1.x += inst->fInvPosRadius.x;
			tri.fPt1.y += inst->fInvPosRadius.y;
			tri.fPt1.z += inst->fInvPosRadius.z;
			tri.fPt2.x += inst->fInvPosRadius.x;
			tri.fPt2.y += inst->fInvPosRadius.y;
			tri.fPt2.z += inst->fInvPosRadius.z;

			tri.fPt0 *= stripMult;
			tri.fPt1 *= stripMult;
			tri.fPt2 *= stripMult;

			stripSphere->fPos = {0,0,0}; // ?? this is still relative right?
			stripSphere->fOffset = ((uintptr_t)stripList)-((uintptr_t)data)-sizeof(WCollisionArticle); // offset to strip from start of strip data?
			stripSphere->fRadius = stripSphereMult * inst->fInvPosRadius.w;
			stripSphere++;

			// one tri per strip, very inefficient but alas i am stupid
			stripList->numTrisOrSurfaceId = 3;
			stripList->pt[0] = tri.fPt0.x;
			stripList->pt[1] = tri.fPt0.y;
			stripList->pt[2] = tri.fPt0.z;
			stripList++;
			stripList->numTrisOrSurfaceId = 0;
			stripList->pt[0] = tri.fPt1.x;
			stripList->pt[1] = tri.fPt1.y;
			stripList->pt[2] = tri.fPt1.z;
			stripList++;
			stripList->numTrisOrSurfaceId = 0;
			stripList->pt[0] = tri.fPt2.x;
			stripList->pt[1] = tri.fPt2.y;
			stripList->pt[2] = tri.fPt2.z;
			stripList++;
		}

		if (surfaceRef) {
			auto surfaceList = (Attrib::Collection**)(data + surfaces_begin);
			surfaceList[0] = surfaceRef;
		}

		// fInvMatRow0Width 1.0 0.0 0.0 33.14
		// fInvMatRow2Length 0.0 0.0 1.0 44.8
		// fInvPosRadius 2499.75 -448.0 -1757.5
		// fHeight 576 - actual height from top to bottom / 2 ? or amount to move by? no that'd be 57.6
		// fFlags 0
		// fGroupNumber 0
		// player pos -2508 148 1762
	}

	WCollisionInstance* CreateCustomCollisionInstance(const std::vector<WCollisionTri>& tris, NyaVec3 centerPos, Attrib::Collection* surfaceRef = nullptr) {
		if (!surfaceRef) {
			surfaceRef = Attrib::FindCollection(Attrib::StringHash32("simsurface"), Attrib::StringHash32("unknown"));
		}

		auto inst = new WCollisionInstance;
		inst->fIterStamp = 0;
		inst->fFlags = 0;
		inst->fHeight = 0.0;
		inst->fGroupNumber = 0;
		inst->fRenderInstanceInd = 0; // todo?

		size_t numStrips = tris.size();
		size_t numVerts = numStrips*3;

		size_t dataSize = sizeof(WCollisionArticle)+4+(sizeof(WCollisionStripSphere)*numStrips)+(sizeof(WCollisionStrip)*numVerts);
		auto data = new uint8_t[dataSize];
		memset(data,0,dataSize);

		auto article = (WCollisionArticle*)data;
		article->fNumStrips = numStrips;
		article->fStripsSize = (sizeof(WCollisionStripSphere)*numStrips)+(sizeof(WCollisionStrip)*numVerts);
		article->fNumEdges = 0;
		article->fEdgesSize = 0;
		article->fResolvedFlag = 0;
		article->fNumSurfaces = 1;
		article->fSurfacesSize = 4;
		article->fIntermediatObjInd = 0; // ??
		article->fFlags = 0;
		inst->fCollisionArticle = article;

		ModifyCustomCollisionInstance(inst, tris, centerPos, surfaceRef);

		return inst;
	}

	WCollisionInstance* pMenuPlatform = nullptr;
	void ProcessMenuPlatform() {
		auto pos = NyaVec3(0,0,0);

		float width = 32.0;
		float length = 32.0;

		WCollisionTri tri = {};
		tri.fPt2 = {pos.x - (width/2.0), pos.y, pos.z - (length/2.0)};
		tri.fPt1 = {pos.x - (width/2.0), pos.y, pos.z + (length/2.0)};
		tri.fPt0 = {pos.x + (width/2.0), pos.y, pos.z - (length/2.0)};

		auto tri3 = tri;
		tri3.fPt2 = {pos.x + (width/2.0), pos.y, pos.z + (length/2.0)};

		auto triRev = tri;
		triRev.fPt0 = tri.fPt2;
		triRev.fPt1 = tri.fPt1;
		triRev.fPt2 = tri.fPt0;
		auto tri3Rev = tri3;
		tri3Rev.fPt0 = tri3.fPt2;
		tri3Rev.fPt1 = tri3.fPt1;
		tri3Rev.fPt2 = tri3.fPt0;

		std::vector<WCollisionTri> tris;
		tris.push_back(tri);
		tris.push_back(tri3);
		tris.push_back(triRev);
		tris.push_back(tri3Rev);

		static auto inst = CreateCustomCollisionInstance(tris, pos);
		ModifyCustomCollisionInstance(inst, tris, pos);
		pMenuPlatform = inst;
	}

	b3WorldId m_worldId;

	b3MeshData* CreateMesh(b3Vec3* vertices, int numVertices, int* indices, int numIndices) {
		b3MeshDef def = {};
		def.vertices = vertices;
		def.vertexCount = numVertices;
		def.indices = indices;
		def.triangleCount = numIndices / 3;
		def.materialIndices = nullptr;
		def.useMedianSplit = false; // todo?
		def.identifyEdges = true;
		def.weldVertices = true;
		def.weldTolerance = 0.002f;

		b3MeshData* meshData = b3CreateMesh( &def, nullptr, 0 );
		return meshData;
	}

	struct CustomArticleInstance {
		int nSceneryGroupId;
		std::vector<WCollisionTri> aTriStrips;
		std::vector<WCollisionTri> aBarriers;
		b3MeshData* pB3Mesh;
		b3BodyId nB3Body;
		bool bB3MeshEnabled;
	};

	struct CustomArticle {
		std::vector<CustomArticleInstance> aInstances;
	};
	CustomArticle aCollisionArticles[2701];

	const int COLLISIONARTICLE_CUSTOM = 2700;

	struct CustomObjectInstance {
		b3BodyId nB3Body;
		IRigidBody* pGameBody;
		bool bReturnChangesToGame = false;
	};
	std::vector<CustomObjectInstance> aB3Objects;

	IRigidBody* GetGameBodyForB3Body(b3BodyId body) {
		for (auto& obj : aB3Objects) {
			if (B3_ID_EQUALS(obj.nB3Body, body)) {
				return obj.pGameBody;
			}
		}
		return nullptr;
	}

	IVehicle* GetVehicleForB3Body(b3BodyId body) {
		if (auto game = GetGameBodyForB3Body(body)) return game->mCOMObject->Find<IVehicle>();
		return nullptr;
	}

	void ProcessCollisionBarriers(CustomArticleInstance* article, WCollisionBarrier* list, int count, NyaVec3 offset) {
		for (int i = 0; i < count; i++) {
			auto ptMin = list[i].fPts[0];
			auto ptMax = list[i].fPts[1];
			ptMin -= offset;
			ptMax -= offset;

			// first tri
			WCollisionTri tri;
			tri.fPt2.x = ptMin.x;
			tri.fPt2.y = ptMin.y;
			tri.fPt2.z = ptMin.z;
			tri.fPt1.x = ptMin.x;
			tri.fPt1.y = ptMax.y;
			tri.fPt1.z = ptMin.z;
			tri.fPt0.x = ptMax.x;
			tri.fPt0.y = ptMax.y;
			tri.fPt0.z = ptMax.z;

			article->aBarriers.push_back(tri);

			// second tri
			tri.fPt2.x = ptMin.x;
			tri.fPt2.y = ptMin.y;
			tri.fPt2.z = ptMin.z;
			tri.fPt1.x = ptMax.x;
			tri.fPt1.y = ptMin.y;
			tri.fPt1.z = ptMax.z;
			tri.fPt0.x = ptMax.x;
			tri.fPt0.y = ptMax.y;
			tri.fPt0.z = ptMax.z;
			article->aBarriers.push_back(tri);

			// first tri
			tri.fPt0.x = ptMin.x;
			tri.fPt0.y = ptMin.y;
			tri.fPt0.z = ptMin.z;
			tri.fPt1.x = ptMin.x;
			tri.fPt1.y = ptMax.y;
			tri.fPt1.z = ptMin.z;
			tri.fPt2.x = ptMax.x;
			tri.fPt2.y = ptMax.y;
			tri.fPt2.z = ptMax.z;

			article->aBarriers.push_back(tri);

			// second tri
			tri.fPt0.x = ptMin.x;
			tri.fPt0.y = ptMin.y;
			tri.fPt0.z = ptMin.z;
			tri.fPt1.x = ptMax.x;
			tri.fPt1.y = ptMin.y;
			tri.fPt1.z = ptMax.z;
			tri.fPt2.x = ptMax.x;
			tri.fPt2.y = ptMax.y;
			tri.fPt2.z = ptMax.z;
			article->aBarriers.push_back(tri);
		}
	}

	void ProcessCollisionArticle(int articleId, WCollisionInstance* inst) {
		if (!inst) return;

		auto article = inst->fCollisionArticle;
		if (!article) return;

		UMath::Matrix4 instMat;
		inst->MakeMatrix(&instMat, true);

		// filter out unused stuff
		//if (inst->fGroupNumber && !SceneryGroupEnabledTable[inst->fGroupNumber]) return;

		auto articles_end_ptr = (uintptr_t)(&article[1]);

		aCollisionArticles[articleId].aInstances.push_back({});
		auto articleInst = &aCollisionArticles[articleId].aInstances[aCollisionArticles[articleId].aInstances.size()-1];

		articleInst->nSceneryGroupId = inst->fGroupNumber;

		auto stripSphere = (WCollisionStripSphere*)articles_end_ptr;
		auto strip = (WCollisionStrip*)(&stripSphere[article->fNumStrips]);
		for (int i = 0; i < article->fNumStrips; i++) {
			int numToIterate = strip->numTrisOrSurfaceId - 2;
			for (int j = 0; j < numToIterate; j++) {
				WCollisionTri tri;
				WCollisionStrip::MakeFace(strip, j, &stripSphere->fPos, &tri);
				tri.fSurfaceRef = *(Attrib::Collection**)(articles_end_ptr + (4 * tri.fSurface.fSurface) + article->fStripsSize + article->fEdgesSize);

				tri.fPt0 -= instMat.p;
				tri.fPt1 -= instMat.p;
				tri.fPt2 -= instMat.p;

				articleInst->aTriStrips.push_back(tri);

				auto flip = tri;
				flip.fPt0 = tri.fPt2;
				flip.fPt1 = tri.fPt1;
				flip.fPt2 = tri.fPt0;
				articleInst->aTriStrips.push_back(flip);
			}
			strip += strip->numTrisOrSurfaceId;
			stripSphere++;
		}

		ProcessCollisionBarriers(articleInst, (WCollisionBarrier*)(articles_end_ptr + article->fStripsSize), article->fNumEdges, instMat.p);
	}

	void ConvertCollisionArticle(CustomArticleInstance& article) {
		if (article.pB3Mesh) return;

		std::vector<b3Vec3> vertices;
		std::vector<int> indices;
		for (auto& tri : article.aTriStrips) {
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt0.x, tri.fPt0.y, tri.fPt0.z});
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt1.x, tri.fPt1.y, tri.fPt1.z});
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt2.x, tri.fPt2.y, tri.fPt2.z});
		}

		for (auto& tri : article.aBarriers) {
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt0.x, tri.fPt0.y, tri.fPt0.z});
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt1.x, tri.fPt1.y, tri.fPt1.z});
			indices.push_back(vertices.size());
			vertices.push_back({tri.fPt2.x, tri.fPt2.y, tri.fPt2.z});
		}

		if (!vertices.empty()) {
			article.pB3Mesh = CreateMesh(&vertices[0], vertices.size(), &indices[0], indices.size());

			b3BodyDef def = b3DefaultBodyDef();
			def.type = b3_staticBody;
			def.position = {0,0,0};
			article.nB3Body = b3CreateBody(m_worldId, &def);

			b3ShapeDef shapeDef = b3DefaultShapeDef();
			//shapeDef.materials = materials;
			//shapeDef.materialCount = 7;
			b3CreateMeshShape(article.nB3Body, &shapeDef, article.pB3Mesh, b3Vec3_one);

			article.bB3MeshEnabled = true;
		}
	}

	bool bCollectLocalPlayerCar = true;
	float fWorldObjectMassScale = 100.0;
	float fWorldObjectMassMinimum = 400.0;
	void CollectWorldObjects() {
		for (auto& obj : aB3Objects) {
			if (obj.pGameBody && obj.bReturnChangesToGame) {
				auto m = b3MakeMatrixFromQuat(b3Body_GetRotation(obj.nB3Body));

				UMath::Matrix4 mat;
				mat.x.x = m.cx.x;
				mat.x.y = m.cx.y;
				mat.x.z = m.cx.z;
				mat.y.x = m.cy.x;
				mat.y.y = m.cy.y;
				mat.y.z = m.cy.z;
				mat.z.x = m.cz.x;
				mat.z.y = m.cz.y;
				mat.z.z = m.cz.z;
				obj.pGameBody->SetOrientation(&mat);

				auto p = b3Body_GetPosition(obj.nB3Body);
				auto v = b3Body_GetLinearVelocity(obj.nB3Body);
				auto av = b3Body_GetAngularVelocity(obj.nB3Body);

				UMath::Vector3 pos;
				pos.x = p.x;
				pos.y = p.y;
				pos.z = p.z;
				obj.pGameBody->SetPosition(&pos);

				UMath::Vector3 vel;
				vel.x = v.x;
				vel.y = v.y;
				vel.z = v.z;
				obj.pGameBody->SetLinearVelocity(&vel);

				UMath::Vector3 avel;
				avel.x = av.x;
				avel.y = av.y;
				avel.z = av.z;
				obj.pGameBody->SetAngularVelocity(&avel);
			}

			b3DestroyBody(obj.nB3Body);
		}
		aB3Objects.clear();

		auto objs = GetActiveRigidBodies();
		for (auto& rb : objs) {
			if (!bCollectLocalPlayerCar && rb == GetLocalPlayerInterface<IRigidBody>()) continue;
			if (rb->GetMass() < fWorldObjectMassMinimum) continue;

			auto objInst = CustomObjectInstance();

			UMath::Vector3 dim;
			rb->GetDimension(&dim);

			auto p = *rb->GetPosition();

			UMath::Matrix4 m;
			rb->GetMatrix4(&m);

			b3Matrix3 m3;
			m3.cx.x = m.x.x;
			m3.cx.y = m.x.y;
			m3.cx.z = m.x.z;
			m3.cy.x = m.y.x;
			m3.cy.y = m.y.y;
			m3.cy.z = m.y.z;
			m3.cz.x = m.z.x;
			m3.cz.y = m.z.y;
			m3.cz.z = m.z.z;

			b3BodyDef def = b3DefaultBodyDef();
			def.type = b3_dynamicBody;
			def.position = {p.x,p.y,p.z};
			def.rotation = b3MakeQuatFromMatrix(&m3);
			objInst.nB3Body = b3CreateBody(m_worldId, &def);

			b3ShapeDef shapeDef = b3DefaultShapeDef();
			auto hull = b3MakeBoxHull(dim.x, dim.y, dim.z);
			b3CreateHullShape(objInst.nB3Body, &shapeDef, &hull.base);

			auto vel = *rb->GetLinearVelocity();
			auto avel = *rb->GetAngularVelocity();
			auto mass = rb->GetMass() * fWorldObjectMassScale;
			b3Body_SetLinearVelocity(objInst.nB3Body, {vel.x,vel.y,vel.z});
			b3Body_SetAngularVelocity(objInst.nB3Body, {avel.x,avel.y,avel.z});
			b3MassData massData;
			massData.mass = mass;
			massData.inertia = {mass*dim.x,mass*dim.y,mass*dim.z};
			massData.center = {0,0,0};
			b3Body_SetMassData(objInst.nB3Body, massData);

			objInst.pGameBody = rb;
			aB3Objects.push_back(objInst);
		}
	}

	bool bEnabled = false;
	void OnWorldTick() {
		if (!bEnabled) return;
		if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING && TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_IN_FRONTEND) return;
		if (IsInLoadingScreen() || IsInMovie() || IsInSplashScreenOrIntros()) return;

		static bool bOnce = true;
		if (bOnce) {
			ProcessMenuPlatform();

			ProcessCollisionArticle(COLLISIONARTICLE_CUSTOM, pMenuPlatform);
			for (auto& inst : aCollisionArticles[COLLISIONARTICLE_CUSTOM].aInstances) {
				ConvertCollisionArticle(inst);
			}

			bOnce = false;
		}

		static CNyaTimer gTimer;
		gTimer.Process();

		if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING) {
			for (int i = 0; i < 2700; i++) {
				auto pack = WCollisionAssets::mCollisionPackList[i];
				if (!pack) continue;

				if (!aCollisionArticles[i].aInstances.empty()) continue;

				for (int j = 0; j < pack->mInstanceNum; j++) {
					ProcessCollisionArticle(i, &pack->mInstanceList[j]);
				}

				for (auto& inst : aCollisionArticles[i].aInstances) {
					ConvertCollisionArticle(inst);
				}
			}

			for (int i = 0; i < 2700; i++) {
				auto pack = WCollisionAssets::mCollisionPackList[i];
				if (!pack) continue;

				for (auto& inst : aCollisionArticles[i].aInstances) {
					auto enabled = !inst.nSceneryGroupId || SceneryGroupEnabledTable[inst.nSceneryGroupId];
					if (enabled != inst.bB3MeshEnabled) {
						if (enabled) {
							b3Body_Enable(inst.nB3Body);
						}
						else {
							b3Body_Disable(inst.nB3Body);
						}
						inst.bB3MeshEnabled = enabled;
					}
				}
			}

			CollectWorldObjects();
		}
		else {
			for (auto& obj : aB3Objects) {
				b3DestroyBody(obj.nB3Body);
			}
			aB3Objects.clear();
		}

		if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND || !FEManager::mPauseRequest) {
			b3World_Step(m_worldId, gTimer.fDeltaTime, 4);
		}
	}

	ChloeHook Init([]{
		aMainLoopFunctions.push_back(OnWorldTick);

		b3WorldDef worldDef = b3DefaultWorldDef();
		worldDef.workerCount = 8;
		m_worldId = b3CreateWorld(&worldDef);
	});
}