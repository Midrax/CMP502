//
// Game.h
//

#pragma once

#include "StepTimer.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
{
public:

    Game() noexcept;
	~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

	// Audio
	void OnNewAudioDevice() { m_retryAudio = true; }

private:
	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};

	float pi = 3.14159265359f;
	float rotation;


    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

	void SetShaderParameters(DirectX::SimpleMath::Matrix* world, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection);

    // Device resources.
    HWND												m_window;
    int													m_outputWidth;
    int													m_outputHeight;

    D3D_FEATURE_LEVEL									m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>				m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>		m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>				m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer				                        m_timer;
	// Input
	std::unique_ptr<DirectX::Keyboard>					m_keyboard;
	std::unique_ptr<DirectX::Mouse>						m_mouse;
	// Room
	std::unique_ptr<DirectX::GeometricPrimitive>		m_room;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_roomTex;
	// Camera
	DirectX::SimpleMath::Vector3						m_cameraPos;
	float												m_pitch;
	float												m_yaw;
	DirectX::SimpleMath::Matrix							m_view;
	DirectX::SimpleMath::Matrix							m_proj;

	DirectX::SimpleMath::Matrix							m_hud_world;
	DirectX::SimpleMath::Matrix							m_hud_view;
	// Shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShader;
	ID3D11Buffer*										m_matrixBuffer;
	D3D11_BUFFER_DESC									m_matrixBufferDesc;
	ID3D11InputLayout*									m_layout;
	// Custom Geometry
	std::unique_ptr<DirectX::CommonStates>									m_states;
	std::unique_ptr<DirectX::BasicEffect>									m_effect;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>	m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>								m_inputLayout;
	// Loading Meshes
	std::unique_ptr<DirectX::Model>						m_skull;
	DirectX::SimpleMath::Matrix							m_skull_world;
	std::unique_ptr<DirectX::IEffectFactory>			m_fxFactory;

	std::unique_ptr<DirectX::GeometricPrimitive>		m_earth;
	DirectX::SimpleMath::Matrix							m_earth_world;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_earth_texture;
	std::unique_ptr<DirectX::BasicEffect>				m_earth_effect;

	std::unique_ptr<DirectX::GeometricPrimitive>		m_teapot;
	DirectX::SimpleMath::Matrix							m_teapot_world;
	std::unique_ptr<DirectX::EnvironmentMapEffect>		m_em_effect;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_teapot_texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_cubemap;

	std::unique_ptr<DirectX::AudioEngine>				m_audEngine;
	bool												m_retryAudio;
	std::unique_ptr<DirectX::SoundEffect>				m_ambient;
	std::unique_ptr<DirectX::SoundEffectInstance>		m_nightLoop;
	float												nightVolume;
	float												nightSlide;
};
