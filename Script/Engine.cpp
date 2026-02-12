#include <Windows.h>
#include <d3d12.h>	// DirectX12関連の関数や構造体の定義に必要
#include <stdio.h>
#include <DirectXTex.h>
#include <directx/d3dx12.h>
#include "Engine.h"

Engine* engineInstance;

bool Engine::Init(HWND hwnd, UINT windowWidth, UINT windowHeight)
{
	frameBufferHeight_ = windowHeight;
	frameBufferWidth_ = windowWidth;
	hWnd_ = hwnd;

	// デバイスを生成
	if(!CreateDevice())
	{
		printf("描画エンジンの初期化に失敗");
		return false;
	}

	// コマンドキューを生成
	if (!CreateCommandQueue())
	{
		printf("コマンドキューの生成に失敗");
		return false;
	}

	// スワップチェインを生成
	if (!CreateSwapChain())
	{
		printf("スワップチェインの生成に失敗");
		return false;
	}

	// コマンドリストとコマンドアロケータを生成
	if (!CreateCommandList())
	{
		printf("コマンドリストの生成に失敗");
		return false;
	}

	// フェンスを生成
	if (!CreateFence())
	{
		printf("フェンスの生成に失敗");
		return false;
	}

	// ビューポートを生成
	CreateViewPort();

	// シザー矩形を生成
	CreateScissorRect();

	// レンダーターゲットを生成
	if (!CreateRenderTarget())
	{
		printf("レンダーターゲットの生成に失敗");
		return false;
	}

	// 初期化に成功
	printf("描画エンジンの初期化に成功\n");
    return true;
}

// D3DD12DeviceはGPUのデバイスのインターフェース
// DirectX12を使うにはこのデバイスが必要
bool Engine::CreateDevice()
{
	auto hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(pDevice_.ReleaseAndGetAddressOf())
	);
	return SUCCEEDED(hr);
}

// コマンドキューとは、デバイスに送るための描画コマンドの投稿、
// 描画コマンド実行の同期処理を行うもの
bool Engine::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	auto hr = pDevice_->CreateCommandQueue(&desc, IID_PPV_ARGS(pQueue_.ReleaseAndGetAddressOf()));

	return SUCCEEDED(hr);
}

// スワップチェインとは、ダブルバッファリングやトリプルバッファリングを実現するためのもの
// DIXGIとは、カーネルモードドライバーとシステムハードウェアと通信するためのAPIで、
// アプリケーションとハードウェアの間に挟まる概念
bool Engine::CreateSwapChain()
{
	// DXGIファクトリーの生成
	IDXGIFactory4* pFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
	{
		return false;
	}

	// スワップチェインの設定
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferDesc.Width = frameBufferWidth_;
	desc.BufferDesc.Height = frameBufferHeight_;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = FRAME_BUFFER_COUNT;
	desc.OutputWindow = hWnd_;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// スワップチェインの生成
	IDXGISwapChain* pSwapChain = nullptr;
	hr = pFactory->CreateSwapChain(
		pQueue_.Get(),
		&desc,
		&pSwapChain
	);
	if (FAILED(hr))
	{
		pFactory->Release();
		return false;
	}

	// IDXGISwapChain3を取得
	hr = pSwapChain->QueryInterface(IID_PPV_ARGS(pSwapChain_.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		pFactory->Release();
		pSwapChain->Release();
		return false;
	}

	// バックバッファ番号を取得
	currentBackBufferIndex_ = pSwapChain_->GetCurrentBackBufferIndex();

	pFactory->Release();
	pSwapChain->Release();
	return true;
}

// 描画命令を溜めておくのがコマンドリストで、コマンドリストを生成するにはコマンドアロケーターが必要
bool Engine::CreateCommandList()
{
	// コマンドアロケーターの作成
	HRESULT hr;
	for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = pDevice_->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(pAllocators_[i].ReleaseAndGetAddressOf()));
	}

	if (FAILED(hr))
	{
		return false;
	}

	// コマンドリストの生成
	hr = pDevice_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pAllocators_[currentBackBufferIndex_].Get(),
		nullptr,
		IID_PPV_ARGS(&pCommandList_)
	);

	if (FAILED(hr))
	{
		return false;
	}

	//コマンドリストは開かれている状態で作成されるので、いったん閉じる。
	pCommandList_->Close();

	return true;
}

// フェンスとは、GPUとCPUの同期を取るためのオブジェクト
// 描画が完了したかどうかを、フェンスの値がインクリメントされることで判断する
bool Engine::CreateFence()
{
	for (auto i = 0u; i < FRAME_BUFFER_COUNT; i++)
	{
		fenceValues_[i] = 0;
	}

	auto hr = pDevice_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pFence_.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	fenceValues_[currentBackBufferIndex_]++;

	//同期を行うときのイベントハンドラを作成する。
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	return fenceEvent_ != nullptr;
}

// ウィンドウに対してレンダリング結果をどう表示するかという設定
void Engine::CreateViewPort()
{
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.Width = static_cast<float>(frameBufferWidth_);
	viewport_.Height = static_cast<float>(frameBufferHeight_);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

// ビューポートに表示された画像のどこからどこまでを画面に映し出すかという設定
void Engine::CreateScissorRect()
{
	scissor_.left = 0;
	scissor_.right = frameBufferWidth_;
	scissor_.top = 0;
	scissor_.bottom = frameBufferHeight_;
}

// レンダーターゲットとは、キャンバスのようなもので、要は描画先のことである。
// レンダーターゲットの実態はバックバッファやテクスチャなどのリソース
bool Engine::CreateRenderTarget()
{
	// RTV用のディスクリプタヒープを作成する
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = FRAME_BUFFER_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = pDevice_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(pRtvHeap_.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// ディスクリプタのサイズを取得
	rtvDescriptorSize_ = pDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap_->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		pSwapChain_->GetBuffer(i, IID_PPV_ARGS(pRenderTargets_[i].ReleaseAndGetAddressOf()));
		pDevice_->CreateRenderTargetView(pRenderTargets_[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize_;
	}

	return true;
}