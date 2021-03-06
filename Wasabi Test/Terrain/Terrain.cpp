#include "Terrain.hpp"

TerrainDemo::TerrainDemo(Wasabi* const app) : WTestState(app) {
}

void TerrainDemo::Load() {
	terrain = new WTerrain(m_app);
	terrain->Create();
}

void TerrainDemo::Update(float fDeltaTime) {
}

void TerrainDemo::Cleanup() {
	terrain->RemoveReference();
}