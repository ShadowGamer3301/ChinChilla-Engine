#include "CC_GraphicsUtils.h"

namespace Cc
{
	namespace GfxUtils
	{
		Camera::Camera()
		{
			m_Position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			m_PositionVec = DirectX::XMLoadFloat3(&m_Position);
			m_Rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			m_RotationVec = DirectX::XMLoadFloat3(&m_Rotation);
			UpdateViewMatrix();
		}

		void Camera::SetProjectionValues(float fov, float aspectRatio, float nz, float fz)
		{
			float fovRadians = (fov / 360.0f) * DirectX::XM_2PI;
			m_ProjMatrix = DirectX::XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nz, fz);
		}

		void Camera::SetPosition(float x, float y, float z)
		{
			m_Position = DirectX::XMFLOAT3(x, y, z);
			m_PositionVec = DirectX::XMLoadFloat3(&m_Position);
			UpdateViewMatrix();
		}

		void Camera::SetRotation(float x, float y, float z)
		{
			m_Rotation = DirectX::XMFLOAT3(x, y, z);
			m_RotationVec = DirectX::XMLoadFloat3(&m_Rotation);
			UpdateViewMatrix();
		}

		void Camera::AdjustPosition(float x, float y, float z)
		{
			m_Position.x += x;
			m_Position.y += y;
			m_Position.z += z;
			m_PositionVec = DirectX::XMLoadFloat3(&m_Position);
			UpdateViewMatrix();
		}

		void Camera::AdjustRotation(float x, float y, float z)
		{
			m_Rotation.x += x;
			m_Rotation.y += y;
			m_Rotation.z += z;
			m_RotationVec = DirectX::XMLoadFloat3(&m_Rotation);
			UpdateViewMatrix();
		}

		void Camera::SetLookAtPos(float x, float y, float z)
		{
			DirectX::XMFLOAT3 lookAtPos(x, y, z);

			if (lookAtPos.x == m_Position.x && lookAtPos.y == m_Position.y && lookAtPos.z == m_Position.z)
				return;

			float pitch = 0.0f;
			if (lookAtPos.y != 0.0f)
			{
				const float distance = sqrt(lookAtPos.x * lookAtPos.x + lookAtPos.z * lookAtPos.z);
				pitch = atan(lookAtPos.y / distance);
			}

			float yaw = 0.0f;
			if (lookAtPos.x != 0.0f)
			{
				yaw = atan(lookAtPos.x / lookAtPos.z);
			}
			if (lookAtPos.z > 0)
				yaw += DirectX::XM_PI;

			SetRotation(pitch, yaw, 0.0f);
		}

		void Camera::UpdateViewMatrix()
		{
			DirectX::XMMATRIX camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(this->m_Rotation.x, this->m_Rotation.y, this->m_Rotation.z);
			DirectX::XMVECTOR camTarget = DirectX::XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, camRotationMatrix);
			camTarget = DirectX::XMVectorAdd(camTarget, m_PositionVec);
			DirectX::XMVECTOR upDir = DirectX::XMVector3TransformCoord(DEFAULT_UP_VECTOR, camRotationMatrix);
			m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_PositionVec, camTarget, upDir);

			DirectX::XMMATRIX vecRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(0.0f, m_Rotation.y, 0.0f);
			m_ForwardVec = DirectX::XMVector3TransformCoord(DEFAULT_FORWARD_VECTOR, vecRotationMatrix);
			m_BackwardVec = DirectX::XMVector3TransformCoord(DEFAULT_BACKWARD_VECTOR, vecRotationMatrix);
			m_LeftVec = DirectX::XMVector3TransformCoord(DEFAULT_LEFT_VECTOR, vecRotationMatrix);
			m_RightVec = DirectX::XMVector3TransformCoord(DEFAULT_RIGHT_VECTOR, vecRotationMatrix);
		}
	}
}