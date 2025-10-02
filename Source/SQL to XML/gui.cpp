#pragma once
#include "gui.h"
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <commdlg.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "Helper.h"
#include "resource.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter );

namespace gui 
{
	bool isRunning = true;
	HWND window = nullptr;
	WNDCLASSEX windowClass = {};
	POINTS position = {};
	PDIRECT3D9 d3d = nullptr;
	LPDIRECT3DDEVICE9 device = nullptr;
	D3DPRESENT_PARAMETERS presentParameters = {};
	Helper* helper = new Helper();
	std::vector<std::string> logBuffer = { "" };
}

long __stdcall WindowProcess( HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter )
{
	if ( ImGui_ImplWin32_WndProcHandler( window, message, wideParameter, longParameter ) )
		return true;
	switch ( message )
	{
		case WM_SIZE: 
		{
			if ( gui::device && wideParameter != SIZE_MINIMIZED )
			{
				gui::presentParameters.BackBufferWidth = LOWORD( longParameter );
				gui::presentParameters.BackBufferHeight = HIWORD( longParameter );
				gui::ResetDevice();
			}
		} return 0;
		case WM_SYSCOMMAND: 
		{
			if ( ( wideParameter & 0xfff0 ) == SC_KEYMENU )
				return 0;
		} break;
		case WM_DESTROY: 
		{
			PostQuitMessage( 0 );
		} return 0;
		case WM_LBUTTONDOWN: 
		{
			gui::position = MAKEPOINTS( longParameter );
		} return 0;
		case WM_MOUSEMOVE: 
		{
			if ( wideParameter == MK_LBUTTON )
			{
				const auto points = MAKEPOINTS( longParameter );
				auto rect = ::RECT{};
				GetWindowRect( gui::window, &rect );
				rect.left += points.x - gui::position.x;
				rect.top += points.y - gui::position.y;
				if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 && gui::position.y <= 19 )
					SetWindowPos( gui::window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER );
			}

		} return 0;
	}
	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow( const WCHAR* windowName ) noexcept
{
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA( 0 );
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = L"class001";
	windowClass.hIconSm = 0;
	RegisterClassEx( &windowClass );
	window = CreateWindowEx( 0, L"class001", windowName, WS_POPUP, 100, 100, WIDTH, HEIGHT, 0, 0, windowClass.hInstance, 0 );
	ShowWindow( window, SW_SHOWDEFAULT );
	UpdateWindow( window );
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow( window );
	UnregisterClass( windowClass.lpszClassName, windowClass.hInstance );
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9( D3D_SDK_VERSION) ;
	if ( !d3d )
		return false;
	ZeroMemory( &presentParameters, sizeof( presentParameters ) );
	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	if ( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &device ) < 0 )
		return false;
	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = device->Reset( &presentParameters );
	if ( result == D3DERR_INVALIDCALL )
		IM_ASSERT( 0 );
	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if ( device )
	{
		device->Release();
		device = nullptr;
	}
	if ( d3d )
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::SetupImGuiStyle() noexcept
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);
	colors[ImGuiCol_CloseButton] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
	colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
	colors[ImGuiCol_CloseButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);
	style.WindowRounding = 0.f;
	style.FrameRounding = 5.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.PopupRounding = 0.f;
	style.ScrollbarRounding = 5.0f;
	style.WindowPadding = ImVec2(10, 10);
	style.FramePadding = ImVec2(6, 4);
	style.ItemSpacing = ImVec2(8, 6);
	style.PopupBorderSize = 0.f;
}

void gui::InitFonts( float baseFontSize ) noexcept
{
    ImGuiIO& io = ImGui::GetIO();
    const char* fontPath = "Config/NotoSans-Regular.ttf";
    float iconFontSize = baseFontSize * 2.0f / 3.0f;
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false;
    const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseFull();
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF( fontPath, baseFontSize, &fontConfig, glyphRanges );
    if ( !mainFont )
        return;
    io.FontDefault = mainFont;
    io.Fonts->Build();
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
    InitFonts();
    ImGuiIO& io = ImGui::GetIO();
    SetupImGuiStyle();
	io.IniFilename = NULL;
	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX9_Init( device );
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while ( PeekMessage( &message, 0, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &message );
		DispatchMessage( &message );
		if ( message.message == WM_QUIT )
		{
			isRunning = !isRunning;
			return;
		}
	}
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();
	device->SetRenderState( D3DRS_ZENABLE, FALSE );
	device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	device->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
	device->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA( 0, 0, 0, 255 ), 1.0f, 0 );
	if ( device->BeginScene() >= 0 )
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData() );
		device->EndScene();
	}
	const auto result = device->Present( 0, 0, 0, 0 );
	if ( result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
		ResetDevice();
}

void gui::AddLog(const char* msg) noexcept
{
	logBuffer.push_back(msg);
}

void gui::Render() noexcept
{
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
    ImGui::Begin("Trickster Myshop SQL to XML Converter", &isRunning, &isRunning, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    static bool executed = false;
    if (!executed) 
	{
        AddLog("==========================================");
        AddLog("Trickster Myshop SQL to XML Converter");
        AddLog("==========================================");
        AddLog(" ");
        AddLog("Connecting to your SQL Server...");
		if (helper->connectDBFromDsnFile("config/database.dsn"))
		{
			AddLog("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
			helper->ExportAllGoods();
			AddLog("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
			AddLog("All done!");
			AddLog("libcmgds_e.xml was generated successfully!");
			AddLog("==========================================");
		}
        AddLog(" ");
        AddLog("You can close this window now c;");
        executed = true;
    }
    for (const auto& line : logBuffer)
        ImGui::TextUnformatted(line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}