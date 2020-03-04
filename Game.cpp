//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
	const XMVECTORF32 START_POSITION = { 0.f, 0.f, -6.f, 0.f };
	const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };
	const float ROTATION_GAIN = 0.004f;
	const float MOVEMENT_GAIN = 0.07f;
}

Game::Game() noexcept :
	m_window(0),
	m_outputWidth(800),
	m_outputHeight(600),
	m_featureLevel(D3D_FEATURE_LEVEL_9_1),
	m_pitch(0),
	m_yaw(0)
{
	m_cameraPos = START_POSITION.v;
}

Game::~Game()
{
	if (m_audEngine)
	{
		m_audEngine->Suspend();
	}

	m_nightLoop.reset();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();

    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);

	m_keyboard = std::make_unique<Keyboard>();
	m_mouse = std::make_unique<Mouse>();
	m_mouse->SetWindow(window);

	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif
	m_audEngine = std::make_unique<AudioEngine>(eflags);
	m_retryAudio = false;

	m_ambient = std::make_unique<SoundEffect>(m_audEngine.get(),
		L"MountainKing.wav");
	m_nightLoop = m_ambient->CreateInstance();
	m_nightLoop->Play(true);

	nightVolume = 0.005f;
	nightSlide = 0.001f;
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.

	Vector3 eye(0.0f, 0.0f, 5.0f);
	Vector3 at(0.0f, 0.0f, 0.0f);
	m_hud_view = Matrix::CreateLookAt(eye, at, Vector3::UnitY);
	m_hud_world = Matrix::Identity;

	auto mouse = m_mouse->GetState();

	if (mouse.positionMode == Mouse::MODE_RELATIVE)
	{
		Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
			* ROTATION_GAIN;

		m_pitch -= delta.y;
		m_yaw -= delta.x;

		// limit pitch to straight up or straight down
		// with a little fudge-factor to avoid gimbal lock
		float limit = XM_PI / 2.0f - 0.01f;
		m_pitch = std::max(-limit, m_pitch);
		m_pitch = std::min(+limit, m_pitch);

		// keep longitude in sane range by wrapping
		if (m_yaw > XM_PI)
		{
			m_yaw -= XM_PI * 2.0f;
		}
		else if (m_yaw < -XM_PI)
		{
			m_yaw += XM_PI * 2.0f;
		}
	}

	m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
	auto kb = m_keyboard->GetState();
	if (kb.Escape)
	{
		ExitGame();
	}

	if (kb.Home)
	{
		m_cameraPos = START_POSITION.v;
		m_pitch = m_yaw = 0;
	}

	Vector3 move = Vector3::Zero;

	if (kb.Up || kb.W)
	{
		move.z += 1.f;
		if (nightVolume <= 1) {
			nightVolume += nightSlide;
		}
		else {
			nightVolume = 1;
		}
	}
	if (kb.Down || kb.S)
	{
		move.z -= 1.f;
		if (nightVolume > 0) {
			nightVolume -= nightSlide;
		}
		else {
			nightVolume = 0.001f;
		}
		
	}
	if (kb.Left || kb.A)
		move.x += 1.f;

	if (kb.Right || kb.D)
		move.x -= 1.f;

	if (kb.PageUp || kb.Space)
		move.y += 1.f;

	if (kb.PageDown || kb.X)
		move.y -= 1.f;

	Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.f);

	move = Vector3::Transform(move, q);

	move *= MOVEMENT_GAIN;

	m_cameraPos += move;

	Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f))
		- Vector3(0.1f, 0.1f, 0.1f);

	m_cameraPos = Vector3::Min(m_cameraPos, halfBound);
	m_cameraPos = Vector3::Max(m_cameraPos, -halfBound);

	float time = float(timer.GetTotalSeconds());

	m_skull_world = Matrix::CreateRotationY(cosf(time)*3.14f) * SimpleMath::Matrix::CreateTranslation(0.0f, -1.0f, 4.5f);
	m_earth_world = Matrix::CreateRotationY(time) * SimpleMath::Matrix::CreateTranslation(0.0f, -2.0f, 0.0f);
	
	if (rotation >= 360) {
		rotation = 0;
	}
	else {
		rotation++;
	}

	m_teapot_world = Matrix::CreateRotationZ(cosf(time) * 2.f) 
		* SimpleMath::Matrix::CreateTranslation(2.0f, -2.0f, 0.0f)
		* SimpleMath::Matrix::CreateRotationY(rotation * pi / 180);;
	m_em_effect->SetFresnelFactor(cosf(time * 2.f));

	if (m_retryAudio)
	{
		m_retryAudio = false;

		if (m_audEngine->Reset())
		{
			// TODO: restart any looped sounds here
			if (m_nightLoop)
				m_nightLoop->Play(true);
		}
	}

	m_nightLoop->SetVolume(nightVolume);

	elapsedTime;
}

// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // TODO: Add your rendering code here.
	
	float y = sinf(m_pitch);
	float r = cosf(m_pitch);
	float z = r * cosf(m_yaw);
	float x = r * sinf(m_yaw);

	XMVECTOR lookAt = m_cameraPos + Vector3(x, y, z);

	XMMATRIX view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);

	m_room->Draw(Matrix::Identity, view, m_proj, Colors::White, m_roomTex.Get());
	m_skull->Draw(m_d3dContext.Get(), *m_states, m_skull_world, view, m_proj);
	
	Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.f);
	m_skull->UpdateEffects([&](IEffect* effect)
		{
			auto lights = dynamic_cast<IEffectLights*>(effect);
			if (lights)
			{
				XMVECTOR dir = XMVector3Rotate(g_XMOne, q);
				lights->SetLightDirection(0, dir/2.f);
			}
			auto fog = dynamic_cast<IEffectFog*>(effect);
			if (fog)
			{
				fog->SetFogEnabled(true);
				fog->SetFogStart(0); // assuming RH coordiantes
				fog->SetFogEnd(12);
				fog->SetFogColor(Colors::Green);
			}
		});

	m_earth->Draw(m_earth_world, view, m_proj, Colors::White, m_earth_texture.Get());

	m_em_effect->SetView(view);
	m_em_effect->SetProjection(m_proj);
	m_em_effect->SetWorld(m_teapot_world);
	m_teapot->Draw(m_em_effect.get(), m_inputLayout.Get(), false, false, [=] {
		auto sampler = m_states->LinearWrap();
		m_d3dContext->PSSetSamplers(1, 1, &sampler);
		});

	m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_d3dContext->RSSetState(m_states->CullNone());
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

	//Arrays of geomtry data.
	const int vCount = 12;
	const int iCount = 12;
	uint16_t iArray[iCount];
	VertexPositionColor vArray[vCount];

	for (int i = 0; i < iCount; i++) {
		iArray[i] = i;
	}
	float time = float(m_timer.GetTotalSeconds());
	float t = cosf(time*2);
	vArray[0] = VertexPositionColor(Vector3(-0.5f, 0.5f, 0.5f)/t, Colors::GreenYellow);
	vArray[1] = VertexPositionColor(Vector3(-1.0f, 0.75f, 0.5f)/t, Colors::Green);
	vArray[2] = VertexPositionColor(Vector3(-0.75f, 1.f, 0.5f)/t, Colors::Green);

	vArray[3] = VertexPositionColor(Vector3(0.5f, 0.5f, 0.5f)/t, Colors::GreenYellow);
	vArray[4] = VertexPositionColor(Vector3(1.0f, 0.75f, 0.5f)/t, Colors::Green);
	vArray[5] = VertexPositionColor(Vector3(0.75f, 1.f, 0.5f)/t, Colors::Green);

	vArray[6] = VertexPositionColor(Vector3(-0.5f, -0.5f, 0.5f)/t, Colors::GreenYellow);
	vArray[7] = VertexPositionColor(Vector3(-1.0f, -0.75f, 0.5f)/t, Colors::Green);
	vArray[8] = VertexPositionColor(Vector3(-0.75f, -1.f, 0.5f)/t, Colors::Green);

	vArray[9] = VertexPositionColor(Vector3(0.5f, -0.5f, 0.5f)/t, Colors::GreenYellow);
	vArray[10] = VertexPositionColor(Vector3(1.0f, -0.75f, 0.5f)/t, Colors::Green);
	vArray[11] = VertexPositionColor(Vector3(0.75f, -1.f, 0.5f)/t, Colors::Green);

	m_batch->Begin();
	//apply loaded shader
	m_d3dContext->IASetInputLayout(m_layout);							//set the input layout for the shader to match out geometry
	SetShaderParameters(&m_hud_world, &m_hud_view, &m_proj);			//send the world, view and projection matrices into the shader
	m_d3dContext->VSSetShader(m_vertexShader.Get(), 0, 0);				//turn on vertex shader
	m_d3dContext->PSSetShader(m_pixelShader.Get(), 0, 0);				//turn on pixel shader

	m_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, iArray, iCount, vArray, vCount);

	m_batch->End();

	Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
	m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
	m_audEngine->Resume();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
        ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));


// TODO: Initialize device dependent objects here (independent of window size).
	
	auto vertexShaderBuffer = DX::ReadData(L"ui_vs.cso");
	DX::ThrowIfFailed(device->CreateVertexShader(vertexShaderBuffer.data(), vertexShaderBuffer.size(), NULL, &m_vertexShader));
	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	// Get a count of the elements in the layout.
	unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	// Create the vertex input layout.
	device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer.data(), vertexShaderBuffer.size(), &m_layout);
	auto pixelShaderBuffer = DX::ReadData(L"ui_ps.cso");		//Yes we are re-using the same buffer
	device->CreatePixelShader(pixelShaderBuffer.data(), pixelShaderBuffer.size(), NULL, &m_pixelShader);
	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	m_matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	m_matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	m_matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_matrixBufferDesc.MiscFlags = 0;
	m_matrixBufferDesc.StructureByteStride = 0;
	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	device->CreateBuffer(&m_matrixBufferDesc, NULL, &m_matrixBuffer);

	m_room = GeometricPrimitive::CreateBox(m_d3dContext.Get(),
		XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]),
		false, true);

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"roomtexture.dds",
			nullptr, m_roomTex.ReleaseAndGetAddressOf()));

	m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());
	m_fxFactory = std::make_unique<EffectFactory>(m_d3dDevice.Get());
	m_skull = Model::CreateFromSDKMESH(m_d3dDevice.Get(), L"skull.sdkmesh", *m_fxFactory);
	m_skull_world = Matrix::Identity;
	m_earth_world = Matrix::Identity;
	
	m_effect = std::make_unique<BasicEffect>(m_d3dDevice.Get());
	m_effect->SetVertexColorEnabled(true);

	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateInputLayout(VertexPositionColor::InputElements,
			VertexPositionColor::InputElementCount,
			shaderByteCode, byteCodeLength,
			m_inputLayout.ReleaseAndGetAddressOf()));
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_d3dContext.Get());

	m_earth = GeometricPrimitive::CreateSphere(m_d3dContext.Get());

	m_earth_effect = std::make_unique<BasicEffect>(m_d3dDevice.Get());
	m_earth_effect->SetTextureEnabled(true);
	m_earth_effect->SetPerPixelLighting(true);
	m_earth_effect->SetLightingEnabled(true);
	m_earth_effect->SetLightEnabled(0, true);
	m_earth_effect->SetLightDiffuseColor(0, Colors::White);
	m_earth_effect->SetLightDirection(0, Vector3::UnitZ);

	m_earth = GeometricPrimitive::CreateSphere(m_d3dContext.Get());
	m_earth->CreateInputLayout(m_earth_effect.get(),
		m_inputLayout.ReleaseAndGetAddressOf());

	DX::ThrowIfFailed(
		CreateWICTextureFromFile(m_d3dDevice.Get(), L"earth.bmp", nullptr,
			m_earth_texture.ReleaseAndGetAddressOf()));

	m_earth_effect->SetTexture(m_earth_texture.Get());


	m_em_effect = std::make_unique<EnvironmentMapEffect>(m_d3dDevice.Get());
	m_em_effect->EnableDefaultLighting();

	m_teapot = GeometricPrimitive::CreateTeapot(m_d3dContext.Get());
	m_teapot->CreateInputLayout(m_em_effect.get(),
		m_inputLayout.ReleaseAndGetAddressOf());
	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"porcelain.dds", nullptr,
			m_teapot_texture.ReleaseAndGetAddressOf()));
	m_em_effect->SetTexture(m_teapot_texture.Get());
	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"cubemap.dds", nullptr,
			m_cubemap.ReleaseAndGetAddressOf()));
	m_em_effect->SetEnvironmentMap(m_cubemap.Get());
	m_teapot_world = Matrix::Identity;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
            ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.
	m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f),
		float(backBufferWidth) / float(backBufferHeight), 0.01f, 100.f);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
	m_room.reset();
	m_roomTex.Reset();

	m_vertexShader.Reset();
	m_pixelShader.Reset();

	m_states.reset();
	m_fxFactory.reset();
	m_skull.reset();
	m_earth.reset();

	m_effect.reset();
	m_earth_effect.reset();
	m_batch.reset();
	m_inputLayout.Reset();

	m_earth_texture.Reset();

	m_teapot.reset();
	m_em_effect.reset();
	m_teapot_texture.Reset();
	m_cubemap.Reset();
    CreateDevice();

    CreateResources();
}

void Game::SetShaderParameters(DirectX::SimpleMath::Matrix* world, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection)
{

	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	unsigned int bufferNumber;
	DirectX::SimpleMath::Matrix  tworld, tview, tproj;
	auto context = m_d3dContext;
	auto device = m_d3dDevice;
	tworld = world->Transpose();
	tview = view->Transpose();
	tproj = projection->Transpose();

	// Lock the constant buffer so it can be written to.
	result = context->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;

	// Unlock the constant buffer.
	context->Unmap(m_matrixBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Now set the constant buffer in the vertex shader with the updated values.
	context->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
}