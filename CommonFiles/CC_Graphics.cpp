#include "CC_Graphics.h"
#include "CC_Convert.h"
#include "CC_FileUtils.h"

namespace Cc
{
	GraphicsException::GraphicsException(HRESULT code, std::source_location loc)
		: m_Code(code), Exception(loc)
	{}

	const char* GraphicsException::what() const noexcept
	{
		std::ostringstream oss;
		oss << "Exception caught!\n"
			<< "[LINE] " << m_Line << "\n"
			<< "[FUNC] " << m_Func << "\n"
			<< "[FILE] " << m_File << "\n"
			<< "[CODE] 0x" << std::hex << m_Code << std::dec << "\n";

		m_WhatBuffer = oss.str();

		return m_WhatBuffer.c_str();
	}

	Graphics::Graphics(Window* p_Window)
	{
		LOG_F(INFO, "Initializing DX11 rendering pipeline...");

		CreateFactory();

		mp_Adapter = FindSuitalbeAdapter();

		if (mp_Adapter == nullptr) throw Exception();

		CreateDevice();

		CreateSwapchain(p_Window);

		std::thread wire_thread(MultiThread::GraphicsMT::CreateRasterizerState, mp_Device.Get(), mp_RasterizerWire.GetAddressOf(), GfxUtils::RasterizerMode::RasterizerMode_WireFrame);
		std::thread solid_thread(MultiThread::GraphicsMT::CreateRasterizerState, mp_Device.Get(), mp_RasterizerSolid.GetAddressOf(), GfxUtils::RasterizerMode::RasterizerMode_Solid);
		std::thread sampler_thread(MultiThread::GraphicsMT::CreateSamplerState, mp_Device.Get(), mp_Sampler.GetAddressOf());

		CreateDepthBuffer(p_Window);
		CreateDepthState();
		CreateDepthView();
		CreateRenderTarget();
		CreateViewport(p_Window);

		wire_thread.join();
		solid_thread.join();
		sampler_thread.join();

		SetRasterizerMode(GfxUtils::RasterizerMode::RasterizerMode_Solid);

		LOG_F(INFO, "Pipeline initialization finished");
	}

	Graphics::~Graphics()
	{
	}

	void Graphics::DrawFrame()
	{
		float color[4] = { 0.0f, 0.2f, 0.6f, 1.0f };

		mp_Context->ClearDepthStencilView(mp_DepthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		mp_Context->ClearRenderTargetView(mp_RenderTarget.Get(), color);

		mp_SwapChain->Present(0,0);
	}

	uint32_t Graphics::CompileShader(const std::string& vertexPath, const std::string& pixelPath)
	{
		std::string pv = g_ShaderPath + StripPathToFileName(vertexPath);
		std::string pp = g_ShaderPath + StripPathToFileName(pixelPath);

		std::wstring wVertexPath = Cc::ConvertStringToWideString(pv);
		std::wstring wPixelPath = Cc::ConvertStringToWideString(pp);

		GfxUtils::Shader shader;

		std::thread vertexThread(MultiThread::GraphicsMT::CompileVertexShader, mp_Device.Get(), shader.mp_Vertex.GetAddressOf(), shader.mp_Layout.GetAddressOf(), wVertexPath);
		std::thread pixelThread(MultiThread::GraphicsMT::CompilePixelShader, mp_Device.Get(), shader.mp_Pixel.GetAddressOf(), wPixelPath);

		vertexThread.join();
		pixelThread.join();

		if (shader.mp_Vertex.Get() == nullptr || shader.mp_Layout.Get() == nullptr || shader.mp_Pixel.Get() == nullptr)
		{
			LOG_F(ERROR, "Failed to compile shaders!");
			return 0;
		}

		shader.m_PixelPath = pp;
		shader.m_VertexPath = pv;
		shader.m_ShaderId = GenerateUniqueShaderId();

		mv_Shaders.push_back(shader);

		return shader.GetShaderId();
	}

	uint32_t Graphics::LoadTexture(const std::string& texturePath)
	{
		std::string path = g_TexturePath + StripPathToFileName(texturePath);

		GfxUtils::Texture result;
		std::vector<unsigned char> buffer;
		unsigned int width, height;

		LOG_F(INFO, "Loading %s", path.c_str());

		int ret = lodepng::decode(buffer, width, height, path);
		if (ret)
		{
			LOG_F(ERROR, "Failed to load %s, error code %u", path.c_str(), ret);
			return 0;
		}

		LOG_F(INFO, "Texture decoded");

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.ArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = buffer.data();
		data.SysMemPitch = (width * 4);
		data.SysMemSlicePitch = (width * height * 4);

		HRESULT hr = mp_Device->CreateTexture2D(&texDesc, &data, result.mp_RawData.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_F(ERROR, "CreateTexture2D failed for %s, error code %u", path.c_str(), hr);
			return 0;
		}

		LOG_F(INFO, "Data buffer created");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		hr = mp_Device->CreateShaderResourceView(result.mp_RawData.Get(), &srvDesc, result.mp_ShaderResource.GetAddressOf());
		if (FAILED(hr))
		{
			LOG_F(ERROR, "CreateShaderResourceView failed for %s, error code %u", path.c_str(), hr);
			if (result.mp_RawData) result.mp_RawData.Reset();
			return 0;
		}

		LOG_F(INFO, "Shader resource view created");

		result.m_TextureId = GenerateUniqueTextureId();
		result.m_TexturePath = path;

		mv_Textures.push_back(result);
		LOG_F(INFO, "%s loaded", path.c_str());

		return result.GetTextureId();
	}

	uint32_t Graphics::LoadModel(const std::string& modelPath)
	{
		Assimp::Importer imp;

		GfxUtils::Model model;

		std::string path = g_ModelPath + StripPathToFileName(modelPath);

		LOG_F(INFO, "Loading %s", path.c_str());

		const aiScene* pScene = imp.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
		if (!pScene)
		{
			LOG_F(ERROR, "Failed to load %s", path.c_str());
			return 0;
		}

		ProcessNode(pScene->mRootNode, pScene, model.mv_Meshes);

		model.m_ModelId = GenerateUniqueModelId();
		model.m_ModelPath = path;
		mv_Models.push_back(model);

		LOG_F(INFO, "%s loaded", path.c_str());

		return model.GetModelId();
	}

	uint32_t Graphics::FindTextureByPath(const std::string& texturePath)
	{
		for (const auto& texture : mv_Textures)
		{
			if (strcmp(texturePath.c_str(), texture.GetTexturePath().c_str()) == 0)
				return texture.GetTextureId();
		}

		return 0;
	}

	void Graphics::SetRasterizerMode(const GfxUtils::RasterizerMode& mode)
	{
		switch (mode)
		{
		case GfxUtils::RasterizerMode::RasterizerMode_Solid:
			LOG_F(INFO, "Rasterizer mode set to solid");
			mp_Context->RSSetState(mp_RasterizerSolid.Get());
			break;
		case GfxUtils::RasterizerMode::RasterizerMode_WireFrame:
			LOG_F(INFO, "Rasterizer mode set to wireframe");
			mp_Context->RSSetState(mp_RasterizerWire.Get());
			break;
		}
	}

	void Graphics::CreateFactory()
	{
		LOG_F(INFO, "Creating DXGI factory...");

		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (LPVOID*)mp_Factory.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "DXGI factory created");
	}

	IDXGIAdapter* Graphics::FindSuitalbeAdapter()
	{
		LOG_F(INFO, "Looking for suitable adapter...");

		IDXGIAdapter* p_TempAdapter;
		UINT i = 0;

		while (mp_Factory->EnumAdapters(i, &p_TempAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc = {};
			p_TempAdapter->GetDesc(&desc);

			LOG_F(INFO, "Validating adapter: %ls", desc.Description);

			if (desc.DedicatedVideoMemory > 0)
			{
				LOG_F(INFO, "Adapter validated! %ls is suitable", desc.Description);
				return p_TempAdapter;
			}

			LOG_F(WARNING, "Adapter validated! %ls is unsuitable", desc.Description);
			p_TempAdapter->Release();
			i++;
		}

		return nullptr;
	}

	void Graphics::CreateDevice()
	{
		LOG_F(INFO, "Creating device...");

		UINT devFlags = 0;
#if defined _DEBUG || defined DEBUG
		LOG_F(INFO, "Enablig D3D11_CREATE_DEVICE_DEBUG flag for device");
		devFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL fLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};

		HRESULT hr = D3D11CreateDevice(mp_Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0, devFlags, fLevels, _countof(fLevels), D3D11_SDK_VERSION, mp_Device.GetAddressOf(), nullptr, mp_Context.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Device and context created");
	}
	
	void Graphics::CreateSwapchain(Window* p_Window)
	{
		LOG_F(INFO, "Creating swapchain...");

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = 1;
		desc.BufferDesc.Width = p_Window->GetWidth();
		desc.BufferDesc.Height = p_Window->GetHeight();
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Windowed = !p_Window->IsFullscreen();
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.OutputWindow = p_Window->GetWindowHandle();

		HRESULT hr = mp_Factory->CreateSwapChain(mp_Device.Get(), &desc, mp_SwapChain.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Swapchain created");
	}

	void Graphics::CreateDepthBuffer(Window* p_Window)
	{
		LOG_F(INFO, "Creating depth/stencil buffer...");

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = p_Window->GetWidth();
		desc.Height = p_Window->GetHeight();
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = 0;

		HRESULT hr = mp_Device->CreateTexture2D(&desc, nullptr, mp_DepthBuffer.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Depth/stencil buffer created");
	}

	void Graphics::CreateDepthState()
	{
		LOG_F(INFO, "Creating depth/stencil state...");

		D3D11_DEPTH_STENCIL_DESC desc = {};
		// Depth test parameters
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;

		// Stencil test parameters
		desc.StencilEnable = true;
		desc.StencilReadMask = 0xFF;
		desc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing
		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing
		desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		HRESULT hr = mp_Device->CreateDepthStencilState(&desc, mp_DepthState.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Depth/stencil state created");

		mp_Context->OMSetDepthStencilState(mp_DepthState.Get(), 0);
		LOG_F(INFO, "Depth/stencil state set");
	}

	void Graphics::CreateDepthView()
	{
		LOG_F(INFO, "Creating depth/stencil view... ");

		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Flags = 0;

		HRESULT hr = mp_Device->CreateDepthStencilView(mp_DepthBuffer.Get(), &desc, mp_DepthView.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Depth/stencil view created");
	}

	void Graphics::CreateRenderTarget()
	{
		LOG_F(INFO, "Creating render target view... ");
		LOG_F(INFO, "Obtaining back buffer... ");

		HRESULT hr = mp_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)mp_BackBuffer.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);
		LOG_F(INFO, "Back buffer obtained");

		hr = mp_Device->CreateRenderTargetView(mp_BackBuffer.Get(), nullptr, mp_RenderTarget.GetAddressOf());
		if (FAILED(hr)) throw GraphicsException(hr);

		LOG_F(INFO, "Render target view created");
		mp_Context->OMSetRenderTargets(1, mp_RenderTarget.GetAddressOf(), mp_DepthView.Get());
		LOG_F(INFO, "Render target view set");
		LOG_F(INFO, "Depth/stencil view set");
	}

	void Graphics::CreateViewport(Window* p_Window)
	{
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (FLOAT)p_Window->GetWidth();
		vp.Height = (FLOAT)p_Window->GetHeight();
		vp.MaxDepth = 1.0f;
		vp.MinDepth = 0.0f;

		mp_Context->RSSetViewports(1, &vp);
		LOG_F(INFO, "Viewport set");
	}

	void Graphics::ProcessNode(aiNode* p_Node, const aiScene* p_Scene, std::vector<GfxUtils::Mesh>& v_meshes)
	{
		for (size_t i = 0; i < p_Node->mNumMeshes; i++)
		{
			v_meshes.push_back(ProcessMesh(p_Scene->mMeshes[p_Node->mMeshes[i]], p_Scene));
		}

		for (size_t i = 0; i < p_Node->mNumChildren; i++)
		{
			ProcessNode(p_Node->mChildren[i], p_Scene, v_meshes);
		}
	}

	GfxUtils::Mesh Graphics::ProcessMesh(aiMesh* p_Mesh, const aiScene* p_Scene)
	{
		GfxUtils::Mesh result;

		std::vector<GfxUtils::VERTEX> v_vertices;
		std::vector<uint32_t> v_indices;

		for (size_t i = 0; i < p_Mesh->mNumVertices; i++)
		{
			GfxUtils::VERTEX v;
			v.m_Pos.x = p_Mesh->mVertices[i].x;
			v.m_Pos.y = p_Mesh->mVertices[i].y;
			v.m_Pos.z = p_Mesh->mVertices[i].z;

			if (p_Mesh->HasNormals())
			{
				v.m_Normal.x = p_Mesh->mNormals[i].x;
				v.m_Normal.y = p_Mesh->mNormals[i].y;
				v.m_Normal.z = p_Mesh->mNormals[i].z;
			}
			else
			{
				v.m_Normal.x = v.m_Normal.y = v.m_Normal.z = 0.0f;
			}

			if (p_Mesh->HasTextureCoords(0))
			{
				v.m_TexCoord.x = p_Mesh->mTextureCoords[0][i].x;
				v.m_TexCoord.y = p_Mesh->mTextureCoords[0][i].y;
			}
			else
			{
				v.m_TexCoord.x = v.m_TexCoord.y = 0.0f;
			}

			v_vertices.push_back(v);
		}

		for (size_t i = 0; i < p_Mesh->mNumFaces; i++)
		{
			aiFace face = p_Mesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; j++)
			{
				v_indices.push_back(face.mIndices[j]);
			}
		}

		std::thread vertex_thread(MultiThread::GraphicsMT::CreateBuffer, mp_Device.Get(), result.mp_VertexBuffer.GetAddressOf(), (sizeof(GfxUtils::VERTEX) * v_vertices.size()), v_vertices.data(), GfxUtils::BufferType::BufferType_Vertex);
		std::thread index_thread(MultiThread::GraphicsMT::CreateBuffer, mp_Device.Get(), result.mp_IndexBuffer.GetAddressOf(), (sizeof(uint32_t) * v_indices.size()), v_indices.data(), GfxUtils::BufferType::BufferType_Index);

		vertex_thread.join();
		index_thread.join();

		if (result.mp_IndexBuffer.Get() == nullptr || result.mp_VertexBuffer.Get() == nullptr)
			LOG_F(ERROR, "Failed to create one or more buffers");

		if (p_Mesh->mMaterialIndex >= 0)
		{
			LOG_F(INFO, "Processing mesh materials... ");
			result.m_Material = ProcessMaterial(p_Scene->mMaterials[p_Mesh->mMaterialIndex]);
		}

		return result;
	}

	GfxUtils::Material Graphics::ProcessMaterial(aiMaterial* p_Material)
	{
		LOG_F(INFO, "Processing material %s", p_Material->GetName().C_Str());

		GfxUtils::Material result;
		GfxUtils::Texture diffuse_texture, specular_texture, normal_texture;
		std::optional<std::thread> diffuse_thread, specular_thread, normal_thread;

		if (p_Material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString str;
			p_Material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			if (!str.Empty())
			{
				std::string texPath = str.C_Str();
				uint32_t texId = FindTextureByPath(texPath);
				diffuse_texture.m_TexturePath = texPath;
				if(texId != 0)
					result.m_DiffuseTextureId = texId;
				else
					diffuse_thread = std::thread(MultiThread::GraphicsMT::LoadTexture, mp_Device.Get(), diffuse_texture.mp_RawData.GetAddressOf(), diffuse_texture.mp_ShaderResource.GetAddressOf(), texPath);
			}
		}

		if (p_Material->GetTextureCount(aiTextureType_SPECULAR) > 0)
		{
			aiString str;
			p_Material->GetTexture(aiTextureType_SPECULAR, 0, &str);
			if (!str.Empty())
			{
				std::string texPath = str.C_Str();
				uint32_t texId = FindTextureByPath(texPath);
				specular_texture.m_TexturePath = texPath;
				if (texId != 0)
					result.m_SpecularTextureId = texId;
				else
					specular_thread = std::thread(MultiThread::GraphicsMT::LoadTexture, mp_Device.Get(), specular_texture.mp_RawData.GetAddressOf(), specular_texture.mp_ShaderResource.GetAddressOf(), texPath);
			}
		}

		if (p_Material->GetTextureCount(aiTextureType_NORMALS) > 0)
		{
			aiString str;
			p_Material->GetTexture(aiTextureType_NORMALS, 0, &str);
			if (!str.Empty())
			{
				std::string texPath = str.C_Str();
				uint32_t texId = FindTextureByPath(texPath);
				normal_texture.m_TexturePath = texPath;
				if (texId != 0)
					result.m_NormalTextureId = texId;
				else
					normal_thread = std::thread(MultiThread::GraphicsMT::LoadTexture, mp_Device.Get(), normal_texture.mp_RawData.GetAddressOf(), normal_texture.mp_ShaderResource.GetAddressOf(), texPath);
			}
		}

		if (diffuse_thread.has_value()) diffuse_thread.value().join();
		if (specular_thread.has_value()) specular_thread.value().join();
		if (normal_thread.has_value()) normal_thread.value().join();

		if (diffuse_texture.mp_RawData.Get() != nullptr && diffuse_texture.mp_ShaderResource.Get() != nullptr)
		{
			diffuse_texture.m_TextureId = GenerateUniqueTextureId();
			mv_Textures.push_back(diffuse_texture);
			result.m_DiffuseTextureId = diffuse_texture.GetTextureId();
		}

		if (specular_texture.mp_RawData.Get() != nullptr && specular_texture.mp_ShaderResource.Get() != nullptr)
		{
			specular_texture.m_TextureId = GenerateUniqueTextureId();
			mv_Textures.push_back(specular_texture);
			result.m_SpecularTextureId = specular_texture.GetTextureId();
		}

		if (normal_texture.mp_RawData.Get() != nullptr && normal_texture.mp_ShaderResource.Get() != nullptr)
		{
			normal_texture.m_TextureId = GenerateUniqueTextureId();
			mv_Textures.push_back(normal_texture);
			result.m_NormalTextureId = normal_texture.GetTextureId();
		}

		aiColor4D color;
		p_Material->Get(AI_MATKEY_BASE_COLOR, color);
		result.m_Color = DirectX::XMFLOAT4(color.r, color.g, color.b, color.a);

		return result;
	}

	uint32_t Graphics::GenerateUniqueShaderId()
	{
		if (mv_Shaders.empty())
			return 1;

		uint32_t id = 1;
		bool idFound = false;

		do {
			idFound = false;

			for (auto& shader : mv_Shaders)
			{
				if (shader.m_ShaderId == id)
					idFound = true;
			}

			if (idFound)
				id++;

		} while (idFound);

		return id;
	}

	uint32_t Graphics::GenerateUniqueTextureId()
	{
		if (mv_Textures.empty())
			return 1;

		uint32_t id = 1;
		bool idFound = false;

		do {
			idFound = false;

			for (auto& texture : mv_Textures)
			{
				if (texture.m_TextureId == id)
					idFound = true;
			}

			if (idFound)
				id++;

		} while (idFound);

		return id;
	}

	uint32_t Graphics::GenerateUniqueModelId()
	{
		if (mv_Textures.empty())
			return 1;

		uint32_t id = 1;
		bool idFound = false;

		do {
			idFound = false;

			for (auto& model : mv_Models)
			{
				if (model.m_ModelId == id)
					idFound = true;
			}

			if (idFound)
				id++;

		} while (idFound);

		return id;
	}

	namespace MultiThread
	{
		void GraphicsMT::CreateRasterizerState(ID3D11Device* p_Device, ID3D11RasterizerState** pp_Rasterizer, GfxUtils::RasterizerMode mode)
		{
			LOG_F(INFO, "Creating rasterizer on thread %x", (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));
			
			D3D11_RASTERIZER_DESC desc = {};
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = FALSE;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = 0.0f;
			desc.DepthClipEnable = FALSE;
			desc.ScissorEnable = FALSE;
			desc.MultisampleEnable = FALSE;
			desc.AntialiasedLineEnable = FALSE;

			switch (mode)
			{
			case Cc::GfxUtils::RasterizerMode::RasterizerMode_Solid:
				LOG_F(INFO, "Rasterizer fill mode set to solid");
				desc.FillMode = D3D11_FILL_SOLID;
				break;
			case Cc::GfxUtils::RasterizerMode::RasterizerMode_WireFrame:
				LOG_F(INFO, "Rasterizer fill mode set to wireframe");
				desc.FillMode = D3D11_FILL_WIREFRAME;
				break;
			default:
				LOG_F(WARNING, "Rasterizer mode was not specified! Defaulting to solid...");
				desc.FillMode = D3D11_FILL_SOLID;
				break;
			}

			HRESULT hr = p_Device->CreateRasterizerState(&desc, pp_Rasterizer);
			if (FAILED(hr)) throw GraphicsException(hr);

			LOG_F(INFO, "Rasterizer created");
		}

		void GraphicsMT::CreateSamplerState(ID3D11Device* p_Device, ID3D11SamplerState** pp_Sampler)
		{
			LOG_F(INFO, "Creating sampler on thread %x", (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));

			D3D11_SAMPLER_DESC desc = {};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			HRESULT hr = p_Device->CreateSamplerState(&desc, pp_Sampler);
			if (FAILED(hr)) throw GraphicsException(hr);

			LOG_F(INFO, "Sampler created");
		}

		void GraphicsMT::CompileVertexShader(ID3D11Device* p_Device, ID3D11VertexShader** pp_Shader, ID3D11InputLayout** pp_Layout, std::wstring filePath)
		{
			LOG_F(INFO, "Compiling %ls on thread %x", filePath.c_str(), (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));

			UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined _DEBUG || DEBUG
			LOG_F(INFO, "Enabling D3DCOMPILE_DEBUG flag for %ls", filePath.c_str());
			compileFlags |= D3DCOMPILE_DEBUG;
#endif

			ID3DBlob* p_Code = nullptr;
			ID3DBlob* p_Error = nullptr;

			HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &p_Code, &p_Error);
			if (FAILED(hr))
			{
				if (p_Error != nullptr)
					LOG_F(ERROR, "%ls compilation failed! Reason: %s", filePath.c_str(), (char*)p_Error->GetBufferPointer());

				if (p_Error) p_Error->Release();
				if (p_Code) p_Code->Release();

				return;
			}

			LOG_F(INFO, "%ls compiled", filePath.c_str());

			hr = p_Device->CreateVertexShader(p_Code->GetBufferPointer(), p_Code->GetBufferSize(), nullptr, pp_Shader);
			if (FAILED(hr))
			{
				LOG_F(ERROR, "Failed to create vertex shader for %ls", filePath.c_str());
				if (p_Error) p_Error->Release();
				if (p_Code) p_Code->Release();
				return;
			}

			LOG_F(INFO, "Created vertex shader for %ls", filePath.c_str());

			D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};

			hr = p_Device->CreateInputLayout(layoutDesc, _countof(layoutDesc), p_Code->GetBufferPointer(), p_Code->GetBufferSize(), pp_Layout);
			if (FAILED(hr))
			{
				LOG_F(ERROR, "Failed to create input layout for %ls", filePath.c_str());
				if (p_Error) p_Error->Release();
				if (p_Code) p_Code->Release();
				return;
			}

			LOG_F(INFO, "Created input layout for %ls", filePath.c_str());

			if (p_Error) p_Error->Release();
			if (p_Code) p_Code->Release();
		}

		void GraphicsMT::CompilePixelShader(ID3D11Device* p_Device, ID3D11PixelShader** pp_Shader, std::wstring filePath)
		{
			LOG_F(INFO, "Compiling %ls on thread %x", filePath.c_str(), (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));

			UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined _DEBUG || DEBUG
			LOG_F(INFO, "Enabling D3DCOMPILE_DEBUG flag for %ls", filePath.c_str());
			compileFlags |= D3DCOMPILE_DEBUG;
#endif

			ID3DBlob* p_Code = nullptr;
			ID3DBlob* p_Error = nullptr;

			HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &p_Code, &p_Error);
			if (FAILED(hr))
			{
				if (p_Error != nullptr)
					LOG_F(ERROR, "%ls compilation failed! Reason: %s", filePath.c_str(), (char*)p_Error->GetBufferPointer());

				if (p_Error) p_Error->Release();
				if (p_Code) p_Code->Release();

				return;
			}

			LOG_F(INFO, "%ls compiled", filePath.c_str());

			hr = p_Device->CreatePixelShader(p_Code->GetBufferPointer(), p_Code->GetBufferSize(), nullptr, pp_Shader);
			if (FAILED(hr))
			{
				LOG_F(ERROR, "Failed to create pixel shader for %ls", filePath.c_str());
				if (p_Error) p_Error->Release();
				if (p_Code) p_Code->Release();
				return;
			}

			LOG_F(INFO, "Created pixel shader for %ls", filePath.c_str());

			if (p_Error) p_Error->Release();
			if (p_Code) p_Code->Release();
		}

		void GraphicsMT::LoadTexture(ID3D11Device* p_Device, ID3D11Texture2D** pp_RawData, ID3D11ShaderResourceView** pp_Srv, std::string filePath)
		{
			//LOG_F(INFO, "Loading %ls on thread %x", filePath.c_str(), (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));

			std::string path = g_TexturePath + StripPathToFileName(filePath);

			std::vector<unsigned char> buffer;
			unsigned int width, height;

			int ret = lodepng::decode(buffer, width, height, path);
			if (ret)
			{
				LOG_F(ERROR, "Failed to decode %s, error code %u", path.c_str(), ret);
				return;
			}

			LOG_F(INFO, "Texture decoded");

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = buffer.data();
			data.SysMemPitch = (width * 4);
			data.SysMemSlicePitch = (width * height * 4);

			HRESULT hr = p_Device->CreateTexture2D(&texDesc, &data, pp_RawData);
			if (FAILED(hr))
			{
				LOG_F(ERROR, "CreateTexture2D failed for %s, error code %u", path.c_str(), hr);
				return;
			}

			LOG_F(INFO, "Data buffer created");

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			hr = p_Device->CreateShaderResourceView(*pp_RawData, &srvDesc, pp_Srv);
			if (FAILED(hr))
			{
				LOG_F(ERROR, "CreateShaderResourceView failed for %s, error code %u", path.c_str(), hr);
				return;
			}

			LOG_F(INFO, "Shader resource view created");
		}

		void GraphicsMT::CreateBuffer(ID3D11Device* p_Device, ID3D11Buffer** pp_Buffer, size_t bufSize, void* p_Data, GfxUtils::BufferType type)
		{
			LOG_F(INFO, "Creating buffer on thread %x", (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()));

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = bufSize;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			switch (type)
			{
			case Cc::GfxUtils::BufferType::BufferType_Vertex:
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				break;
			case Cc::GfxUtils::BufferType::BufferType_Index:
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				break;
			case Cc::GfxUtils::BufferType::BufferType_Constant:
				desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				break;
			default:
				LOG_F(WARNING, "Unrecognized buffer type! Defaulting to constant buffer!");
				desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				break;
			}

			if (p_Data != nullptr)
			{
				D3D11_SUBRESOURCE_DATA data = {};
				data.pSysMem = p_Data;

				HRESULT hr = p_Device->CreateBuffer(&desc, &data, pp_Buffer);
				if (FAILED(hr))
				{
					LOG_F(ERROR, "Failed to create buffer!");
					return;
				}
			}
			else
			{
				HRESULT hr = p_Device->CreateBuffer(&desc, nullptr, pp_Buffer);
				if (FAILED(hr))
				{
					LOG_F(ERROR, "Failed to create buffer!");
					return;
				}
			}

		}
	}

	
}