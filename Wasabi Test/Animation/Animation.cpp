#include "Animation.hpp"

AnimationDemo::AnimationDemo(Wasabi* const app) : WTestState(app) {
}

void AnimationDemo::Load() {
	WGeometry* geometry = new WGeometry(m_app);
	geometry->LoadFromHXM("Media/dante.HXM");

	WImage* texture = new WImage(m_app);
	texture->Load("Media/dante.bmp");

	character = new WObject(m_app);
	character->SetGeometry(geometry);
	((WFRMaterial*)character->GetMaterial())->Texture(texture);

	// don't need these anymore, character has the reference to them
	geometry->RemoveReference();
	texture->RemoveReference();

	WSkeleton* animation = new WSkeleton(m_app);
	animation->LoadFromWA("Media/dante.HXS");

	character->SetAnimation(animation);
	animation->SetPlaySpeed(20.0f);
	animation->Loop();

	// don't need this anymore
	animation->RemoveReference();
}

void AnimationDemo::Update(float fDeltaTime) {
}

void AnimationDemo::Cleanup() {
	character->RemoveReference();
}