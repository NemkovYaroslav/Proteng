#pragma once
#include "includes.h"

using namespace DirectX::SimpleMath;

class DisplayWin32;
class RenderComponent;
class RenderShadowsComponent;
class GBuffer;

class RenderSystem
{
public:

	RenderSystem();

	void PrepareFrame();
	void Draw();
	void EndFrame();

	//void InitializeShader(std::string shaderFileName);

	void InitializeOpaqueShader(std::string shaderFileName);
	void InitializeLightingShader(std::string shaderFileName);
	void InitializeLightingShaderPoi(std::string shaderFileName);

	std::shared_ptr<D3D11_VIEWPORT>                viewport;
	Microsoft::WRL::ComPtr<ID3D11Device>           device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>    context;
	Microsoft::WRL::ComPtr<IDXGISwapChain>         swapChain;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>        backBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>        depthBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthView;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>     inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>    vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     pixelShader;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rastState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>    samplerState;

	ID3D11BlendState* blendStateOpaque; ///
	ID3D11BlendState* blendStateLight;  ///

	ID3D11DepthStencilState* dsOpaque;          ///
	ID3D11DepthStencilState* dsLightingLess;    ///
	ID3D11DepthStencilState* dsLightingGreater; ///

	ID3D11DepthStencilState* dsDirLight; ///

	ID3D11RasterizerState* rastCullBack;  ///
	ID3D11RasterizerState* rastCullFront; ///

	GBuffer* gBuffer; ///

	ID3D11InputLayout*  layoutOpaque;   ///
	ID3D11InputLayout*  layoutLighting; ///
	ID3D11InputLayout*  layoutLightingPoi; ///
	ID3D11VertexShader* vsOpaque;       ///
	ID3D11VertexShader* vsLighting;     ///
	ID3D11VertexShader* vsLightingPoi;     ///
	ID3D11PixelShader*  psOpaque;       ///
	ID3D11PixelShader*  psLighting;     ///
	ID3D11PixelShader*  psLightingPoi;     ///

	std::vector<RenderComponent*> renderComponents;
};