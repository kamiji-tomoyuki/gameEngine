#include "WinApp.h"
#ifdef _DEBUG
#include"imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _DEBUG

#pragma comment(lib,"winmm.lib")

WinApp* WinApp::instance = nullptr;

// ウィンドウプロシージャ
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef _DEBUG
	// ImGui用ウィンドウプロシージャ呼び出し
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif // _DEBUG
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

WinApp* WinApp::GetInstance()
{
	if (instance == nullptr) {
		instance = new WinApp();
	}
	return instance;
}

void WinApp::Initialize()
{
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名
	wc.lpszClassName = L"WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	// クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);

	//ウィンドウの生成
	hwnd = CreateWindow(
		wc.lpszClassName,      	 // 利用するクラス名
		L"myEngine",			 // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	 // よく見るウィンドウスタイル
		CW_USEDEFAULT,			 // 表示X座標
		CW_USEDEFAULT,			 // 表示Y座標
		wrc.right - wrc.left,	 // ウィンドウ横幅
		wrc.bottom - wrc.top,	 // ウィンドウ縦幅
		nullptr,				 // 親ウィンドウハンドル
		nullptr,				 // メニューハンドル
		wc.hInstance,			 // インスタンスハンドル
		nullptr);				 // オプション

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

}

void WinApp::Update()
{

}

void WinApp::Finalize()
{
	CoUninitialize();
	CloseWindow(hwnd);
	delete instance;
	instance = nullptr;
}

bool WinApp::ProcessMessage()
{
	MSG msg{};

	//Windowにメッセージが来てたら最優先で処理させる
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//ウィンドウの×ボタンが押されるまでループ
	if (msg.message == WM_QUIT) {
		return true;
	}

	return false;
}
