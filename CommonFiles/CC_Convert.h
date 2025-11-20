#pragma once
#include "CC_Core.h"

namespace Cc
{
	std::wstring ConvertStringToWideString(const std::string& s);
	std::string ConvertWideStringToString(const std::wstring& ws);

#if defined PLAT_WIN32 && GAPI_DX
	glm::mat4x4 ConvertXmMatrixToMat4x4(const DirectX::XMMATRIX& input);
	glm::mat4x4 ConvertXmFloat4x4ToMat4x4(const DirectX::XMFLOAT4X4& input);
	DirectX::XMMATRIX ConvertMat4x4ToXmMatrix(const glm::mat4x4& input);
	DirectX::XMFLOAT4X4 ConvertMat4x4ToXmFloat4x4(const glm::mat4x4& input);
	DirectX::XMFLOAT2 ConvertVec2ToXmFloat2(const glm::vec2& input);
	DirectX::XMFLOAT3 ConvertVec3ToXmFloat3(const glm::vec3& input);
	DirectX::XMFLOAT4 ConvertVec4ToXmFloat4(const glm::vec4& input);
#endif

}