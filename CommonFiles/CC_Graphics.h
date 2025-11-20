#pragma once
#include "CC_Core.h"
#include "CC_Window.h"
#include "CC_Exception.h"
#include "CC_GraphicsUtils.h"

namespace Cc
{
#if defined PLAT_WIN32 && defined GAPI_DX

	class CCAPI GraphicsException : public Exception
	{
	public:
		GraphicsException(HRESULT code, std::source_location loc = std::source_location::current());
		const char* what() const noexcept override;

	private:
		HRESULT m_Code;
	};

	class CCAPI Graphics
	{
	public:
		Graphics(Window* p_Window);
		~Graphics();

		void DrawFrame();
		void SetRasterizerMode(const GfxUtils::RasterizerMode& mode);
		uint32_t CompileShader(const std::string& vertexPath, const std::string& pixelPath);
		uint32_t LoadTexture(const std::string& texturePath);
		uint32_t LoadModel(const std::string& modelPath);

	public:
		uint32_t FindTextureByPath(const std::string& texturePath);

	private:
		void CreateFactory();
		IDXGIAdapter* FindSuitalbeAdapter();
		void CreateDevice();
		void CreateSwapchain(Window* p_Window);
		void CreateDepthBuffer(Window* p_Window);
		void CreateDepthState();
		void CreateDepthView();
		void CreateRenderTarget();
		void CreateViewport(Window* p_Window);

	private:
		void ProcessNode(aiNode* p_Node, const aiScene* p_Scene, std::vector<GfxUtils::Mesh>& v_meshes);
		GfxUtils::Mesh ProcessMesh(aiMesh* p_Mesh, const aiScene* p_Scene);
		GfxUtils::Material ProcessMaterial(aiMaterial* p_Material);

	private:
		uint32_t GenerateUniqueShaderId();
		uint32_t GenerateUniqueTextureId();
		uint32_t GenerateUniqueModelId();

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory> mp_Factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter> mp_Adapter;
		Microsoft::WRL::ComPtr<IDXGISwapChain> mp_SwapChain;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> mp_Context;
		Microsoft::WRL::ComPtr<ID3D11Device> mp_Device;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mp_RasterizerSolid;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mp_RasterizerWire;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> mp_BackBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> mp_DepthBuffer;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mp_DepthState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mp_DepthView;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> mp_Sampler;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mp_RenderTarget;

	private:
		std::vector<GfxUtils::Shader> mv_Shaders;
		std::vector<GfxUtils::Texture> mv_Textures;
	};

	namespace MultiThread
	{
		class CCAPI GraphicsMT
		{
			friend class Cc::Graphics;
		private:
			static void CreateRasterizerState(ID3D11Device* p_Device, ID3D11RasterizerState** pp_Rasterizer, GfxUtils::RasterizerMode mode);
			static void CreateSamplerState(ID3D11Device* p_Device, ID3D11SamplerState** pp_Sampler);
			static void CompileVertexShader(ID3D11Device* p_Device, ID3D11VertexShader** pp_Shader, ID3D11InputLayout** pp_Layout, std::wstring filePath);
			static void CompilePixelShader(ID3D11Device* p_Device, ID3D11PixelShader** pp_Shader, std::wstring filePath);
			static void LoadTexture(ID3D11Device* p_Device, ID3D11Texture2D** pp_RawData, ID3D11ShaderResourceView** pp_Srv, std::string filePath);
			static void CreateBuffer(ID3D11Device* p_Device, ID3D11Buffer** pp_Buffer, size_t bufSize, void* p_Data, GfxUtils::BufferType type);
		};
	}

#endif
}