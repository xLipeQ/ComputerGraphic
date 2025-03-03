#include "dxApplication.h"
#include <DirectXMath.h>

using namespace mini;
using namespace DirectX;

DxApplication::DxApplication(HINSTANCE hInstance)
	: WindowApplication(hInstance), m_device(m_window)
{
	ID3D11Texture2D *temp;
	dx_ptr<ID3D11Texture2D> backTexture;
	m_device.swapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&temp));
	backTexture.reset(temp);

	m_backBuffer = m_device.CreateRenderTargetView(backTexture);

	SIZE wndSize = m_window.getClientSize();
	m_depthBuffer = m_device.CreateDepthStencilView(wndSize);
	auto backBuffer = m_backBuffer.get();
	m_device.context()->OMSetRenderTargets(1, &backBuffer, m_depthBuffer.get());
	Viewport viewport{ wndSize };
	m_device.context()->RSSetViewports(1, &viewport);

	const auto vsBytes = DxDevice::LoadByteCode(L"vs.cso");
	const auto psBytes = DxDevice::LoadByteCode(L"ps.cso");
	m_vertexShader = m_device.CreateVertexShader(vsBytes);
	m_pixelShader = m_device.CreatePixelShader(psBytes);

	// matrices inicialization
	XMStoreFloat4x4(&m_modelMtx, XMMatrixIdentity());
	XMStoreFloat4x4(&m_viewMtx,
		XMMatrixRotationX(XMConvertToRadians(-30)) *
		XMMatrixTranslation(0.0f, 0.0f, 10.0f));
	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45),
		static_cast<float>(wndSize.cx) / wndSize.cy,
		0.1f, 100.0f));
	m_cbMVP = m_device.CreateConstantBuffer<XMFLOAT4X4>();


	/*const auto vertices = CreateTriangleVertices();
	m_vertexBuffer = m_device.CreateVertexBuffer(vertices);
	std::vector<D3D11_INPUT_ELEMENT_DESC> elements{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	m_layout = m_device.CreateInputLayout(elements, vsBytes);*/

	const auto vertices = CreateCubeVertices();
	m_vertexBuffer = m_device.CreateVertexBuffer(vertices);
	const auto indices = CreateCubeIndices();
	m_indexBuffer = m_device.CreateIndexBuffer(indices);
	std::vector<D3D11_INPUT_ELEMENT_DESC> elements{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			offsetof(VertexPositionColor, color),
			D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	m_layout = m_device.CreateInputLayout(elements, vsBytes);
}

int DxApplication::MainLoop()
{
	MSG msg;
	//PeekMessage doesn't change MSG if there are no messages to be recieved.
	//However unlikely the case may be, that the first call to PeekMessage
	//doesn't find any messages, msg is zeroed out to make sure loop condition
	//isn't reading unitialized values.
	ZeroMemory(&msg, sizeof msg);
	do
	{
		if (PeekMessage(&msg, nullptr, 0,0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update();
			Render();
			m_device.swapChain()->Present(0, 0);
		}
	} while (msg.message != WM_QUIT);
	return static_cast<int>(msg.wParam);
}

void DxApplication::Update()
{
	XMStoreFloat4x4(&m_modelMtx, XMLoadFloat4x4(&m_modelMtx) *
		XMMatrixRotationX(0.003f));
	D3D11_MAPPED_SUBRESOURCE res;
	m_device.context()-> Map(m_cbMVP.get(), 0,
	 D3D11_MAP_WRITE_DISCARD, 0, &res);
	XMMATRIX mvp = XMLoadFloat4x4(&m_modelMtx) *
	 XMLoadFloat4x4(&m_viewMtx) * XMLoadFloat4x4(&m_projMtx);
	memcpy(res.pData, &mvp, sizeof(XMMATRIX));
	m_device.context()-> Unmap(m_cbMVP.get(), 0);

}

std::vector<DirectX::XMFLOAT2> DxApplication::CreateTriangleVertices()
{
	return {
		{0.0f, 0.5f},
		{0.45f, -0.5},
		{-0.45f, -0.5f}
	};
}

std::vector<DxApplication::VertexPositionColor> DxApplication::CreateCubeVertices()
{
	return {
		// Front Face
		{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },

		// Back
		{ { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } },
		
		// left
		{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, +0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, 1.0f } },

		// top
		{ { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, // 12 (3)
		{ { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, 1.0f } }, // 13 (6)
		{ { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, // 14 (2)
		{ { -0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, 1.0f } }, // 15 (7)

		// right 
		{ { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } }, // 16 (1)
		{ { +0.5f, +0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } }, // 17 (2)
		{ { +0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, 1.0f } }, // 18 (6)
		{ { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, 1.0f } }, // 19 (5)

	};
}

std::vector<unsigned short> DxApplication::CreateCubeIndices()
{
	return {
		// front
		0,2,1, 0,3,2,

		// left 
		8,9,10, 9,11,10,

		// top
		12,13,14, 12,15,13,

		// right
		16,17,18, 19,16,18,

		// back
		4,5,7, 5,6,7,

		// botton
		4,1,5, 4,0,1

	};
}

void DxApplication::Render()
{
	float clearColor[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	m_device.context()->ClearRenderTargetView(m_backBuffer.get(), clearColor);

	m_device.context()->ClearDepthStencilView(m_depthBuffer.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_device.context()->VSSetShader(m_vertexShader.get(), nullptr, 0);
	m_device.context()->PSSetShader(m_pixelShader.get(), nullptr, 0);
	m_device.context()->IASetInputLayout(m_layout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	/*ID3D11Buffer * vbs[] = { m_vertexBuffer.get() };
	UINT strides[] = { sizeof(DirectX::XMFLOAT2) };
	UINT offsets[] = { 0 };
	m_device.context()->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	m_device.context()->Draw(3, 0);*/
	ID3D11Buffer* cbs[] = { m_cbMVP.get() };
	m_device.context()->VSSetConstantBuffers(0, 1, cbs);

	ID3D11Buffer* vbs[] = { m_vertexBuffer.get() };
	UINT strides[] = { sizeof(VertexPositionColor) };
	UINT offsets[] = { 0 };
	m_device.context()-> IASetVertexBuffers(
		0, 1, vbs, strides, offsets);
	m_device.context()->IASetIndexBuffer(m_indexBuffer.get(),
		DXGI_FORMAT_R16_UINT, 0);
	m_device.context()->DrawIndexed(36, 0, 0);

}