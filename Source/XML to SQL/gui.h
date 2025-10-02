#pragma once
#include <d3d9.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

class Helper;

namespace gui
{
	constexpr float WIDTH = 600;
	constexpr float HEIGHT = 400;
	extern Helper* helper;
	extern bool isRunning;
	extern HWND window;
	extern WNDCLASSEX windowClass;
	extern POINTS position;
	extern PDIRECT3D9 d3d;
	extern LPDIRECT3DDEVICE9 device;
	extern D3DPRESENT_PARAMETERS presentParameters;
	extern std::vector<std::string> logBuffer;
	void CreateHWindow(const WCHAR* windowName) noexcept;
	void DestroyHWindow() noexcept;
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;
	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
	void SetupImGuiStyle() noexcept;
	void InitFonts( float baseFontSize = 14.0f ) noexcept;
	void AddLog(const char* msg) noexcept;
}
