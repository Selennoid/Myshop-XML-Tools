#define MI_MALLOC_OVERRIDE
#include <mimalloc-new-delete.h>
#include "gui.h"
#include <thread>

int __stdcall wWinMain( HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow )
{
	gui::CreateHWindow( L"Trickster Myshop SQL to XML Converter" );
	gui::CreateDevice();
	gui::CreateImGui();
	while ( gui::isRunning )
	{
		gui::BeginRender();
		gui::Render();
		gui::EndRender();
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();
	return EXIT_SUCCESS;
}