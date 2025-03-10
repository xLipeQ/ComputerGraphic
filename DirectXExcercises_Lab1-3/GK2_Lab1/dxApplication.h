#pragma once
#include "windowApplication.h"
#include "dxDevice.h"
#include <DirectXMath.h>

class DxApplication : public mini::WindowApplication
{
private:
	float m_cameraAngle = 0.0f; // K¹t nachylenia kamery (RX)
	float m_cameraDistance = 10.0f; // Odleg³oœæ kamery od kostki (TZ)
	POINT m_lastMousePos; // Pozycja myszy w poprzedniej klatce
	bool m_leftMouseDown = false;
	bool m_rightMouseDown = false;

	LARGE_INTEGER m_frequency; // Czêstotliwoœæ licznika
	LARGE_INTEGER m_lastTime;  // Ostatni czas aktualizacji
	float m_deltaSeconds;      // Czas miêdzy klatkami
public:
	explicit DxApplication(HINSTANCE hInstance);

protected:
	int MainLoop() override;
	bool ProcessMessage(mini::WindowMessage& msg) override;

	float rotationX = 0.f;
	float rotationDeltaPerSecond = DirectX::XM_PIDIV4;
	
	void Tick(float deltaSeconds);

private:
	void Render();
	void Update();
	std::vector<DirectX::XMFLOAT2> CreateTriangleVertices();

	struct VertexPositionColor {
		DirectX::XMFLOAT3 position, color;
	};
	static std::vector<VertexPositionColor> CreateCubeVertices();
	static std::vector<unsigned short> CreateCubeIndices();
	mini::dx_ptr<ID3D11Buffer> m_indexBuffer;

	DirectX::XMFLOAT4X4 m_modelMtx2, m_modelMtx, m_viewMtx, m_projMtx;
	mini::dx_ptr<ID3D11Buffer> m_cbMVP;

	DxDevice m_device;
	mini::dx_ptr<ID3D11RenderTargetView> m_backBuffer;
	mini::dx_ptr<ID3D11DepthStencilView> m_depthBuffer;
	mini::dx_ptr<ID3D11Buffer> m_vertexBuffer;
	mini::dx_ptr<ID3D11VertexShader> m_vertexShader;
	mini::dx_ptr<ID3D11PixelShader> m_pixelShader;
	mini::dx_ptr<ID3D11InputLayout> m_layout;
};
