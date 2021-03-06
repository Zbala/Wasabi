#include "WParticles.h"
#include "../Cameras/WCamera.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Renderers/WRenderer.h"

class WParticlesGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({
		W_ATTRIBUTE_POSITION,
		W_VERTEX_ATTRIBUTE("particleSize", 3),
		W_VERTEX_ATTRIBUTE("particleAlpha", 1),
	});

public:
	WParticlesGeometry(Wasabi* const app, unsigned int ID = 0) : WGeometry(app, ID) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return m_desc.GetSize();
	}
};

class ParticlesVS : public WShader {
public:
	ParticlesVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "world"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "view"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "proj"), // projection
			}),
		};
	}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // size
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 1), // alpha
		}) };
		LoadCodeGLSL(
			#include "Shaders/particles_vs.glsl"
		);
	}
};

class ParticlesGS : public WShader {
public:
	ParticlesGS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "world"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "view"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "proj"), // projection
			}),
		};
	}

	virtual void Load() {
		m_desc.type = W_GEOMETRY_SHADER;
		m_desc.bound_resources = GetBoundResources();
		LoadCodeGLSL(
			#include "Shaders/particles_gs.glsl"
		);
	}
};

class ParticlesPS : public WShader {
public:
	ParticlesPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
			#include "Shaders/particles_ps.glsl"
		);
	}
};

void WParticlesBehavior::Emit(void* particle) {
	if (m_numParticles < m_maxParticles) {
		memcpy((char*)m_buffer + (m_numParticles*m_particleSize), particle, m_particleSize);
		m_numParticles++;
	}
}

WParticlesBehavior::WParticlesBehavior(unsigned int max_particles, unsigned int particle_size) {
	m_maxParticles = max_particles;
	m_particleSize = particle_size;
	m_numParticles = 0;
	m_buffer = (void*)new char[max_particles*particle_size];
}

WParticlesBehavior::~WParticlesBehavior() {
	delete[] m_buffer;
}

unsigned int WParticlesBehavior::UpdateAndCopyToVB(float cur_time, void* vb, unsigned int max_particles) {
	UpdateSystem(cur_time);

	unsigned int cur_offset_in_vb = 0;
	for (unsigned int i = 0; i < m_numParticles; i++) {
		void* cur_particle = (char*)m_buffer + (m_particleSize*i);
		if (!UpdateParticle(cur_time, cur_particle)) {
			memcpy(cur_particle, (char*)m_buffer + (m_particleSize*(m_numParticles-1)), m_particleSize);
			m_numParticles--;
			i--;
		} else {
			memcpy((char*)vb + cur_offset_in_vb, cur_particle, sizeof(WParticlesVertex));
			cur_offset_in_vb += sizeof(WParticlesVertex);
		}
	}

	return cur_offset_in_vb / sizeof(WParticlesVertex);
}

WDefaultParticleBehavior::WDefaultParticleBehavior(unsigned int max_particles)
	: WParticlesBehavior(max_particles, sizeof(WDefaultParticleBehavior::Particle)) {
	m_lastEmit = 0;
	m_emissionPosition = WVector3(0, 0, 0);
	m_emissionRandomness = WVector3(1, 1, 1);
	m_particleLife = 3;
	m_particleSpawnVelocity = WVector3(0, 2, 0);
	m_emissionFrequency = 20;
	m_emissionSize = 1;
	m_deathSize = 3;
	m_type = BILLBOARD;
}

void WDefaultParticleBehavior::UpdateSystem(float cur_time) {
	int num_emitted = 0;
	while (cur_time > m_lastEmit + 1.0f / m_emissionFrequency && num_emitted++ < 10) {
		m_lastEmit += 1.0f / m_emissionFrequency;
		Particle p = {};
		p.initialPos = m_emissionPosition + m_emissionRandomness * WVector3(WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f);
		p.velocity = m_particleSpawnVelocity;
		p.spawnTime = m_lastEmit;
		Emit((void*)&p);
	}
}

inline bool WDefaultParticleBehavior::UpdateParticle(float cur_time, void* particle) {
	Particle* p = (Particle*)particle;
	float lifePercentage = (cur_time - p->spawnTime) / m_particleLife;
	float size = WUtil::flerp(m_emissionSize, m_deathSize, lifePercentage);

	p->vtx.pos = p->initialPos + lifePercentage * m_particleLife * p->velocity;
	p->vtx.size = m_type == BILLBOARD ? WVector3(0, size, 0) : WVector3(size, 0, 0);
	p->vtx.alpha = fmin(fmax(0.8f - fabs(lifePercentage * 2.0f - 1.0f), 0.0f), 1.0f);
	return lifePercentage <= 1.0f;
}

WParticlesManager::WParticlesManager(class Wasabi* const app)
	: WManager<WParticles>(app) {
	m_vertexShader = nullptr;
	m_geometryShader = nullptr;
	m_fragmentShader = nullptr;
}

WParticlesManager::~WParticlesManager() {
	for (auto it = m_particleEffects.begin(); it != m_particleEffects.end(); it++)
		W_SAFE_REMOVEREF(it->second);
	W_SAFE_REMOVEREF(m_vertexShader);
	W_SAFE_REMOVEREF(m_geometryShader);
	W_SAFE_REMOVEREF(m_fragmentShader);
}

void WParticlesManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

std::string WParticlesManager::GetTypeName() const {
	return "Particles";
}

WError WParticlesManager::Load() {
	unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, VkPipelineColorBlendAttachmentState> blend_states;
	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};

	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.depthTestEnable = VK_TRUE;
	depth_stencil_state.depthWriteEnable = VK_FALSE;
	depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil_state.stencilTestEnable = VK_FALSE;
	depth_stencil_state.front = depth_stencil_state.back;

	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.cullMode = VK_CULL_MODE_NONE;
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blend_state = {};
	blend_state.colorWriteMask = 0xff;
	blend_state.blendEnable = VK_TRUE;
	blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.colorBlendOp = VK_BLEND_OP_ADD;
	blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
	blend_states.insert(std::pair<W_DEFAULT_PARTICLE_EFFECT_TYPE, VkPipelineColorBlendAttachmentState>(
		W_DEFAULT_PARTICLES_ADDITIVE, blend_state
	));
	
	blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.colorBlendOp = VK_BLEND_OP_ADD;
	blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
	blend_states.insert(std::pair<W_DEFAULT_PARTICLE_EFFECT_TYPE, VkPipelineColorBlendAttachmentState>(
		W_DEFAULT_PARTICLES_ALPHA, blend_state
	));
	
	blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
	blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
	blend_states.insert(std::pair<W_DEFAULT_PARTICLE_EFFECT_TYPE, VkPipelineColorBlendAttachmentState>(
		W_DEFAULT_PARTICLES_SUBTRACTIVE, blend_state
	));

	m_vertexShader = new ParticlesVS(m_app);
	m_vertexShader->Load();
	m_geometryShader = new ParticlesGS(m_app);
	m_geometryShader->Load();
	m_fragmentShader = new ParticlesPS(m_app);
	m_fragmentShader->Load();

	for (auto it = blend_states.begin(); it != blend_states.end(); it++) {
		WEffect* fx = new WEffect(m_app);
		m_particleEffects.insert(std::pair<W_DEFAULT_PARTICLE_EFFECT_TYPE, WEffect*>(it->first, fx));

		WError werr = fx->BindShader(m_vertexShader);
		if (!werr)
			return werr;

		werr = fx->BindShader(m_geometryShader);
		if (!werr)
			return werr;

		werr = fx->BindShader(m_fragmentShader);
		if (!werr)
			return werr;

		fx->SetBlendingState(it->second);
		fx->SetDepthStencilState(depth_stencil_state);
		fx->SetRasterizationState(rasterization_state);
		fx->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

		werr = fx->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
		if (!werr)
			return werr;
	}

	return WError(W_SUCCEEDED);
}

class WEffect* WParticlesManager::GetDefaultEffect(W_DEFAULT_PARTICLE_EFFECT_TYPE type) const {
	return m_particleEffects.find(type)->second;
}


WParticles::WParticles(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();

	m_behavior = nullptr;
	m_geometry = nullptr;
	m_material = nullptr;

	app->ParticlesManager->AddEntity(this);
}

WParticles::~WParticles() {
	_DestroyResources();

	m_app->ParticlesManager->RemoveEntity(this);
}

std::string WParticles::GetTypeName() const {
	return "Particles";
}

void WParticles::_DestroyResources() {
	W_SAFE_DELETE(m_behavior);
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);
}

WParticlesBehavior* WParticles::GetBehavior() const {
	return m_behavior;
}

bool WParticles::Valid() const {
	return m_behavior && m_geometry && m_material;
}

void WParticles::Show() {
	m_hidden = false;
}

void WParticles::Hide() {
	m_hidden = true;
}

bool WParticles::Hidden() const {
	return m_hidden;
}

void WParticles::EnableFrustumCulling() {
	m_bFrustumCull = true;
}

void WParticles::DisableFrustumCulling() {
	m_bFrustumCull = false;
}

bool WParticles::InCameraView(class WCamera* cam) {
	WMatrix worldM = GetWorldMatrix();
	WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM) + WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM) - WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 pos = (max + min) / 2.0f;
	WVector3 size = (max - min) / 2.0f;
	return cam->CheckBoxInFrustum(pos, size);
}

WError WParticles::Create(class WEffect* effect, unsigned int max_particles, WParticlesBehavior* behavior) {
	_DestroyResources();
	if (max_particles == 0)
		return WError(W_INVALIDPARAM);

	m_geometry = new WParticlesGeometry(m_app);
	m_material = new WMaterial(m_app);

	m_material->SetEffect(effect);

	WParticlesVertex* vertices = new WParticlesVertex[max_particles];
	WError err = m_geometry->CreateFromData(vertices, max_particles, nullptr, 0, true);
	delete[] vertices;

	if (err != W_SUCCEEDED) {
		_DestroyResources();
		return err;
	}

	m_behavior = behavior ? behavior : new WDefaultParticleBehavior(max_particles);
	m_maxParticles = max_particles;

	return WError(W_SUCCEEDED);
}

void WParticles::Render(class WRenderTarget* const rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return;

			m_material->SetVariableMatrix("world", GetWorldMatrix());
			m_material->SetVariableMatrix("view", cam->GetViewMatrix());
			m_material->SetVariableMatrix("proj", cam->GetProjectionMatrix());

			// update the geometry
			WParticlesVertex* vb;
			m_geometry->MapVertexBuffer((void**)&vb);
			float cur_time = m_app->Timer.GetElapsedTime();
			unsigned int num_particles = m_behavior->UpdateAndCopyToVB(cur_time, vb, m_maxParticles);
			m_geometry->UnmapVertexBuffer();

			m_material->Bind(rt);
			m_geometry->Draw(rt, num_particles, 1, false);
		}
	}
}

WError WParticles::SetTexture(class WImage* texture) {
	return m_material->SetTexture(1, texture);
}

WMatrix WParticles::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WParticles::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and invert it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos); //apply offset
		float y = -WVec3Dot(_up, _pos); //apply offset
		float z = -WVec3Dot(_look, _pos); //apply offset
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		return true;
	}

	return false;
}

void WParticles::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

