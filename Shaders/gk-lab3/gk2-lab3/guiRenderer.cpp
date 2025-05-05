#include "guiRenderer.h"
#include "dxDevice.h"
#include "imgui.h"
#include "window.h"
#include <windowsx.h>
#include "exceptions.h"

using namespace DirectX;
using namespace mini;
using namespace gk2;
using namespace directx;
using namespace utils;

static constexpr D3D11_INPUT_ELEMENT_DESC elements[3] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

GUIRenderer::GUIRenderer(const DxDevice& device, const windows::window& w)
	: m_vertexCount(0), m_indexCount(0)
{
	auto vs = device.LoadByteCode(L"guiVS.cso");
	m_vs = device.CreateVertexShader(vs);
	auto ps = device.LoadByteCode(L"guiPS.cso");
	m_ps = device.CreatePixelShader(ps);
	m_layout = device.CreateInputLayout(elements, vs);

	auto wndSize = w.client_size();
	XMFLOAT4X4 projMtx = { 2.0f / wndSize.cx,0.0f,0.0f,0.0f,
		0.0f, 2.0f / -wndSize.cy, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		-1.0f, 1.0f, 0.5f, 1.0f };
	m_cbProj = device.CreateConstantBuffer<XMFLOAT4X4>();
	m_cbProj.Update(device.context(), projMtx);
	m_bs = device.CreateBlendState(blend_info::alpha_blend());
	rasterizer_info rsDesc;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.ScissorEnable = true;
	m_rs = device.CreateRasterizerState(rsDesc);
	depth_stencil_info dssDesc;
	dssDesc.DepthEnable = false;
	dssDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	m_dss = device.CreateDepthStencilState(dssDesc);
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	tex2d_info texDesc(width, height);
	texDesc.MipLevels = 1;
	subresource_data texData;
	texData.SysMemPitch = width * 4;
	texData.pSysMem = pixels;
	auto tex = device.CreateTexture(texDesc, texData);
	shader_resource_view_info texViewDesc = shader_resource_view_info::tex2d_view();
	texViewDesc.Texture2D.MipLevels = 1;
	m_fontTexture = device.CreateShaderResourceView(tex, texViewDesc);
	io.Fonts->TexID = m_fontTexture.get();
	sampler_info sDesc;
	sDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sDesc.MinLOD = sDesc.MaxLOD = 0;
	sDesc.MaxAnisotropy = 0;
	sDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	m_sampler = device.CreateSamplerState(sDesc);

	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';
	io.ImeWindowHandle = w.handle();
	io.DisplaySize = { static_cast<float>(wndSize.cx), static_cast<float>(wndSize.cy) };
}

GUIRenderer::~GUIRenderer()
{
	ImGui::DestroyContext();
}

std::optional<LRESULT> GUIRenderer::ProcessMessage(windows::message const &msg)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (msg.type)
	{
	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		break;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		break;
	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		break;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		break;
	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true;
		break;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		break;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(msg.w_param) > 0 ? +1.0f : -1.0f;
		break;
	case WM_MOUSEMOVE:
		io.MousePos.x = static_cast<float>(GET_X_LPARAM(msg.l_param));
		io.MousePos.y = static_cast<float>(GET_Y_LPARAM(msg.l_param));
		break;
	case WM_KEYDOWN:
		if (msg.w_param < 256)
			io.KeysDown[msg.w_param] = true;
		break;
	case WM_KEYUP:
		if (msg.w_param < 256)
			io.KeysDown[msg.w_param] = false;
		break;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (msg.w_param > 0 && msg.w_param < 0x10000)
			io.AddInputCharacter(static_cast<unsigned short>(msg.w_param));
		break;
	default:
		return std::nullopt;
	}
	return 0;
}

void GUIRenderer::Update(float dt)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = dt;
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;
	ImGui::NewFrame();
}

void GUIRenderer::Render(const DxDevice& device)
{
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (!m_vertexBuffer || m_vertexCount < draw_data->TotalVtxCount)
	{
		m_vertexCount = draw_data->TotalVtxCount + 5000;
		m_vertexBuffer = device.CreateVertexBuffer<ImDrawVert>(m_vertexCount);
	}
	if (!m_indexBuffer || m_indexCount < draw_data->TotalIdxCount)
	{
		m_indexCount = draw_data->TotalIdxCount + 10000;
		m_indexBuffer = device.CreateIndexBuffer<ImDrawIdx>(m_indexCount);
	}
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	auto hr = device.context()->Map(m_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource);
	if (FAILED(hr))
		throw winapi_error{ hr };
	hr = device.context()->Map(m_indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource);
	if (FAILED(hr))
		throw winapi_error{ hr };
	ImDrawVert* vtx_dst = reinterpret_cast<ImDrawVert*>(vtx_resource.pData);
	ImDrawIdx* idx_dst = reinterpret_cast<ImDrawIdx*>(idx_resource.pData);
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	device.context()->Unmap(m_vertexBuffer.get(), 0);
	device.context()->Unmap(m_indexBuffer.get(), 0);

	device.context()->IASetInputLayout(m_layout.get());
	ID3D11Buffer* tmp = m_vertexBuffer.get();
	unsigned stride = sizeof(ImDrawVert), offset = 0;
	device.context()->IASetVertexBuffers(0, 1, &tmp, &stride, &offset);
	constexpr auto format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	device.context()->IASetIndexBuffer(m_indexBuffer.get(), format, 0);
	device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device.context()->VSSetShader(m_vs.get(), nullptr, 0);
	tmp = m_cbProj;
	device.context()->VSSetConstantBuffers(0, 1, &tmp);
	device.context()->PSSetShader(m_ps.get(), nullptr, 0);
	ID3D11SamplerState* tmps = m_sampler.get();
	device.context()->PSSetSamplers(0, 1, &tmps);
	device.context()->OMSetBlendState(m_bs.get(), nullptr, 0xffffffff);
	device.context()->OMSetDepthStencilState(m_dss.get(), 0);
	device.context()->RSSetState(m_rs.get());

	int vtx_offset = 0;
	int idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				const D3D11_RECT r = { static_cast<LONG>(pcmd->ClipRect.x), static_cast<LONG>(pcmd->ClipRect.y),
					static_cast<LONG>(pcmd->ClipRect.z), static_cast<LONG>(pcmd->ClipRect.w) };
				device.context()->PSSetShaderResources(0, 1, reinterpret_cast<ID3D11ShaderResourceView* const*>(&pcmd->TextureId));
				device.context()->RSSetScissorRects(1, &r);
				device.context()->DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
			}
			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.Size;
	}
	device.context()->ClearState();
}
