#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include "ComPtr.h"

#pragma comment(lib, "d3d12.lib") // d3d12ライブラリをリンクする
#pragma comment(lib, "dxgi.lib") // dxgiライブラリをリンクする

class Engine
{
public:

	enum 
	{
		FRAME_BUFFER_COUNT = 2, // ダブルバッファリング
	};

public:

	// 初期化
	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight);

	// 描画開始
	void DrawBegin();

	// 描画終了
	void DrawEnd();

	// 外部に情報を渡すもの
	ID3D12Device6* Device();
	ID3D12GraphicsCommandList* CommandList();
	UINT CurrentBackBufferIndex();

private:

	// 描画に使うDirectX12のオブジェクトたち----------------------------------------------
	HWND hWnd_;
	UINT frameBufferWidth_;
	UINT frameBufferHeight_;
	UINT currentBackBufferIndex_;

	// デバイス
	ComPtr<ID3D12Device6> pDevice_;

	// コマンドキュー
	ComPtr<ID3D12CommandQueue> pQueue_;

	// スワップチェイン
	ComPtr<IDXGISwapChain3> pSwapChain_;

	// コマンドアロケータ
	ComPtr<ID3D12CommandAllocator> pAllocators_[FRAME_BUFFER_COUNT];

	// コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> pCommandList_;

	// フェンス
	ComPtr<ID3D12Fence> pFence_;

	// フェンスで使うイベント
	HANDLE fenceEvent_;

	// フェンスの値
	UINT64 fenceValues_[FRAME_BUFFER_COUNT];	// ダブルバッファー用に2個

	// ビューポート情報
	D3D12_VIEWPORT viewport_;

	// シザー矩形情報
	D3D12_RECT scissor_;

	// デバイスを生成
	bool CreateDevice();

	// コマンドキューを生成
	bool CreateCommandQueue();

	// スワップチェインを生成
	bool CreateSwapChain();

	// コマンドリストとコマンドアロケータを生成
	bool CreateCommandList();

	// フェンスを生成
	bool CreateFence();

	// ビューポートを生成
	void CreateViewPort();

	// シザー矩形を生成
	void CreateScissorRect();

	// 描画に使うオブジェクトとその生成関数----------------------------------------------
	// レンダーターゲットビューのディスクリプタサイズ
	UINT rtvDescriptorSize_;

	// レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> pRtvHeap_;

	// レンダーターゲット（ダブルバッファリング用に2個）
	ComPtr<ID3D12Resource> pRenderTargets_[FRAME_BUFFER_COUNT];

	// 深度ステンシルのディスクリプターサイズ
	UINT dsvDescriptorSize_;

	// 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> pDsvHeap_;

	// 深度ステンシルバッファ（これは1つで良い）
	ComPtr<ID3D12Resource> pDepthStencilBuffer_;	
	
	// レンダーターゲットを生成
	bool CreateRenderTarget();
	
	// 深度ステンシルバッファを生成
	bool CreateDepthStencil();

	// 描画ループで使用するもの----------------------------------------------
	// 現在のフレームのレンダーターゲットを一時的に保存しておく関数
	ID3D12Resource* currentRenderTarget_;

	// 描画完了を待つ処理
	void WaitRender();
};

// どこからでも参照出来るようグローバルにする
extern Engine* engineInstance; 
