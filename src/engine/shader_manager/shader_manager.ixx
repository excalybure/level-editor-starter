// Shader Manager - Automatic shader reloading and hot recompilation
// Watches shader files for changes and automatically recompiles and reloads them
export module engine.shader_manager;

import std;
import engine.renderer;
import runtime.console;

export namespace shader_manager
{

// Shader resource handle - identifies a specific shader
using ShaderHandle = size_t;
const ShaderHandle INVALID_SHADER_HANDLE = 0;

// Types of shaders we can manage
enum class ShaderType
{
	Vertex,
	Pixel,
	Compute,
	Geometry,
	Hull,
	Domain
};

// Information about a managed shader
struct ShaderInfo
{
	ShaderHandle handle;
	std::filesystem::path filePath;
	std::string entryPoint;
	std::string target;
	ShaderType type;
	std::filesystem::file_time_type lastModified;
	std::vector<std::filesystem::path> includedFiles;					// List of included files for dependency tracking
	std::vector<std::filesystem::file_time_type> includedFilesModTimes; // Modification times of included files
	renderer::ShaderBlob compiledBlob;
	bool isValid = false;
};

// Callback function type for shader reload notifications
// Called when a shader is successfully recompiled
using ShaderReloadCallback = std::function<void( ShaderHandle handle, const renderer::ShaderBlob &newBlob )>;

// Callback registration handle - used to unregister callbacks
using CallbackHandle = size_t;
const CallbackHandle INVALID_CALLBACK_HANDLE = 0;

// Shader Manager - manages automatic shader reloading
class ShaderManager
{
public:
	ShaderManager() = default;
	~ShaderManager() = default;

	// Register a shader for automatic reloading
	// Returns a handle to the shader that can be used to access it
	ShaderHandle registerShader(
		const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &target,
		ShaderType type );

	// Unregister a shader (stops watching for changes)
	void unregisterShader( ShaderHandle handle );

	// Register a callback for when shaders are reloaded
	// Returns a handle that can be used to unregister the callback
	CallbackHandle registerReloadCallback( ShaderReloadCallback callback );

	// Unregister a reload callback
	void unregisterReloadCallback( CallbackHandle callbackHandle );

	// Get the current compiled blob for a shader
	const renderer::ShaderBlob *getShaderBlob( ShaderHandle handle ) const;

	// Check for file changes and recompile if necessary
	// Should be called regularly (e.g., every frame)
	void update();

	// Force recompilation of a specific shader
	bool forceRecompile( ShaderHandle handle );

	// Force recompilation of all shaders
	void forceRecompileAll();

	// Get shader info for debugging/display purposes
	const ShaderInfo *getShaderInfo( ShaderHandle handle ) const;

	// Get all registered shaders
	std::vector<ShaderHandle> getAllShaderHandles() const;

private:
	// Internal data
	std::unordered_map<ShaderHandle, ShaderInfo> m_shaders;
	std::unordered_map<size_t, ShaderHandle> m_shaderHashMap; // Hash -> Handle mapping for fast lookup
	ShaderHandle m_nextHandle = 1;

	// Multiple callback support
	std::unordered_map<CallbackHandle, ShaderReloadCallback> m_reloadCallbacks;
	CallbackHandle m_nextCallbackHandle = 1;

	// Helper functions
	bool compileShader( ShaderInfo &shaderInfo );
	std::filesystem::file_time_type getFileModificationTime( const std::filesystem::path &path );
	std::string shaderTypeToString( ShaderType type ) const;
	ShaderHandle findExistingShader( const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &target,
		ShaderType type ) const;
	size_t computeShaderHash( const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &target,
		ShaderType type ) const;
};

} // namespace shader_manager
