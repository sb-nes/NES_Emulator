// NES_Emulation_Engine.cpp : 'main' function -> Program Execution Entry Point.

#include <iostream>
#include <crtdbg.h>

#include "CPU/R6502.h"
#include "NES_Emulation_Engine.h"
//#include "Utilities/Disassembler.h"

// For Timing and other clock stuff
// https://stackoverflow.com/questions/72403979/writing-a-clockloop-that-is-triggered-in-the-mhz-consistently-and-effeciently

using namespace NES;

CPU::R6502*				_nes_instance;
pattern_table			_table1;
pattern_table			_table2;
palette					_palette;
int						_count{ 0 };

#if _WIN64

#include <Windows.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Disable Rarely Used Windows API/Stuff
#endif // !WIN32_LEAN_AND_MEAN

// Forward Declarations
void attach_console();
void test();
bool initialize();
void print_cpu_status();
void print_acuumulator();
void print_x_register();
void print_y_register();
void print_stack_pointer();
void print_program_counter();

void print_status_value(u8 value, int x_pos, int y_pos, u8 scale, int value_count = 0, u32 colour = 0x00FFFFFF, u32 bg_colour = 0x00000000 , int left = 10);
void print_hex_value(u8 value, int x_pos, int y_pos, u8 scale, int value_count = 0, u32 colour = 0x00FFFFFF, u32 bg_colour = 0x00000000, int left = 10);

/// Window Code ///
HWND window{ nullptr };
WNDCLASSEX wc;
HINSTANCE hInst;

// Pixel Array Code
static BITMAPINFO		_frame_bitmap_info; // to tell GDI about our pixel format
static HBITMAP			_frame_bitmap = 0; // bitmap handle to hold/encapsulate info and array data
static HDC				_frame_device_context = 0;

struct {
	int		width;
	int		height;
	u32*	pixels;
} _frame = { 0 };

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
			static PAINTSTRUCT ps;
			static HDC hdc;

			hdc = BeginPaint(hwnd, &ps);

			// Paint the Rendered Frame to the window | any edits to the render frame should be done in the main function loop
			BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
				   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
				   _frame_device_context, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			EndPaint(hwnd, &ps); // completes drawing -> ends the paint request and releases the device context.
		}
		break;

		case WM_SIZE: { // Resize window
			_frame_bitmap_info.bmiHeader.biWidth = LOWORD(lparam);
			_frame_bitmap_info.bmiHeader.biHeight = HIWORD(lparam);

			if (_frame_bitmap) DeleteObject(_frame_bitmap); // if already created, destroy it!
			_frame_bitmap = CreateDIBSection(NULL, &_frame_bitmap_info, DIB_RGB_COLORS, (void**)&_frame.pixels, 0, 0); // &_frame.pixels -> pixel array pointer
			SelectObject(_frame_device_context, _frame_bitmap);

			_frame.width = LOWORD(lparam);
			_frame.height = HIWORD(lparam);
		}
		break;

		case WM_CLOSE: DestroyWindow(hwnd); return 0;
		// DefWindowProc executes the default action for any window message.
		// In the case of WM_CLOSE, DefWindowProc automatically calls DestroyWindow.

		case WM_DESTROY: PostQuitMessage(0); return 0; // when the window is closed. | WM_CREATE is sent when a window is first created. |

		case WM_CREATE:
		return 0;

		case WM_MOUSEMOVE:
		return 0;

		case WM_MOVE:
		return 0;

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
	// NOTE: WS_THICKFRAME disables the ability to resize the window
	window = CreateWindowEx(
		0,											// Extended Style
		wc.lpszClassName,							// Window Class Name
		caption,									// Instance Title
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,		// Window Style
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

// Initializes and fills the bitmap frame buffer info struct
int init_frame() {
	_frame_bitmap_info.bmiHeader.biSize = sizeof(_frame_bitmap_info.bmiHeader);
	_frame_bitmap_info.bmiHeader.biPlanes = 1; // No. of colour planes is always 1
	_frame_bitmap_info.bmiHeader.biBitCount = 32; // Bits per pixel
	_frame_bitmap_info.bmiHeader.biCompression = BI_RGB; // Compression Type = Uncompressed RGB
	_frame_device_context = CreateCompatibleDC(0);

	return 1;
}

// Subsystem Windows: Entry Point
int WINAPI WinMain(_In_ HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Google it, dammit
	attach_console();
#endif

	hInst = hInstance;

	_table1.resize(128);
	_table2.resize(128);
	for (int i = 0; i < 128; ++i) {
		_table1[i].resize(128);
		_table2[i].resize(128);
	}

	if (!register_win32()) return 0;

	init_frame(); // Initialize bitmap frame

	if (create_win32(hInstance, SW_SHOWNORMAL, (SCREEN_WIDTH+4)*RENDER_SCALE_MULTIPLIER, SCREEN_HEIGHT*RENDER_SCALE_MULTIPLIER)) { 
		MSG msg{};
		bool is_running{ true };
		
		// TODO: Read on Keyboard Accelerator Tables: https://learn.microsoft.com/en-us/windows/win32/learnwin32/accelerator-tables
		// HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));
		// if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))

#if CPU_TEST
		test(); // CPU/PPU TEST
#endif // CPU_TEST

		if (!initialize()) return 0;

		while (is_running) {
			// Engine's update function

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				// This loop reads, removes and dispatches messages from the mssg queue, till there are no messages left to process.
				TranslateMessage(&msg);
				DispatchMessage(&msg); // passed onto the window

				is_running &= (msg.message != WM_QUIT); // If a quit signal is not sent, the loop will continue
			}

			// Run CPU and PPU tasks -> does CPU have to wait for PPU to complete 3 cycles
			_nes_instance->clock();

			// TODO: Display status of all registers on the window

			// Get Sprites/Tiles, Palettes for debug purposes
			_table1 = _nes_instance->get_pattern_table(0, 0);
			_table2 = _nes_instance->get_pattern_table(1, 0); // is it working properly?
			_palette = _nes_instance->get_palette();

			// Any edits to the frame buffer should be done here in the main loop [Not in the WM_PAINT window procedure]

#if SCREEN_TEST // NICK WALTON -> Draw Pixels to a Win32 Window in C with GDI
			static unsigned int p = 0;
			_frame.pixels[(p++) % (_frame.width * _frame.height)] = (rand() << 16) | (rand() << 8) | rand();
			_frame.pixels[((rand() << 16) | (rand() << 8) | rand()) % (_frame.width * _frame.height)] = 0;
#else
			// Table 1
			for (int y = 127; y >= 0; --y) { // Each Scanline
				for (int x = 0; x < 128; ++x) { // Each Pixel
					u8 pixel = _table1[127-y][x];
					u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

					// So that it doesn't overwrite some other memory or worse, crash the program:
					assert((((y * RENDER_SCALE_MULTIPLIER) + 1) * _frame.width) + (x * RENDER_SCALE_MULTIPLIER) + 1 <= (_frame.width * _frame.height)); 

					for (int h = 0; h < RENDER_SCALE_MULTIPLIER; ++h) {
						for (int w = 0; w < RENDER_SCALE_MULTIPLIER; ++w) {
							_frame.pixels[(((y * RENDER_SCALE_MULTIPLIER) + h) * _frame.width) + (x * RENDER_SCALE_MULTIPLIER)+w] = pixel_colour;
						}
					}
				}
			}
			// Table 2
			for (int y = 127; y >= 0; --y) { // Each Scanline
				for (int x = 128; x < 256; ++x) { // Each Pixel
					u8 pixel = _table2[127 - y][x-128];
					u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

					// So that it doesn't overwrite some other memory or worse, crash the program:
					assert((((y * RENDER_SCALE_MULTIPLIER) + 1) * _frame.width) + (x * RENDER_SCALE_MULTIPLIER) + 1 <= (_frame.width * _frame.height));

					for (int h = 0; h < RENDER_SCALE_MULTIPLIER; ++h) {
						for (int w = 0; w < RENDER_SCALE_MULTIPLIER; ++w) {
							_frame.pixels[(((y * RENDER_SCALE_MULTIPLIER) + h) * _frame.width) + (x * RENDER_SCALE_MULTIPLIER) + w] = pixel_colour;
						}
					}
				}
			}

			// Colour Palette
			u8 offset{ 0 };
			for (int x{ 0 }; x < 32; ++x) {
				if (x % 4 == 0) ++offset;

				u8 pixel = _palette[x];
				u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

				for (int h = 0; h < 3*RENDER_SCALE_MULTIPLIER; ++h) {
					for (int w = 0; w < 3*RENDER_SCALE_MULTIPLIER; ++w) {
						_frame.pixels[(((132 * RENDER_SCALE_MULTIPLIER) + h) * _frame.width) + (x * 3 * RENDER_SCALE_MULTIPLIER) + w + (offset * RENDER_SCALE_MULTIPLIER * 3)] = pixel_colour;
					}
				}
			}

			// HEX Value

			//++_count;
			//if (_count == 8) _count = 0;
			//
			//print_hex_value(16, 18, 150, 2, 0, 0x00FFFFFF, 0x00000000, 0);
			//print_status_value(_count, 18, 150, 2, 1, 0x00FFFFFF, 0x00000000, 0);
			//print_status_value(_count, 18, 154, 2, 1, 0x00FFFFFF, 0x00000000, 0); // 4x pixel size due to multiplier
			//print_status_value(_count, 18, 150, 2, 2, 0x00FFFF00, 0x00007878, 0);

			// Status Values
			print_cpu_status();

			print_acuumulator();
			print_x_register();
			print_y_register();
			print_stack_pointer();
			print_program_counter();

#endif // SCREEN_TEST

			// Render Next Frame
			InvalidateRect(window, NULL, FALSE);
			UpdateWindow(window);

			// TODO: find how to update the window title
			
			// TODO: set the timing circuit code for the cpu/ppu clock
		}
		delete _nes_instance;
	}

	return 1;
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		Beep(750, 300);
		return TRUE;

		// CTRL-CLOSE: confirm that the user wants to exit.
	case CTRL_CLOSE_EVENT:
		Beep(600, 200);
		printf("Ctrl-Close event\n\n");
		return TRUE;

		/*
		// Pass other signals to the next handler.
	case CTRL_BREAK_EVENT:
		Beep(900, 200);
		printf("Ctrl-Break event\n\n");
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		Beep(1000, 200);
		printf("Ctrl-Logoff event\n\n");
		return FALSE;

	case CTRL_SHUTDOWN_EVENT:
		Beep(750, 500);
		printf("Ctrl-Shutdown event\n\n");
		return FALSE;
		*/

	default:
		return FALSE;
	}
}

void print_status_value(u8 value, int x_pos, int y_pos, u8 scale, int value_count, u32 colour, u32 bg_colour, int left) {
	for (int y{ y_pos }; y < y_pos + 8; ++y) {
		for (int x{ x_pos }; x < x_pos + 8; ++x) {
			u8 pixel = _status_value_table[value][y_pos + 7 - y][x - x_pos];
			u32 pixel_colour = pixel > 0 ? colour : bg_colour;

			for (int h = 0; h < scale; ++h) {
				for (int w = 0; w < scale; ++w) {
					_frame.pixels[(((y_pos * RENDER_SCALE_MULTIPLIER) + (y - y_pos) * scale + h) * _frame.width) + (left + ((8 * value_count) * scale)) + (x - x_pos) * scale + w] = pixel_colour;
				}
			}
		}
	}
}

void print_hex_value(u8 value, int x_pos, int y_pos, u8 scale, int value_count, u32 colour, u32 bg_colour, int left) {
	for (int y{ y_pos }; y < y_pos + 8; ++y) {
		for (int x{ x_pos }; x < x_pos + 8; ++x) {
			u8 pixel = _hex_table[value][y_pos + 7 - y][x - x_pos];
			u32 pixel_colour = pixel > 0 ? colour : bg_colour;

			for (int h = 0; h < scale; ++h) {
				for (int w = 0; w < scale; ++w) {
					_frame.pixels[(((y_pos * RENDER_SCALE_MULTIPLIER) + (y - y_pos) * scale + h) * _frame.width) + (left + ((8 * value_count) * scale)) + (x - x_pos) * scale + w] = pixel_colour;
				}
			}
		}
	}
}

void print_cpu_status() {
	u8 stats = _nes_instance->get_status_register();
	u32 flag_value = 0;

	print_hex_value(16, 18, 221, 2, 0, 0x00FFFF00, 0x00007878, 0);
	
	for (int i{ 0 }; i < 8; ++i) {
		flag_value = stats & 0x01;
		stats >>= 1;
		flag_value = flag_value ? 0x0000FF00 : 0x00FF0000;
		print_status_value(i, 18, 221, 2, 8-i, 0x00000000, flag_value, 0);
	}
}

void print_acuumulator() {
	u8 stats = _nes_instance->get_accumulator();
	u32 flag_value = 0;

	print_hex_value(16, 18, 217, 2, 0, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, 18, 217, 2, 2-i, 0x00FFFFFF, 0, 0);
	}
}

void print_x_register() {
	u8 stats = _nes_instance->get_x_register();
	u32 flag_value = 0;

	print_hex_value(16, 18, 213, 2, 0, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, 18, 213, 2, 2-i, 0x00FFFFFF, 0, 0);
	}
}

void print_y_register() {
	u8 stats = _nes_instance->get_y_register();
	u32 flag_value = 0;

	print_hex_value(16, 18, 209, 2, 0, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, 18, 209, 2, 2-i, 0x00FFFFFF, 0, 0);
	}
}

void print_stack_pointer() {
	u8 stats = _nes_instance->get_stack_pointer();
	u32 flag_value = 0;

	print_hex_value(16, 18, 205, 2, 0, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, 18, 205, 2, 2 - i, 0x00FFFFFF, 0, 0);
	}
}

void print_program_counter() {
	u16 stats = _nes_instance->get_program_counter();
	u32 flag_value = 0;

	print_hex_value(16, 18, 201, 2, 0, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 4; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, 18, 201, 2, 4 - i, 0x00FFFFFF, 0, 0);
	}
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

	SetConsoleCtrlHandler(CtrlHandler, TRUE);
}

void test() {
	std::cout << "Test Initialize!\n";

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

	std::cout << "Finished...\n\n";
	//getchar();
}

bool initialize() {
	std::cout << "Initializing!\n";

	_nes_instance = new CPU::R6502();

	_nes_instance->SetBus(new CPU::Bus());
	_nes_instance->reset();

	std::cout << "Ready...\n\n";
	return true;
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

