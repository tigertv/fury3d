#include "Fury/Camera.h"
#include "Fury/Plane.h"
#include "Fury/SceneNode.h"

namespace fury
{
	Camera::Ptr Camera::Create()
	{
		return std::make_shared<Camera>();
	}

	Camera::Camera() : m_Perspective(false)
	{
		m_TypeIndex = typeid(Camera);
		m_ShadowAABB = BoxBounds(Vector4(0), Vector4(0));
	}

	Component::Ptr Camera::Clone() const
	{
		auto ptr = Camera::Create();
		ptr->m_ProjectionParams = m_ProjectionParams;
		ptr->m_Perspective = m_Perspective;
		ptr->m_ProjectionMatrix = m_ProjectionMatrix;
		ptr->m_Frustum = m_Frustum;
		ptr->m_ShadowAABB = m_ShadowAABB;
		return ptr;
	}

	void Camera::PerspectiveFov(float fov, float ratio, float near, float far)
	{
		m_Perspective = true;

		float top = near * tan(fov / 2.0f);
		float right = top * ratio;

		m_ProjectionParams[0] = -right;
		m_ProjectionParams[1] = right;
		m_ProjectionParams[2] = -top;
		m_ProjectionParams[3] = top;
		m_ProjectionParams[4] = near;
		m_ProjectionParams[5] = far;

		m_ProjectionMatrix.PerspectiveOffCenter(-right, right, -top, top, near, far);
		m_Frustum.Setup(-right, right, -top, top, near, far);
	}

	void Camera::PerspectiveOffCenter(float left, float right, float bottom, float top, float near, float far)
	{
		m_ProjectionParams[0] = left;
		m_ProjectionParams[1] = right;
		m_ProjectionParams[2] = bottom;
		m_ProjectionParams[3] = top;
		m_ProjectionParams[4] = near;
		m_ProjectionParams[5] = far;
		m_Perspective = true;

		m_ProjectionMatrix.PerspectiveOffCenter(left, right, bottom, top, near, far);
		m_Frustum.Setup(left, right, bottom, top, near, far);
	}

	void Camera::OrthoOffCenter(float left, float right, float bottom, float top, float near, float far)
	{
		m_Perspective = true;

		m_ProjectionParams[0] = left;
		m_ProjectionParams[1] = right;
		m_ProjectionParams[2] = bottom;
		m_ProjectionParams[3] = top;
		m_ProjectionParams[4] = near;
		m_ProjectionParams[5] = far;

		m_ProjectionMatrix.OrthoOffCenter(left, right, bottom, top, near, far);
		m_Frustum.Setup(left, right, bottom, top, near, far);
	}

	Matrix4 Camera::GetProjectionMatrix() const
	{
		return m_ProjectionMatrix;
	}

	Matrix4 Camera::GetProjectionMatrix(float near, float far) const
	{
		Matrix4 pm;
		if (m_Perspective)
			pm.PerspectiveOffCenter(m_ProjectionParams[0], m_ProjectionParams[1], m_ProjectionParams[2], m_ProjectionParams[3], near, far);
		else
			pm.OrthoOffCenter(m_ProjectionParams[0], m_ProjectionParams[1], m_ProjectionParams[2], m_ProjectionParams[3], near, far);
		return pm;
	}

	Frustum Camera::GetFrustum() const
	{
		return m_Frustum;
	}

	Frustum Camera::GetFrustum(float near, float far) const
	{
		Frustum clone;
		float curNear = GetNear();
		float curLeft = m_ProjectionParams[0];
		float curRight = m_ProjectionParams[1];
		float curBottom = m_ProjectionParams[2];
		float curTop = m_ProjectionParams[3];

		float toNear = near / curNear;
		float top = curTop * toNear;
		float right = curRight * toNear;
		float bottom = curBottom * toNear;
		float left = curLeft * toNear;

		clone.Setup(left, right, bottom, top, near, far);
		clone.Transform(m_Frustum.GetTransformMatrix());
		return clone;
	}

	float Camera::GetNear() const
	{
		return m_ProjectionParams[4];
	}

	float Camera::GetFar() const
	{
		return m_ProjectionParams[5];
	}

	float Camera::GetShadowFar() const
	{
		return m_ShadowFar;
	}

	void Camera::SetShadowFar(float far)
	{
		m_ShadowFar = far;
	}

	BoxBounds Camera::GetShadowBounds(bool worldSpace) const
	{
		if (worldSpace)
			return m_Frustum.GetTransformMatrix().Multiply(m_ShadowAABB);
		else
			return m_ShadowAABB;
	}

	void Camera::SetShadowBounds(Vector4 min, Vector4 max)
	{
		m_ShadowAABB.SetMinMax(min, max);
	}

	bool Camera::IsPerspective() const
	{
		return m_Perspective;
	}

	void Camera::Transform(const Matrix4 &matrix)
	{
		m_Frustum.Transform(matrix);
	}

	bool Camera::IsVisible(const BoxBounds &aabb) const
	{
		return m_Frustum.IsInsideFast(aabb);
	}

	bool Camera::IsVisible(const SphereBounds &bsphere) const
	{
		return m_Frustum.IsInsideFast(bsphere);
	}

	bool Camera::IsVisible(Vector4 point) const
	{
		return m_Frustum.IsInsideFast(point);
	}

	void Camera::OnAttaching(const std::shared_ptr<SceneNode> &node)
	{
		Component::OnAttaching(node);
		Camera::Ptr selfPtr = shared_from_this();
		m_SignalKey = node->OnTransformChange->Connect(selfPtr, &Camera::OnSceneNodeTransformChange);
	}

	void Camera::OnDetaching(const std::shared_ptr<SceneNode> &node)
	{
		Component::OnDetaching(node);
		node->OnTransformChange->Disconnect(m_SignalKey);
		m_SignalKey = 0;
	}

	void Camera::OnSceneNodeTransformChange(const std::shared_ptr<SceneNode> &sender)
	{
		Transform(sender->GetWorldMatrix());
	}

}
