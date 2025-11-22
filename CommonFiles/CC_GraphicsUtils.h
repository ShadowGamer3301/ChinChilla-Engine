#pragma once
#include "CC_Core.h"
#include "CC_Convert.h"

namespace Cc
{
	class Graphics;

	namespace GfxUtils
	{
#if defined PLAT_WIN32 && defined GAPI_DX

		enum class RasterizerMode : uint32_t
		{
			RasterizerMode_Solid = 0,
			RasterizerMode_WireFrame = 1,
		};

		enum class SamplerMode : uint32_t
		{
			SamplerModeLinear = 0,
			SamplerModeAnisotropic = 1
		};

		enum class BufferType : uint32_t
		{
			BufferType_Vertex = 0,
			BufferType_Index = 1,
			BufferType_Constant = 2,
		};

		struct VERTEX
		{
			DirectX::XMFLOAT3 m_Pos;
			DirectX::XMFLOAT3 m_Normal;
			DirectX::XMFLOAT2 m_TexCoord;
		};

		class Material
		{
			friend class Cc::Graphics;
		public:
			inline void SetColor(float r, float g, float b, float a) noexcept { m_Color = DirectX::XMFLOAT4(r, g, b, a); }
			inline void SetDiffuseTexture(uint32_t textureId) noexcept { m_DiffuseTextureId = textureId; }
			inline void SetSpecularTexture(uint32_t textureId) noexcept { m_SpecularTextureId = textureId; }
			inline void SetNormalTexture(uint32_t textureId) noexcept { m_NormalTextureId = textureId; }

		private:
			DirectX::XMFLOAT4 m_Color;

			uint32_t m_DiffuseTextureId = 0;
			uint32_t m_SpecularTextureId = 0;
			uint32_t m_NormalTextureId = 0;
		};

		class Texture
		{
			friend class Cc::Graphics;
		public:
			inline uint32_t GetTextureId() const noexcept { return m_TextureId; }
			inline std::string GetTexturePath() const noexcept { return m_TexturePath; }

		private:
			uint32_t m_TextureId = 0;
			std::string m_TexturePath = "";
			Microsoft::WRL::ComPtr<ID3D11Texture2D> mp_RawData;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mp_ShaderResource;
		};

		class Mesh
		{
			friend class Model;
			friend class Cc::Graphics;
		private:
			Microsoft::WRL::ComPtr<ID3D11Buffer> mp_VertexBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer> mp_IndexBuffer;
			Material m_Material;
		};

		class Model
		{
			friend class Cc::Graphics;
		public:
			inline uint32_t GetModelId() const noexcept { return m_ModelId; }
			inline std::string GetModelPath() const noexcept { return m_ModelPath; }

		private:
			std::vector<Mesh> mv_Meshes;
			Microsoft::WRL::ComPtr<ID3D11Buffer> mp_ConstBuffer;
			uint32_t m_ModelId = 0;
			std::string m_ModelPath = "";
		};

		class Shader
		{
			friend class Cc::Graphics;
		public:
			inline std::string GetPixelPath() const noexcept { return m_PixelPath; }
			inline std::string GetVertexPath() const noexcept { return m_VertexPath; }
			inline uint32_t GetShaderId() const noexcept { return m_ShaderId; }

		private:
			uint32_t m_ShaderId = 0;
			std::string m_VertexPath = "", m_PixelPath = "";
			Microsoft::WRL::ComPtr<ID3D11VertexShader> mp_Vertex;
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mp_Pixel;
			Microsoft::WRL::ComPtr<ID3D11InputLayout> mp_Layout;
		};

		class CCAPI Camera
		{
		public:
			Camera();
			void SetProjectionValues(float fov, float aspectRatio, float nz, float fz);

			inline glm::mat4x4 GetViewMatrix() const noexcept { return ConvertXmMatrixToMat4x4(m_ViewMatrix); }
			inline glm::mat4x4 GetProjectionMatrix() const noexcept { return ConvertXmMatrixToMat4x4(m_ProjMatrix); }

			void SetPosition(float x, float y, float z);
			void SetRotation(float x, float y, float z);
			void AdjustPosition(float x, float y, float z);
			void AdjustRotation(float x, float y, float z);
			void SetLookAtPos(float x, float y, float z);

		private:
			void UpdateViewMatrix();

		private:
			DirectX::XMFLOAT3 m_Position;
			DirectX::XMFLOAT3 m_Rotation;
			DirectX::XMVECTOR m_PositionVec;
			DirectX::XMVECTOR m_RotationVec;
			DirectX::XMMATRIX m_ViewMatrix;
			DirectX::XMMATRIX m_ProjMatrix;

			const DirectX::XMVECTOR DEFAULT_FORWARD_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			const DirectX::XMVECTOR DEFAULT_UP_VECTOR = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			const DirectX::XMVECTOR DEFAULT_RIGHT_VECTOR = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			const DirectX::XMVECTOR DEFAULT_BACKWARD_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
			const DirectX::XMVECTOR DEFAULT_DOWN_VECTOR = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
			const DirectX::XMVECTOR DEFAULT_LEFT_VECTOR = DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);

			DirectX::XMVECTOR m_ForwardVec;
			DirectX::XMVECTOR m_BackwardVec;
			DirectX::XMVECTOR m_LeftVec;
			DirectX::XMVECTOR m_RightVec;
		};
#endif
	}
}