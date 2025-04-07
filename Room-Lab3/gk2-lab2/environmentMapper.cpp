#include "environmentMapper.h"
#include "dxstructures.h"
#include "dxDevice.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const int EnvironmentMapper::TEXTURE_SIZE = 256;

EnvironmentMapper::EnvironmentMapper(const DxDevice& device, float nearPlane, float farPlane, XMFLOAT3 position)
	: m_nearPlane(nearPlane), m_farPlane(farPlane),	m_position(position.x, position.y, position.z, 1.0f)
{
	Texture2DDescription texDesc;
	// TODO : 1.11 Setup texture width, height, mip levels and bind flags
	texDesc.Height = texDesc.Width = TEXTURE_SIZE;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	texDesc.MipLevels = 1;

	// TODO : 1.12 Uncomment following lines
	m_faceTexture = device.CreateTexture(texDesc); 
	m_renderTarget = device.CreateRenderTargetView(m_faceTexture);

	SIZE s;
	s.cx = s.cy = TEXTURE_SIZE;
	m_depthBuffer = device.CreateDepthStencilView(s);

	// TODO : 1.13 Create description for empty texture used as environment cube map, setup texture's width, height, mipLevels, bindflags, array size and miscFlags
	texDesc.ArraySize = 6;
	texDesc.Width = texDesc.Height = TEXTURE_SIZE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	// TODO : 1.14 Uncomment following lines
	m_envTexture = device.CreateTexture(texDesc);
	m_envView = device.CreateShaderResourceView(m_envTexture);

	//Shaders
	auto vsCode = device.LoadByteCode(L"envMapVS.cso");
	m_envVS = device.CreateVertexShader(vsCode);
	auto psCode = device.LoadByteCode(L"envMapPS.cso");
	m_envPS = device.CreatePixelShader(psCode);
}

void EnvironmentMapper::Begin(const dx_ptr<ID3D11DeviceContext>& context) const
{
	context->VSSetShader(m_envVS.get(), nullptr, 0);
	auto tx_ptr = m_envView.get();
	context->PSSetShaderResources(0, 1, &tx_ptr);
	context->PSSetShader(m_envPS.get(), nullptr, 0);
}

DirectX::XMMATRIX mini::gk2::EnvironmentMapper::FaceViewMtx(D3D11_TEXTURECUBE_FACE face) const
{
	// TODO : 1.15 Setup view matrix
	XMVECTOR upDirection;
	XMVECTOR faceDirection;
	switch (face)
	{
	case D3D11_TEXTURECUBE_FACE_POSITIVE_X:
		// For the +X face, we look toward -X with up along +Y.
		upDirection = XMVectorSet(0, 1, 0, 0);
		faceDirection = XMVectorSet(-1, 0, 0, 0);
		break;
	case D3D11_TEXTURECUBE_FACE_NEGATIVE_X:
		// For the -X face, we look toward +X with up along +Y.
		upDirection = XMVectorSet(0, 1, 0, 0);
		faceDirection = XMVectorSet(1, 0, 0, 0);
		break;
	case D3D11_TEXTURECUBE_FACE_POSITIVE_Y:
		// For the +Y face, we look upward (+Y). To avoid a collinear up vector, we use +Z as “up”.
		upDirection = XMVectorSet(0, 0, 1, 0);
		faceDirection = XMVectorSet(0, 1, 0, 0);
		break;
	case D3D11_TEXTURECUBE_FACE_NEGATIVE_Y:
		// For the -Y face, we look downward (–Y) with an up vector of –Z.
		upDirection = XMVectorSet(0, 0, -1, 0);
		faceDirection = XMVectorSet(0, -1, 0, 0);
		break;
	case D3D11_TEXTURECUBE_FACE_POSITIVE_Z:
		// For the +Z face, we look toward –Z with up along +Y.
		upDirection = XMVectorSet(0, 1, 0, 0);
		faceDirection = XMVectorSet(0, 0, -1, 0);
		break;
	case D3D11_TEXTURECUBE_FACE_NEGATIVE_Z:
		// For the –Z face, we look toward +Z with up along +Y.
		upDirection = XMVectorSet(0, 1, 0, 0);
		faceDirection = XMVectorSet(0, 0, 1, 0);
		break;
	default:
		// Fallback: default to the +X face orientation.
		upDirection = XMVectorSet(0, 1, 0, 0);
		faceDirection = XMVectorSet(-1, 0, 0, 0);
		break;
	}

	// Return the view matrix using the camera position (m_position), the face's look direction, and up vector.
	return XMMatrixLookToLH(XMLoadFloat4(&m_position), faceDirection, upDirection);
}

DirectX::XMFLOAT4X4 mini::gk2::EnvironmentMapper::FaceProjMtx() const
{
	XMFLOAT4X4 proj;

	// TODO : 1.17 Replace with correct implementation
	// 90-degree FOV (XM_PIDIV2), square aspect ratio (1.0f),
	// near plane = m_nearPlane, far plane = m_farPlane.
	XMStoreFloat4x4(&proj, XMMatrixPerspectiveFovLH(
		XM_PIDIV2,      // Vertical field of view (90°)
		1.0f,           // Aspect ratio = 1 (square)
		m_nearPlane,
		m_farPlane
	));;

	return proj;
}

void EnvironmentMapper::SetTarget(const dx_ptr<ID3D11DeviceContext>& context)
{
	D3D11_VIEWPORT viewport;

	// TODO : 1.18 Setup viewport
	viewport.TopLeftX = viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	context->RSSetViewports(1, &viewport);
	ID3D11RenderTargetView* targets[1] = { m_renderTarget.get() };
	context->OMSetRenderTargets(1, targets, m_depthBuffer.get());
}

void mini::gk2::EnvironmentMapper::ClearTarget(const dx_ptr<ID3D11DeviceContext>& context)
{
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(m_renderTarget.get(), clearColor);
	context->ClearDepthStencilView(m_depthBuffer.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void EnvironmentMapper::SaveFace(const dx_ptr<ID3D11DeviceContext>& context, D3D11_TEXTURECUBE_FACE face)
{
	if (face < 0 || face > 5)
		return;
	
	// TODO : 1.19 Copy face to environment cube map
	context.get()->CopySubresourceRegion(m_envTexture.get(), (UINT)face, 0, 0, 0, m_faceTexture.get(), 0, nullptr);
}