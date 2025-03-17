#include "butterflyDemo.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

#pragma region Constants
const float ButterflyDemo::DODECAHEDRON_R = sqrtf(0.375f + 0.125f * sqrtf(5.0f));
const float ButterflyDemo::DODECAHEDRON_H = 1.0f + 2.0f * DODECAHEDRON_R;
const float ButterflyDemo::DODECAHEDRON_A = XMScalarACos(-0.2f * sqrtf(5.0f));

const float ButterflyDemo::MOEBIUS_R = 1.0f;
const float ButterflyDemo::MOEBIUS_W = 0.1f;
const unsigned int ButterflyDemo::MOEBIUS_N = 128;

const float ButterflyDemo::LAP_TIME = 10.0f;
const float ButterflyDemo::FLAP_TIME = 2.0f;
const float ButterflyDemo::WING_W = 0.15f;
const float ButterflyDemo::WING_H = 0.1f;
const float ButterflyDemo::WING_MAX_A = 8.0f * XM_PIDIV2 / 9.0f; //80 degrees

const unsigned int ButterflyDemo::BS_MASK = 0xffffffff;

const XMFLOAT4 ButterflyDemo::GREEN_LIGHT_POS = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
const XMFLOAT4 ButterflyDemo::BLUE_LIGHT_POS = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
const XMFLOAT4 ButterflyDemo::COLORS[] = {
	XMFLOAT4(253.0f / 255.0f, 198.0f / 255.0f, 137.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(255.0f / 255.0f, 247.0f / 255.0f, 153.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(196.0f / 255.0f, 223.0f / 255.0f, 155.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(162.0f / 255.0f, 211.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(130.0f / 255.0f, 202.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(122.0f / 255.0f, 204.0f / 255.0f, 200.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(109.0f / 255.0f, 207.0f / 255.0f, 246.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(125.0f / 255.0f, 167.0f / 255.0f, 216.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(131.0f / 255.0f, 147.0f / 255.0f, 202.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(135.0f / 255.0f, 129.0f / 255.0f, 189.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(161.0f / 255.0f, 134.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(244.0f / 255.0f, 154.0f / 255.0f, 193.0f / 255.0f, 100.0f / 255.0f)
};
#pragma endregion

#pragma region Initalization
ButterflyDemo::ButterflyDemo(HINSTANCE hInstance)
	: Base(hInstance, 1280, 720, L"Motyl"),
	  m_cbWorld(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	  m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	  m_cbLighting(m_device.CreateConstantBuffer<Lighting>()),
	  m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>())

{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	m_cbProj = m_device.CreateConstantBuffer<XMFLOAT4X4>();
	UpdateBuffer(m_cbProj, m_projMtx);
	XMFLOAT4X4 cameraMtx;
	XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
	UpdateCameraCB(cameraMtx);

	//Regular shaders
	auto vsCode = m_device.LoadByteCode(L"vs.cso");
	auto psCode = m_device.LoadByteCode(L"ps.cso");
	m_vs = m_device.CreateVertexShader(vsCode);
	m_ps = m_device.CreatePixelShader(psCode);

	m_il = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	//Billboard shaders
	vsCode = m_device.LoadByteCode(L"vsBillboard.cso");
	psCode = m_device.LoadByteCode(L"psBillboard.cso");
	m_vsBillboard = m_device.CreateVertexShader(vsCode);
	m_psBillboard = m_device.CreatePixelShader(psCode);
	D3D11_INPUT_ELEMENT_DESC elements[1] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	m_ilBillboard = m_device.CreateInputLayout(elements, vsCode);

	//Render states
	CreateRenderStates();

	//Meshes

	m_box = Mesh::ShadedBox(m_device);

	m_pentagon = Mesh::Pentagon(m_device);
	m_wing = Mesh::DoubleRect(m_device, WING_W, WING_H);
	CreateMoebuisStrip();

	m_bilboard = Mesh::Billboard(m_device, 2.0f);

	//Model matrices
	CreateDodecahadronMtx();

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb);
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLighting.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb);
}

void ButterflyDemo::CreateRenderStates()
//Setup render states used in various stages of the scene rendering
{
	DepthStencilDescription dssDesc;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; //Disable writing to depth buffer
	m_dssNoDepthWrite = m_device.CreateDepthStencilState(dssDesc); // depth stencil state for billboards

	//TODO : 1.20. Setup depth stencil state for writing to stencil buffer

	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	//TODO : 1.36. Setup depth stencil state for stencil test for billboards

	m_dssStencilTestNoDepthWrite = m_device.CreateDepthStencilState(dssDesc);

	//TODO : 1.21. Setup depth stencil state for stencil test for 3D objects

	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);

	RasterizerDescription rsDesc;
	//TODO : 1.13. Setup rasterizer state with ccw front faces

	m_rsCCW = m_device.CreateRasterizerState(rsDesc);

	BlendDescription bsDesc;
	//TODO : 1.26. Setup alpha blending state

	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	//TODO : 1.30. Setup additive blending state

	m_bsAdd = m_device.CreateBlendState(bsDesc);
}

void ButterflyDemo::CreateDodecahadronMtx()
//Compute dodecahedronMtx and mirrorMtx
{
	std::vector<DirectX::XMMATRIX> matrices(12);

	matrices[0] = XMMatrixIdentity() *
		XMMatrixRotationX(DirectX::XM_PIDIV2) *
		XMMatrixTranslation(0, -DODECAHEDRON_H / 2, 0) * 
		XMMatrixScaling(2, 2, 2);

	matrices[1] = matrices[0] *
		XMMatrixRotationY(DirectX::XM_PI) *
		XMMatrixRotationZ(DODECAHEDRON_A - DirectX::XM_PI);

	for (int i = 2; i < 6; i++) {
		matrices[i] = matrices[i - 1] *
			XMMatrixRotationY(DirectX::XM_PI * 2 / 5.f);
	}

	for (int i = 6; i < 12; i++) {
		matrices[i] = matrices[i - 6] *
			XMMatrixRotationZ(DirectX::XM_PI);
	}

	for (int i = 0; i < 12; i++) {
		XMStoreFloat4x4(&m_dodecahedronMtx[i], matrices[i]);
	}

	//TODO : 1.12. calculate m_mirrorMtx matrices
}

XMFLOAT3 ButterflyDemo::MoebiusStripPos(float t, float s)
{
	return XMFLOAT3(
		cos(t) * (MOEBIUS_R + MOEBIUS_W * s * cos(0.5f * t)),
		sin(t) * (MOEBIUS_R + MOEBIUS_W * s * cos(0.5f * t)),
		MOEBIUS_W * s * sin(0.5f * t)
	);
}

XMVECTOR ButterflyDemo::MoebiusStripDs(float t, float s)
{
	return XMVectorSet(
		cos(t) * cos(t / 2),
		sin(t) * cos(t / 2),
		sin(t / 2),
		0.0f
	);
}

XMVECTOR ButterflyDemo::MoebiusStripDt(float t, float s)
{
	float dx = -MOEBIUS_R * sin(t) - 0.5f * s * MOEBIUS_W * sin(0.5f * t) * cos(t) - MOEBIUS_W * s * cos(0.5f * t) * sin(t);
	float dy = MOEBIUS_R * cos(t) - 0.5f * s * MOEBIUS_W * sin(0.5f * t) * sin(t) + MOEBIUS_W * s * cos(0.5f * t) * cos(t);
	float dz = 0.5f * s * MOEBIUS_W * cos(0.5f * t);

	return XMVectorSet(dx, dy, dz, 0.0f);
}

void ButterflyDemo::CreateMoebuisStrip()
{
	std::vector<unsigned short> indices;
	std::vector<VertexPositionNormal> vertexBuffers;

	// Genereting vertices and normals - for every t we have to points -s and +s
	// We get its position and normal by derivative
	for (unsigned int i = 0; i <= MOEBIUS_N; ++i)
	{
		float t = 4.0f * XM_PI * i / MOEBIUS_N;

		for (int j = -1; j <= 1; j += 2) 
		{
			float s = static_cast<float>(j);

			// Pozycja
			VertexPositionNormal v;
			v.position = MoebiusStripPos(t, s);

			// Wektory styczne
			XMVECTOR ds = MoebiusStripDs(t, s);
			XMVECTOR dt = MoebiusStripDt(t, s);

			// Normalna - iloczyn wektorowy ds x dt
			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(ds, dt));
			XMFLOAT3 normalFloat3;
			XMStoreFloat3(&normalFloat3, normal);
			v.normal = normalFloat3;
			vertexBuffers.push_back(v);
		}
	}

	// Generating indices
	for (unsigned int i = 1; i <= MOEBIUS_N; ++i)
	{
		unsigned int i0 = (i - 1) * 2;
		unsigned int i1 = (i - 1) * 2 + 1;
		unsigned int i2 = i * 2;
		unsigned int i3 = i * 2 + 1;

		indices.push_back(i0);
		indices.push_back(i1);
		indices.push_back(i2);

		indices.push_back(i1);
		indices.push_back(i3);
		indices.push_back(i2);
	}

	// Tworzenie buforów Direct3D
	m_moebius = Mesh::SimpleTriMesh(m_device, vertexBuffers, indices);


}
#pragma endregion

#pragma region Per-Frame Update
void ButterflyDemo::Update(const Clock& c)
{
	Base::Update(c);
	double dt = c.getFrameTime();
	if (HandleCameraInput(dt))
	{
		XMFLOAT4X4 cameraMtx;
		XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
		UpdateCameraCB(cameraMtx);
	}
	UpdateButterfly(static_cast<float>(dt));
}

void ButterflyDemo::UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx)
{
	XMMATRIX mtx = XMLoadFloat4x4(&cameraMtx);
	XMVECTOR det;
	auto invvmtx = XMMatrixInverse(&det, mtx);
	XMFLOAT4X4 view[2] = { cameraMtx };
	XMStoreFloat4x4(view + 1, invvmtx);
	UpdateBuffer(m_cbView, view);
}

void ButterflyDemo::UpdateButterfly(float dtime)
//TODO : 1.10. Compute the matrices for butterfly wings. Position on the strip is determined based on time
{
	static float lap = 0.0f;
	lap += dtime;
	while (lap > LAP_TIME)
		lap -= LAP_TIME;
	//Value of the Moebius strip t parameter
	float t = 2 * lap / LAP_TIME;
	//Angle between wing current and vertical position
	float a = t * WING_MAX_A;
	t *= XM_2PI;
	if (a > WING_MAX_A)
		a = 2 * WING_MAX_A - a;

	DirectX::XMFLOAT3 P = MoebiusStripPos(t, 0);
	auto Pt = XMVector3Normalize(MoebiusStripDt(t, 0));
	auto Ps = XMVector3Normalize(MoebiusStripDs(t, 0));
	auto PtPs = XMVector3Normalize(DirectX::XMVector3Cross(Ps, Pt));

	DirectX::XMMATRIX moebius;
	moebius.r[0] = DirectX::XMVectorSetW(Pt, 0.0f); 
	moebius.r[1] = DirectX::XMVectorSetW(PtPs, 0.0f); 
	moebius.r[2] = DirectX::XMVectorSetW(Ps, 0.0f);
	moebius.r[3] = DirectX::XMVectorSetW(XMLoadFloat3(&P), 1.0f); 

	//Write the rest of code here
	XMStoreFloat4x4(&m_wingMtx[0],
		XMMatrixTranslation(0.f, +WING_H / 2.f, 0.f) *
		XMMatrixRotationY(XM_PI / 2) *
		XMMatrixRotationZ(a) *
		XMMatrixRotationY(XM_PI / 2) *
		moebius
	);
	XMStoreFloat4x4(&m_wingMtx[1], 
		XMMatrixTranslation(0.f, +WING_H / 2.f, 0.f) *
		XMMatrixRotationY(XM_PI / 2) *
		XMMatrixRotationZ(-a) *
		XMMatrixRotationY(XM_PI / 2) *
		moebius
	);
}
#pragma endregion

#pragma region Frame Rendering Setup
void ButterflyDemo::SetShaders()
{
	m_device.context()->VSSetShader(m_vs.get(), 0, 0);
	m_device.context()->PSSetShader(m_ps.get(), 0, 0);
	m_device.context()->IASetInputLayout(m_il.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ButterflyDemo::SetBillboardShaders()
{
	m_device.context()->VSSetShader(m_vsBillboard.get(), 0, 0);
	m_device.context()->PSSetShader(m_psBillboard.get(), 0, 0);
	m_device.context()->IASetInputLayout(m_ilBillboard.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ButterflyDemo::Set1Light()
//Setup one positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ m_camera.getCameraPosition(), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};
	UpdateBuffer(m_cbLighting, l);
}

void ButterflyDemo::Set3Lights()
//Setup one white positional light at the camera
//TODO : 1.28. Setup two additional positional lights, green and blue.
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/{
			{ /*.position =*/ m_camera.getCameraPosition(), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			//Write the rest of the code here

		}
	};

	//comment the following line when structure is filled
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);

	UpdateBuffer(m_cbLighting, l);
}
#pragma endregion

#pragma region Drawing
void ButterflyDemo::DrawBox()
{
	XMFLOAT4X4 worldMtx;
	XMStoreFloat4x4(&worldMtx, XMMatrixIdentity());
	UpdateBuffer(m_cbWorld, worldMtx);
	m_box.Render(m_device.context());
}

void ButterflyDemo::DrawDodecahedron(bool colors)
{
	for (int i = 0; i < 12; i++) {
		if (colors) 
			UpdateBuffer(m_cbSurfaceColor, COLORS[i]);
		UpdateBuffer(m_cbWorld, m_dodecahedronMtx[i]);
		m_pentagon.Render(m_device.context());
	}
	//TODO : 1.14. Modify function so if colors parameter is set to false, all faces are drawn white instead
}

void ButterflyDemo::DrawMoebiusStrip()
{
	UpdateBuffer(m_cbWorld, XMMatrixIdentity());
	m_moebius.Render(m_device.context());
}

void ButterflyDemo::DrawButterfly()
//TODO : 1.11. Draw the butterfly
{
	UpdateBuffer(m_cbWorld, m_wingMtx[0]);
	m_wing.Render(m_device.context());
	UpdateBuffer(m_cbWorld, m_wingMtx[1]);
	m_wing.Render(m_device.context());
}

void ButterflyDemo::DrawBillboards()
//Setup billboards rendering and draw them
{
	//TODO : 1.33. Setup shaders and blend state

	//TODO : 1.34. Draw both billboards with appropriate colors and transformations

	//TODO : 1.35. Restore rendering state to it's original values

}

void ButterflyDemo::DrawMirroredWorld(unsigned int i)
//Draw the mirrored scene reflected in the i-th dodecahedron face
{
	//TODO : 1.22. Setup render state for writing to the stencil buffer

	//TODO : 1.23. Draw the i-th face

	//TODO : 1.24. Setup depth stencil state for rendering mirrored world

	//TODO : 1.15. Setup rasterizer state and view matrix for rendering the mirrored world

	//TODO : 1.16. Draw 3D objects of the mirrored scene - dodecahedron should be drawn with only one light and no colors and without blending

	//TODO : 1.17. Restore rasterizer state to it's original value

	//TODO : 1.37. Setup depth stencil state for rendering mirrored billboards
	
	//TODO : 1.38. Draw mirrored billboards - they need to be drawn after restoring rasterizer state, but with mirrored view matrix

	//TODO : 1.18. Restore view matrix to its original value

	//TODO : 1.25. Restore depth stencil state to it's original value
}

void ButterflyDemo::Render()
{
	Base::Render();

	//render mirrored worlds
	for (int i = 0; i < 12; ++i)
		DrawMirroredWorld(i);

	//render dodecahedron with one light and alpha blending
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	Set1Light();
	//TODO : 1.19. Comment the following line for now
	DrawDodecahedron(true);
	//TODO : 1.27. Uncomment the above line again
	m_device.context()->OMSetBlendState(nullptr, nullptr, BS_MASK);

	//render the rest of the scene with all lights
	Set3Lights();
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	DrawMoebiusStrip();
	DrawButterfly();
	m_device.context()->OMSetDepthStencilState(m_dssNoDepthWrite.get(), 0);
	DrawBillboards();
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}
#pragma endregion