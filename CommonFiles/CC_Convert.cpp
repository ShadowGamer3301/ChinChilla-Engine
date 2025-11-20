#include "CC_Convert.h"

namespace Cc
{
	std::wstring ConvertStringToWideString(const std::string& s)
	{
		return std::wstring(s.begin(), s.end());
	}

	std::string ConvertWideStringToString(const std::wstring& ws)
	{
		return std::string(ws.begin(), ws.end());
	}

#if defined PLAT_WIN32 && defined GAPI_DX

	glm::mat4x4 ConvertXmMatrixToMat4x4(const DirectX::XMMATRIX& input)
	{
		glm::mat4x4 output;
		DirectX::XMFLOAT4X4 temp;

		//Since we cannot directly acess contents of
		//XMMATRIX copy the values to XMFLOAT4X4
		DirectX::XMStoreFloat4x4(&temp, input);
		
		//Copy values from XMFLOAT4X4 to mat4x4
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				output[i][j] = temp.m[i][j];

		//Since DirectX matricies are row-major
		//and glms are column-major we need to
		//transpose the matrix
		output = glm::transpose(output);

		return output;
	}

	glm::mat4x4 ConvertXmFloat4x4ToMat4x4(const DirectX::XMFLOAT4X4& input)
	{
		glm::mat4x4 output;

		//Copy values from XMFLOAT4X4 to mat4x4
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				output[i][j] = input.m[i][j];

		//Since DirectX matricies are row-major
		//and glms are column-major we need to
		//transpose the matrix
		output = glm::transpose(output);

		return output;
	}

	DirectX::XMMATRIX ConvertMat4x4ToXmMatrix(const glm::mat4x4& input)
	{
		DirectX::XMMATRIX output;
		DirectX::XMFLOAT4X4 temp;

		//Copy values from XMFLOAT4X4 to mat4x4
		//since we cannot access members of
		//XMMATRIX directly
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				temp.m[i][j] = input[i][j];

		//Copy data from XMFLOAT4x4 to XMMATRIX
		output = DirectX::XMLoadFloat4x4(&temp);

		//Since DirectX matricies are row-major
		//and glms are column-major we need to
		//transpose the matrix
		output = DirectX::XMMatrixTranspose(output);

		return output;
	}

	DirectX::XMFLOAT4X4 ConvertMat4x4ToXmFloat4x4(const glm::mat4x4& input)
	{
		DirectX::XMFLOAT4X4 output;
		DirectX::XMMATRIX temp;

		//Copy values from XMFLOAT4X4 to mat4x4
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				output.m[i][j] = input[i][j];

		//Copy data from XMFLOAT4x4 to XMMATRIX
		//so we can transpose the matrix
		temp = DirectX::XMLoadFloat4x4(&output);

		//Since DirectX matricies are row-major
		//and glms are column-major we need to
		//transpose the matrix
		temp = DirectX::XMMatrixTranspose(temp);

		//Copy back transposed data to XMFLOAT4X4
		DirectX::XMStoreFloat4x4(&output, temp);

		return output;
	}

	DirectX::XMFLOAT2 ConvertVec2ToXmFloat2(const glm::vec2& input)
	{
		return DirectX::XMFLOAT2(input.x, input.y);
	}

	DirectX::XMFLOAT3 ConvertVec3ToXmFloat3(const glm::vec3& input)
	{
		return DirectX::XMFLOAT3(input.x, input.y, input.z);
	}

	DirectX::XMFLOAT4 ConvertVec4ToXmFloat4(const glm::vec4& input)
	{
		return DirectX::XMFLOAT4(input.x, input.y, input.z, input.w);
	}

#endif
}


