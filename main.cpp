
#include<cstdint>
#include <Windows.h>
#include <string>
#include <format>
#include <cassert>

#include <d3d12.h>
#pragma comment(lib,"d3d12.lib")
#include <dxgi1_6.h>
#pragma comment(lib,"dxgi.lib")
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include "MyMath.h"

//ImGui
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



struct Vector4
{
	float x, y, z, w;
};
///-----------------------------------------------///
//			ウィンドウプロシージャ					　//
///-----------------------------------------------///
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

///-----------------------------------------------///
//			ここまでウィンドウプロシージャ			//
///-----------------------------------------------///

///-----------------------------------------------///
//			ログを出せるようにする					//
///-----------------------------------------------///

//OutputDebugStirngA関数でstd::stringが利用できるようにラップしておく
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

// string -> wstring
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

//wstring -> string
std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

///-----------------------------------------------///
//			ここまでログを出せるようにする					//
///-----------------------------------------------///


///-----------------------------------------------///
//		DXCを使ってShaderをコンパイルする関数			//
///-----------------------------------------------///
IDxcBlob* CompileShader(
	//CompilerするShaderファイルへのパス
	const std::wstring& filePath,
	//Compilerに使用するProfile
	const wchar_t* profile,
	//初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler)
{
	///hlslファイルの内容をDXCの機能を利用して読み、コンパイラに渡すための設定をしていく
	//「これからシェーダーをコンパイルする」とログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));

	///1.hlslファイルを読み込む

	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら停止する
	assert(SUCCEEDED(hr));

	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知

	///2.Compileする

	LPCWSTR arguments[] = {
		filePath.c_str(),		//コンパイル対象のhlslファイル名
		L"-E",L"main",			//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile,			//ShaderProfileの設定
		L"-Zi",L"Qembed_debug"	//デバッグの情報を埋め込む	(L"-Qembed_debug"でエラー)
		L"-Od",					//最適化を外しておく
		L"-Zpr",				//メモリレイアウトは行優先
	};
	//実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,			// 読み込んだファイル
		arguments,			            // コンパイルオプション
		_countof(arguments),            // コンパイルオプションの数
		includeHandler,	            // includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)     // コンパイル結果
	);

	//コンパイルエラーではなくdxcが起動できないなど致命的な状況のとき停止
	assert(SUCCEEDED(hr));

	///3.警告・エラーが出ていないか確認する	
	//警告・エラーが出ていたらログに出して停止する

	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	///4.コンパイル結果を受け取って返す

	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;//Blob = Binary Large OBject(バイナリーデータの塊)
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succesed,path:{},profile:{}\n", filePath, profile)));
	//もう使わないリソースを開放
	shaderSource->Release();
	shaderResult->Release();

	//実行用のバイナリを返却
	return shaderBlob;
}



///-----------------------------------------------///
//		ここまでDXCを使ってShaderをコンパイルする			//
///-----------------------------------------------///

///-----------------------------------------------///
//				Resource作成の関数化					//
///-----------------------------------------------///

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;	// UploadHeapを使う

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	// バッファリソース。テクスチャの場合は別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;	// リソースのサイズ。

	//バッファの場合はこれらを１にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	//バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//実際に頂点リソースを作る
	ID3D12Resource* vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

///-----------------------------------------------///
//			DescriptorHeap作成の関数化			//
///-----------------------------------------------///
ID3D12DescriptorHeap* CreateDescroptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors, bool shaderVisible) {
	//ディスクリプイヒープの生成
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;								//レンダーターゲットビュー用
	descriptorHeapDesc.NumDescriptors = 2;													//バブルバッファ用に２つ。多くても構わない
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	// ディスクリプタヒープが作れなかったら起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}





///-----------------------------------------------///
//					メイン関数					　//
///-----------------------------------------------///

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {


	//
	// ウィンドウクラスを登録する
	//

	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;

	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);

	//
	// ウィンドウを生成する
	// 

	// クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//　ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//クライアント領域をもとに実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,		//利用するクラス名
		L"CG2",					//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//よく見るウィンドウスタイル
		CW_USEDEFAULT,			//表示X座標
		CW_USEDEFAULT,			//表示Y座標
		wrc.right - wrc.left,	//ウィンドウの横幅
		wrc.bottom - wrc.top,	//ウィンドウの縦幅
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		wc.hInstance,			//インスタンスハンドル
		nullptr);				//オプション

	//ウィンドウを表示する関数
	ShowWindow(hwnd, SW_SHOW);

	//
	///	debugLayer
	//
#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行うにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif




	//
	///　DXGIFactoryの生成
	//

	IDXGIFactory7* dxgiFactory = nullptr;
	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	//
	/// 使用するアダプタを決定する
	//

	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {

		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事

		//ソフトウェアアダプタでなければ採用する
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstring の方なので注意
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));


			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);


	//							
	///　D3D12Deviceの生成
	//							

	ID3D12Device* device = nullptr;
	//機能レベルとログの出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスを生成できたかを確認
		if (SUCCEEDED(hr)) {
			//生成できたのでログ出力を行ってループを抜ける
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}

	//デバイスの生成が上手くいかなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n ");//初期化完了のログを出す

	//
	///	エラー・警告が出た時止まるようにする
	//

#ifdef _DEBUG	//デバッグ時
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// ヤバイエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);


		//警告時に止まる
		/// 開放を忘れたことが判明した時、ここをコメントアウトしてログを確認することでどこを確認する
		///　ここがコメントアウトされていない場合、警告は出るが、どのオブジェクトが残っているかはわからない。
		/// 情報を得て修正したら必ずもとに戻して停止しないことを確認する！！
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);


		//解放
		infoQueue->Release();

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//Windows11のDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
		D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};

		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

	}
#endif



	//
	///	CommandQueueとCommandListを生成
	//

	//コマンドキューを生成
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成が上手くいかなかったら起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成が上手くいかなかったら起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	//コマンドリストの生成が上手くいかなかったら起動できない
	assert(SUCCEEDED(hr));


	//
	///　SwapChainを生成
	//

	//スワップチェーンを生成
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;									//画面の幅、ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;								//画面の高さ、ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;					//色の形式
	swapChainDesc.SampleDesc.Count = 1;									//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;										//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			//モニタにうつしたら、中身を破棄する
	//コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	//生成が上手くいかなかったら起動できない
	assert(SUCCEEDED(hr));

	//
	///　descriptorHeapを生成する
	//

	//RTVディスクリプタヒープの生成
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescroptorHeap(
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用のディスクリプタヒープの生成
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescroptorHeap(
		device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);


	//
	///　SwapChainからResourceを引っ張ってくる
	//

	//　SwapChainからResourceを引っ張ってくる
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//　うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	//
	///　RTV(レンダーターゲットビュー)を作る
	//

	//RTV（レンダーターゲットビュー）の設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;			//出力結果をSRGBに変換して書き込むf
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;		//2dテクスチャとして書き込む
	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	//まず1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	//2つ目のディスクリプタハンドルを得る（自力で）
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//2つ目を作る
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);


	//
	///　FenceとEventを生成
	//

	//初期値0でFanceを作る
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//FenecのSignalを持つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);


	//
	///DXCを初期化
	//

	//	dxcCompiler
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//includeに対応するための設定を作っておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	//
	///RootSignatureの生成
	//

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameter作成。複数設定できるので配列。今回はConstantBuffer1つ読むだけなので長さ１の配列
	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	//PS.hlslのregister（レジスタ）のb0のbと一致する
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
	//b0の0と一致する	b11と紐づけたいときは11を入れる
	rootParameters[0].Descriptor.ShaderRegister = 0;					//レジスタ番号0とバインド

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//PixelShaderで使う
	//b0の0と一致する	b11と紐づけたいときは11を入れる
	rootParameters[1].Descriptor.ShaderRegister = 0;					//レジスタ番号0とバインド
	descriptionRootSignature.pParameters = rootParameters;				//ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);	//配列の長さ


	//シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));


	// InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//Shaderをコンパイル
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl",
		L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	//
	///	PSOを生成
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;					// RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;					// InputLayout 
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize() };									// VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize() };										// PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;							// BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;				// RasterizerState

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ (形状)のタイプ。三角形 
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//
	/// VertexResourcesを生成する
	//

	ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(Vector4) * 3);

	//
	///VertexBufferviewを作成する
	//

	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//リリースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4);

	//
	/// Material用のResourceを作る
	//

	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意
	ID3D12Resource* materialResource =
		CreateBufferResource(device, sizeof(Vector4));
	//マテリアルデータに書き込む
	Vector4* materialData = nullptr;
	//書き込むためのアドレスを取得
	materialResource->
		Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は赤を書き込んでみる
	*materialData = Vector4(1.0f, 0.0f, 0.0f, 1.0f);


	//
	///	TransformationMatrix用のリソースを作る
	//

	//WVP用のリソースを作る、Matrix4x4　１つ分のサイズを用意する
	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
	//データを書き込む
	Matrix4x4* wvpData = nullptr;
	//書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//単位行列を書き込んでおく
	*wvpData = MakeIdentity4x4();

	//
	/// Resourceにデータを書き込む
	//

	// 頂点リソースにデータを書き込む
	Vector4* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 左下
	vertexData[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	// 上
	vertexData[1] = { 0.0f, 0.5f, 0.0f, 1.0f };
	// 右下
	vertexData[2] = { 0.5f, -0.5f, 0.0f, 1.0f };


	//
	///	ビューポート
	//

	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズを一緒にして画面全体を表示
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//
	///　シザー矩形
	//

	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が形成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;


	///ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);

	///宣言

	///Transforom変数を作る
	Vector3Transform transform{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	/// WorldViewProjectionMatrixを作る
	Vector3Transform cameraTransform{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,-5.0f}
	};

	///-----------------------------------------------///
	//					メインループ					　//
	///-----------------------------------------------///

	MSG msg{};

	//ウィンドウのxボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//Windowにメッセージが来てたら最優先で処理
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		} else {
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			
			///							//
			///		 ゲームの処理			///
			///							///

			
	

			//
			///画面をクリアする処理が含まれたコマンドリストを作る
			//

			//これから書き込むバックバッファのインデックスの取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			//
			/// TransitionBarierを張る処理
			//

			//トランジションバリア設定
			D3D12_RESOURCE_BARRIER barrier{};
			//　今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//バリアを張る対象のリソース。対称のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			//遷移前（現在）のResourcesState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//遷移後のResourcesState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			//
			/// TransitionBarierを張る処理ここまで
			//

			//描画先のRTVを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			// 指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };//青っぽい色。RGBAの順（KAMATAENGINEの色）
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			//	描画用のDesctiptorHeapの設定
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);
			


			//
			///描画に必要なコマンドを積む
			//

			commandList->RSSetViewports(1, &viewport);					//Viewportを設定
			commandList->RSSetScissorRects(1, &scissorRect);			//Scissorを設定

			// RootSignatureを設定。PSOに設定しているけど別途設定（PSOと同じもの）が必要
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicsPipelineState);		//PSOを設定
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);	//VBを設定

			// 形状を設定。PSOに設定しているものとはまた別。RootSignatureと同じように同じものを設定すると考えておけばいい
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// マテリアルのCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0,
				materialResource->GetGPUVirtualAddress());

			// wvp用のCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1,
				wvpResource->GetGPUVirtualAddress());

			// 描画（DrawCall／ドローコール）。３頂点で1つのインスタンス
			commandList->DrawInstanced(3, 1, 0, 0);

			ImGui::ShowDemoWindow();

			transform.rotate.y += 0.03f;
			Matrix4x4 worldMatrix = MakeAffineMatrix(
				transform.scale, transform.rotate, transform.translate
			);
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f,
				float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);

			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

			*wvpData = worldViewProjectionMatrix;



			ImGui::Render();

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			//	画面に描く処理はすべて終わり、画面に映すので、状態を遷移
			//	今回はRenerTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			//コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseすること
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			//
			///　コマンドをキックし、画面を交換。１フレームの処理を完了させる
			//

			//GPUにコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			//GPUとOSに画面の交換を行うよう通知する
			swapChain->Present(1, 0);


			///GPUにSignal（シグナル）を送る

			//Fenceの値を更新
			fenceValue++;
			//GPUがここまでたどり着いたときに、Fenceの値を指定した相対に代入するようにシグナルを送る
			commandQueue->Signal(fence, fenceValue);

			//Fenceの値が指定したSignal値にたどり着いているか確認する
			//GetCompleteValueの初期値はFence作成時に渡した初期値
			if (fence->GetCompletedValue() < fenceValue) {
				//指定したSignalにたどり着いていないので，たどり着くまで待つようにイベントを設定する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				//イベント待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			//次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));

		}
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//*-----------------------------------------------*//
	///					解放処理						///
	//*-----------------------------------------------*//

	///三角形を生成するのに必要なもの
	vertexResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob)
	{
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	materialResource->Release();
	wvpResource->Release();

	///
	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescriptorHeap->Release();
	srvDescriptorHeap->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();
#ifdef _DEBUG
	debugController->Release();
#endif // _DEBUG
	CloseWindow(hwnd);

	//リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();

	}

	return 0;

}
