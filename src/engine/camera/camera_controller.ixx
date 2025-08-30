// Camera Controller module for handling input and camera navigation
// Provides high-level camera control with mouse/keyboard input abstraction
export module engine.camera.controller;

import std;
import engine.vec;
import engine.camera;

export namespace camera
{

// Input state for camera controllers
struct InputState
{
	// Mouse state
	struct Mouse
	{
		float x = 0.0f, y = 0.0f;			// Current mouse position
		float deltaX = 0.0f, deltaY = 0.0f; // Mouse delta since last frame
		bool leftButton = false;			// Left mouse button pressed
		bool rightButton = false;			// Right mouse button pressed
		bool middleButton = false;			// Middle mouse button pressed
		float wheelDelta = 0.0f;			// Mouse wheel delta
	} mouse;

	// Keyboard state
	struct Keyboard
	{
		bool shift = false; // Shift key pressed
		bool ctrl = false;	// Control key pressed
		bool alt = false;	// Alt key pressed

		// Navigation keys
		bool w = false, a = false, s = false, d = false;
		bool q = false, e = false; // Up/down
		bool f = false;			   // Focus/frame key
		bool space = false;		   // Space key
	} keyboard;

	// Frame timing
	float deltaTime = 0.016f; // Time since last frame (60 FPS default)
};

// Base camera controller interface
export class CameraController
{
public:
	explicit CameraController( Camera *camera );
	virtual ~CameraController() = default;

	// No copy/move for now
	CameraController( const CameraController & ) = delete;
	CameraController &operator=( const CameraController & ) = delete;

	// Update camera based on input
	virtual void update( const InputState &input ) = 0;

	// Camera management
	void setCamera( Camera *camera ) noexcept { m_camera = camera; }
	Camera *getCamera() const noexcept { return m_camera; }

	// Controller settings
	void setEnabled( bool enabled ) noexcept { m_enabled = enabled; }
	bool isEnabled() const noexcept { return m_enabled; }

protected:
	Camera *m_camera = nullptr;
	bool m_enabled = true;

	// Helper for smooth camera movements
	float m_smoothingSpeed = 8.0f;
};

// Controller for perspective cameras with orbit/pan/zoom
export class PerspectiveCameraController : public CameraController
{
public:
	explicit PerspectiveCameraController( PerspectiveCamera *camera );
	~PerspectiveCameraController() override = default;

	// Update camera with perspective-specific controls
	void update( const InputState &input ) override;

	// Control sensitivity settings
	void setOrbitSensitivity( float sensitivity ) noexcept { m_orbitSensitivity = sensitivity; }
	void setPanSensitivity( float sensitivity ) noexcept { m_panSensitivity = sensitivity; }
	void setZoomSensitivity( float sensitivity ) noexcept { m_zoomSensitivity = sensitivity; }

	float getOrbitSensitivity() const noexcept { return m_orbitSensitivity; }
	float getPanSensitivity() const noexcept { return m_panSensitivity; }
	float getZoomSensitivity() const noexcept { return m_zoomSensitivity; }

	// Keyboard movement settings
	void setKeyboardMoveSpeed( float speed ) noexcept { m_keyboardMoveSpeed = speed; }
	float getKeyboardMoveSpeed() const noexcept { return m_keyboardMoveSpeed; }

	// Focus/framing functionality
	void focusOnPoint( const math::Vec3<> &point, float distance = 10.0f ) noexcept;
	void focusOnBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

	// Auto-rotation (for demos/previews)
	void setAutoRotate( bool enabled ) noexcept { m_autoRotate = enabled; }
	void setAutoRotateSpeed( float speed ) noexcept { m_autoRotateSpeed = speed; }
	bool getAutoRotate() const noexcept { return m_autoRotate; }

private:
	PerspectiveCamera *m_perspectiveCamera = nullptr;

	// Control sensitivity
	float m_orbitSensitivity = 0.5f;
	float m_panSensitivity = 1.0f;
	float m_zoomSensitivity = 1.0f;
	float m_keyboardMoveSpeed = 10.0f;

	// Auto-rotation for demos
	bool m_autoRotate = false;
	float m_autoRotateSpeed = 30.0f; // degrees per second

	// Input state tracking
	bool m_isDragging = false;
	bool m_isPanning = false;
	math::Vec2<> m_lastMousePos{ 0.0f, 0.0f };

	// Smooth focusing
	struct FocusState
	{
		bool isFocusing = false;
		math::Vec3<> targetPosition{ 0.0f, 0.0f, 0.0f };
		math::Vec3<> targetLookAt{ 0.0f, 0.0f, 0.0f };
		math::Vec3<> startPosition{ 0.0f, 0.0f, 0.0f };
		math::Vec3<> startLookAt{ 0.0f, 0.0f, 0.0f };
		float focusTime = 0.0f;
		float focusDuration = 1.0f;
	} m_focusState;

	// Handle different input modes
	void handleOrbitInput( const InputState &input );
	void handlePanInput( const InputState &input );
	void handleZoomInput( const InputState &input );
	void handleKeyboardInput( const InputState &input );
	void updateFocusing( float deltaTime );
	void updateAutoRotation( float deltaTime );
};

// Controller for orthographic cameras with 2D navigation
export class OrthographicCameraController : public CameraController
{
public:
	explicit OrthographicCameraController( OrthographicCamera *camera );
	~OrthographicCameraController() override = default;

	// Update camera with orthographic-specific controls
	void update( const InputState &input ) override;


	// Control sensitivity settings
	void setPanSensitivity( float sensitivity ) noexcept { m_panSensitivity = sensitivity; }
	void setZoomSensitivity( float sensitivity ) noexcept { m_zoomSensitivity = sensitivity; }

	float getPanSensitivity() const noexcept { return m_panSensitivity; }
	float getZoomSensitivity() const noexcept { return m_zoomSensitivity; }

	// Frame content to fit bounds
	void frameBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

	// Zoom limits for orthographic view
	void setZoomLimits( float minSize, float maxSize ) noexcept;
	float getMinZoom() const noexcept { return m_minOrthographicSize; }
	float getMaxZoom() const noexcept { return m_maxOrthographicSize; }

private:
	OrthographicCamera *m_orthographicCamera = nullptr;

	// Control sensitivity
	float m_panSensitivity = 1.0f;
	float m_zoomSensitivity = 1.0f;

	// Zoom limits
	float m_minOrthographicSize = 0.1f;
	float m_maxOrthographicSize = 1000.0f;

	// Input state tracking
	bool m_isDragging = false;
	math::Vec2<> m_lastMousePos{ 0.0f, 0.0f };

	// Handle different input modes
	void handlePanInput( const InputState &input );
	void handleZoomInput( const InputState &input );
};

// Factory functions for creating controllers
export namespace ControllerFactory
{
// Create appropriate controller for camera type
std::unique_ptr<CameraController> createController( Camera *camera );

// Create specific controller types
std::unique_ptr<PerspectiveCameraController> createPerspectiveController( PerspectiveCamera *camera );
std::unique_ptr<OrthographicCameraController> createOrthographicController( OrthographicCamera *camera );
} // namespace ControllerFactory

// Utility functions for input processing
export namespace InputUtils
{
// Convert screen coordinates to normalized device coordinates
math::Vec2<> ScreenToNDC( const math::Vec2<> &screenPos, const math::Vec2<> &screenSize ) noexcept;

// Calculate mouse sensitivity based on camera distance
float CalculateDistanceBasedSensitivity( float baseSpeed, float distance, float minSpeed = 0.1f ) noexcept;

// Smooth input filtering for smoother camera movement
float SmoothInput( float current, float target, float smoothing, float deltaTime ) noexcept;
math::Vec2<> SmoothInput( const math::Vec2<> &current, const math::Vec2<> &target, float smoothing, float deltaTime ) noexcept;

// Input deadzone processing
float ApplyDeadzone( float input, float deadzone ) noexcept;
math::Vec2<> ApplyDeadzone( const math::Vec2<> &input, float deadzone ) noexcept;
} // namespace InputUtils

} // namespace camera
