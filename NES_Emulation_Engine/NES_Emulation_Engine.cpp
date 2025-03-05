// NES_Emulation_Engine.cpp : 'main' function -> Program Execution Entry Point.

#include <iostream>
#include <crtdbg.h>

#include "CPU/R6502.h"
#include "NES_Emulation_Engine.h"
//#include "Utilities/Disassembler.h"

// For Timing and other clock stuff
// https://stackoverflow.com/questions/72403979/writing-a-clockloop-that-is-triggered-in-the-mhz-consistently-and-effeciently

using namespace NES;

#if _WIN64

#include <Windows.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Disable Rarely Used Windows API/Stuff
#endif // !WIN32_LEAN_AND_MEAN

// Forward Declarations
void attach_console();
void test();

/// Window Code ///
HWND window{ nullptr };
WNDCLASSEX wc;
HINSTANCE hInst;

// About Box Window Procedure
INT_PTR CALLBACK about_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Main Window Procedure
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	PAINTSTRUCT ps;
	HDC hdc;
	LPCWSTR greeting = L"Hello, Windows desktop!";

	switch (msg) {
	case WM_COMMAND:
			{
				int wmId = LOWORD(wparam);
				// Parse the menu selections:
				switch (wmId)
				{
				case ID_HELP_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, about_proc); 
					// Creates a new dialog box using application instace, int resource of the dialog box, 
					// window handle of parent and the dialog box's window procedure.
					break;
				case ID_EXIT:
					DestroyWindow(hwnd);
					break;
				default:
					return DefWindowProc(hwnd, msg, wparam, lparam);
				}
			}
		break;

	case WM_PAINT: // The application receives a WM_PAINT mssg when part of its displayed window must be updated. [lazy message] WM_SIZE comes first
		{
			// prepares for drawing -> returns a handle to the display device context used for drawing in the client area
			// BeginPaint() fills in the PaintStruct structure with information upon repaint request.
			hdc = BeginPaint(hwnd, &ps);

			// The FillRect function is part of the Graphics Device Interface(GDI), which has powered
			// Windows graphics for a very long time. In Windows 7, Microsoft introduced a new
			// graphics engine, named Direct2D, which supports high - performance graphics
			// operations, such as hardware acceleration. Direct2D is also available for Windows Vista
			// through the Platform Update for Windows Vista and for Windows Server 2008 through
			// the Platform Update for Windows Server 2008. (GDI is still fully supported.)

			// Save the original object 
			HGDIOBJ original = NULL;
			original = SelectObject(ps.hdc, GetStockObject(DC_PEN));


			// Create a pen.             
			HPEN blackPen = CreatePen(PS_SOLID, 1, 0);
			// Select the pen. 
			SelectObject(ps.hdc, blackPen);

			// Draw a rectangle. 
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1)); // uses a logical brush
			Rectangle(ps.hdc, 5, 5, 6, 6); // uses a pen
			DeleteObject(blackPen);
			// Restore the original object 
			SelectObject(ps.hdc, original);

			//TextOutW(hdc, 5, 5, greeting, wcslen(greeting)); // wcslen -> strlen for wide-string

			// End application-specific layout section.
			EndPaint(hwnd, &ps); // completes drawing -> ends the paint request and releases the device context.
		}
		break;

	case WM_SIZE: // Resize window
		// Event Not Handled RN
		break;

	case WM_CLOSE: DestroyWindow(hwnd); return 0;
	// DefWindowProc executes the default action for any window message.
	// In the case of WM_CLOSE, DefWindowProc automatically calls DestroyWindow.

	case WM_DESTROY: PostQuitMessage(0); return 0; // when the window is closed. | WM_CREATE is sent when a window is first created. |

	default: break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam); // Returns default window procedure
}

// wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEST));
// wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
// wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
// wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

// Register Window Class
int register_win32() {
	ZeroMemory(&wc, sizeof(wc)); // Fills a block of memory with zeros.

	wc.cbSize = sizeof(wc); // ??
	wc.style = CS_HREDRAW | CS_VREDRAW; // Redraws the entire window if a movement or size adjustment of the client area occurs -> expensive || https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
	wc.lpfnWndProc = window_proc; // Assigns our window procedure function
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	// HWINSTANCE -> handle to identify your application for others WINAPI calls. But actually, it is not even to identify your application from other instances, but to identify it from others applications executable files 
	//				 inside your applications, like DLLs (a DLL inside your app will have its own HINSTANCE, usually given as a HMODULE, which is the same). If you run your program twice, the HINSTANCE may be the same for both.
	//				 As a side note, HINSTANCE is actually a pointer to the memory image of the executable file. Therefore you can do printf("%s\n",hInstance);, and it will always print MZ? (? depends on your locale), 
	//				 because a windows executable file always starts with "MZ\x90\x00". - ElderBug | StackOverflow
	wc.hInstance = hInst;
	wc.hInstance = 0; // https://devblogs.microsoft.com/oldnewthing/20050418-59/?p=35873 

	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);		// Default
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);	// Default
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		// Default

	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Black Background

	wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU); // Assigns the menu created in the resource script
	wc.lpszClassName = L"NESWin32Window";

	// Register Window Class -> why do we register class?
	// -> you register it with Windows so that it knows about your window and how to send messages to it.
	RegisterClassEx(&wc);

	return 1;
}

// Create a Window
int create_win32(HINSTANCE hInstance, int nCmdShow, int width, int height) {
	
	//RECT rect{};
	//GetWindowRect(parent, &rect);
	//
	//// Adjust window size to current device size
	//AdjustWindowRect(&rect, WS_CHILD, FALSE);

	//const int top{ rect.top };
	//const int left{ rect.left };
	//const int init_width{ rect.right - left };
	//const int init_height{ rect.bottom - top };

	const wchar_t* caption{ L"NES Emulator" };

	// Create an instance of window class
	window = CreateWindowEx(
		0,											// Extended Style
		wc.lpszClassName,							// Window Class Name
		caption,									// Instance Title
		WS_OVERLAPPEDWINDOW,						// Window Style
		CW_USEDEFAULT, CW_USEDEFAULT,				// Initial Window Left, Top Position
		width, height,								// Initial Window Width, Height
		NULL,										// Handle to Parent [HWND]
		NULL, NULL, NULL							// Handle to Menu, Instance of this Application, Extra Creation Parameters [LPARAM] [TYPE: void*]
	);

	if (window == NULL) {
		return 0;
	}

	ShowWindow(window, nCmdShow);
	UpdateWindow(window);

	return 1;
}

// Subsystem Windows: Entry Point
int WINAPI WinMain(_In_ HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Google it, dammit
	attach_console();
#endif

	hInst = hInstance;

	if (!register_win32()) return 0;

	if (create_win32(hInstance, SW_SHOWNORMAL, SCREEN_WIDTH*RENDER_SCALE_MULTIPLIER, SCREEN_HEIGHT*RENDER_SCALE_MULTIPLIER)) { 
		MSG msg{};
		bool is_running{ true };

		PAINTSTRUCT ps;
		HDC hdc;
		
		// TODO: Read on Keyboard Accelerator Tables: https://learn.microsoft.com/en-us/windows/win32/learnwin32/accelerator-tables
		// HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));
		// if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))

		test(); // CPU/PPU TEST

		while (is_running) {
			// Engine's update function

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				// This loop reads, removes and dispatches messages from the mssg queue, till there are no messages left to process.
				TranslateMessage(&msg);
				DispatchMessage(&msg); // passed onto the window

				is_running &= (msg.message != WM_QUIT); // If a quit signal is not sent, the loop will continue
			}

			// Run CPU and PPU tasks -> does CPU have to wait for PPU to complete 3 cycles
			// TODO: call clock

			hdc = BeginPaint(window, &ps); // Begin Drawing on the Window

			// i. Clear Window?
			// ii. Display the render from PPU
			for (int x = 0; x < SCREEN_HEIGHT; ++x) { // Each Scanline
				for (int y = 0; y < SCREEN_WIDTH; ++y) { // Each Pixel
					// TODO: Complete implementation:
					// if set, get brush/colour
					// create a rect/rectangle
					// multiply position with window size multiplier
					// fill the rect/rectangle
				}
			}

			EndPaint(window, &ps); // completes drawing -> ends the paint request and releases the device context.

			// TODO: find how to update the window title
			
			// TODO: set the timing circuit code for the cpu/ppu clock
		}
	}

	return 1;
}

void attach_console() {
	// create a separate new console window
	AllocConsole();

	// attach the new console to this application's process
	AttachConsole(GetCurrentProcessId());

	// reopen the std I/O streams to redirect I/O to the new console
	FILE* stream;
	freopen_s(&stream, "CON", "w", stdout);
	freopen_s(&stream, "CON", "w", stderr);
	freopen_s(&stream, "CON", "r", stdin);
}

void test() {
	std::cout << "Initializing!\n\n";

	CPU::R6502* Cpu = new CPU::R6502();

	Cpu->SetBus(new CPU::Bus());
	Cpu->reset();

#if CPU_TEST
	Cpu->set_instructions_count(88);

	for (; Cpu->get_instructions_count() > 0;) {
		Cpu->clock();
	}
	Cpu->DisassembleRAM(0, 40);
#else

#endif // CPU_TEST

	delete Cpu;

	std::cout << "Done...\n Press Any Key To Continue! \n";
	//getchar();
}

#else

int main()
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Google it, dammit
#endif

    std::cout << "Initializing!\n\n";

    CPU::R6502* Cpu = new CPU::R6502();

    Cpu->SetBus(new CPU::Bus());
    Cpu->reset();

#if CPU_TEST
    Cpu->set_instructions_count(88);

    for (; Cpu->get_instructions_count() > 0;) {
        Cpu->clock();
    }
    Cpu->DisassembleRAM(0, 40);
#else

#endif // CPU_TEST

    delete Cpu;

    std::cout << "Done...\n Press Any Key To Continue! \n";
    getchar();
}

#endif //_WIN64

