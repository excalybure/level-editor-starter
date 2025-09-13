export module runtime.mesh_rendering_system;

import runtime.systems;
import runtime.ecs;
import runtime.components;
import engine.renderer;
import engine.camera;
import engine.gpu.mesh_gpu;
import engine.matrix;
import std;

export namespace runtime::systems
{

export class MeshRenderingSystem : public ::systems::System
{
public:
	MeshRenderingSystem( renderer::Renderer &renderer );

	void update( ecs::Scene &scene, float deltaTime ) override;
	void render( ecs::Scene &scene, const camera::Camera &camera );

	// Public for testing
	math::Mat4<> calculateMVPMatrix(
		const components::Transform &transform,
		const camera::Camera &camera );

	// Public for testing
	void renderEntity( const components::Transform &transform,
		const components::MeshRenderer &meshRenderer,
		const camera::Camera &camera );

private:
	renderer::Renderer &m_renderer;
};

} // namespace runtime::systems