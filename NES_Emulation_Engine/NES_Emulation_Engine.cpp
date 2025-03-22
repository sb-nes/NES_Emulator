// NES_Emulation_Engine.cpp : 'main' function -> Program Execution Entry Point.

#include <iostream>
#include <crtdbg.h>
#include <thread>
#include <future>
#include <chrono>

#include "CPU/R6502.h"
#include "NES_Emulation_Engine.h"

//#include "Utilities/Disassembler.h"

// For Timing and other clock stuff
// https://stackoverflow.com/questions/72403979/writing-a-clockloop-that-is-triggered-in-the-mhz-consistently-and-effeciently

using namespace NES;

// Platform Independant Code

namespace {
	u8 get_palette_ram_address(u8 address) {
		address &= 0x1F;
		if ((address & 0x03) == 0) return address & 0x0F;
		else return address;
	}
} // anonymous namespace


CPU::R6502														_nes_instance{};
PPU::R2C02*														_ppu_instance{};
display															_display;
u8																_controller1{ 0x00 };
u8																_controller2{ 0x00 };
std::array<std::shared_ptr<NES::Input::Controller>, 2>			_controller;

pattern_table													_table1;
pattern_table													_table2;
palette															_palette;
nametable														_nametable;
int																_count{ 0 };
unsigned int													_tick{ 0 };
bool															_dispatched{ false };
std::mutex														_nes_mutex;
std::future<void>												_nes;

bool createNES() {
	std::cout << "\nCreating NES Hardware Instance!" << std::endl;
	try {

		_nes_instance.reset();

#if CPU_TEST
		_nes_instance.set_instructions_count(88);

		for (; _nes_instance.get_instructions_count() > 0; ) {
			_nes_instance.clock();
		}
		_nes_instance.DisassembleRAM(0, 40);

		return false;
#else

#endif // CPU_TEST
	}
	catch (const std::exception&) {
		std::cout << "Failed...\n\n";
		return false;
	}

	std::cout << "Initialized H/W...\n\n";
	return true;
}

void destroyNES() {
	//delete _nes_instance;
	std::cout << "NES Instance Terminated!\n\n";
}

// Platform Dependant Code

#if _WIN64 & WINDOWS_GDI

#include <Windows.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Disable Rarely Used Windows API/Stuff
#endif // !WIN32_LEAN_AND_MEAN

// Forward Declarations
void attach_console();
void print_cpu_status();
void print_acuumulator();
void print_x_register();
void print_y_register();
void print_stack_pointer();
void print_program_counter();

void print_status_value(u8 value, int x_pos, int y_pos, u8 scale, u32 colour = 0x00FFFFFF, u32 bg_colour = 0x00000000, int char_index = 0, int line_index = 0);
void print_hex_value(u8 value, int x_pos, int y_pos, u8 scale, u32 colour = 0x00FFFFFF, u32 bg_colour = 0x00000000, int char_index = 0, int line_index = 0);

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
					break;
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

		//Keyboard
		case WM_KEYDOWN:
			switch (wparam) {
				case 'A':
					_controller1 = _controller1 | 0x01;
				break;
				case 'B':
					_controller1 = _controller1 | 0x02;
				break;
				case VK_RETURN:
					_controller1 = _controller1 | 0x04;
				break;
				case VK_SPACE:
					_controller1 = _controller1 | 0x08;
				break;
				case VK_UP:
					_controller1 = _controller1 | 0x10;
				break;
				case VK_DOWN:
					_controller1 = _controller1 | 0x20;
				break;
				case VK_LEFT:
					_controller1 = _controller1 | 0x40;
				break;
				case VK_RIGHT:
					_controller1 = _controller1 | 0x80;
				break;

				default:
					return DefWindowProc(hwnd, msg, wparam, lparam);
			}
			return 0;

		case WM_KEYUP:
			switch (wparam) {
				case 'A':
					_controller1 = _controller1 & 0xFE;
				break; 
				case 'B':
					_controller1 = _controller1 & 0xFD;
				break;
				case VK_RETURN:
					_controller1 = _controller1 & 0xFB;
				break;
				case VK_SPACE:
					_controller1 = _controller1 & 0xF7;
				break;
				case VK_UP:
					_controller1 = _controller1 & 0xEF;
				break;
				case VK_DOWN:
					_controller1 = _controller1 & 0xDF;
				break;
				case VK_LEFT:
					_controller1 = _controller1 & 0xBF;
				break;
				case VK_RIGHT:
					_controller1 = _controller1 & 0x7F;
				break;

			default:
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}
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

// Clocks the CPU till the PPU is past v_blank and ready to display the output, then Update the Bitmap screen elements.
void update_frame() {

	FrameTimer timer;
	
	// Run CPU and PPU tasks -> does CPU have to wait for PPU to complete 3 cycles
	while (!_ppu_instance->_frame_scan_complete) {
		if (_controller[0]->get_latch()) {
			_controller[0]->set(_controller1);
			_controller[1]->set(_controller2);
		}
		_nes_instance.clock();
	}

	// Get Sprites/Tiles, Palettes for debug purposes
	_table1 = _ppu_instance->get_pattern_table(0, 0); // TODO: fix vector's wrong usage: don't copy, pass reference
	_table2 = _ppu_instance->get_pattern_table(1, 2); // is it working properly?
	_palette = _ppu_instance->get_palette();


#if SCREEN_TEST // NICK WALTON -> Draw Pixels to a Win32 Window in C with GDI
	static unsigned int p = 0;
	if ((_frame.width * _frame.height * RENDER_SCALE_MULTIPLIER * RENDER_SCALE_MULTIPLIER) >= (SCREEN_WIDTH * SCREEN_HEIGHT * RENDER_SCALE_MULTIPLIER * RENDER_SCALE_MULTIPLIER)) { // to fix error on minimize
		_frame.pixels[(p++) % (_frame.width * _frame.height)] = (rand() << 16) | (rand() << 8) | rand();
		_frame.pixels[((rand() << 16) | (rand() << 8) | rand()) % (_frame.width * _frame.height)] = 0;
	}
#else

	std::lock_guard<std::mutex> lock(_nes_mutex);

	// Pattern Tables
	for (int y = 127; y >= 0; --y) { // Each Scanline
		for (int x = 0; x < 128; ++x) { // Each Pixel
			u8 pixel1 = _table1[127 - y][x];
			u8 pixel2 = _table2[127 - y][x]; // is there some unknown writing happening to pattern tables behind my back | or is it writing hi instead of lo
			u32 pixel_colour1 = (_pal_colour_lookup[pixel1 >> 4][pixel1 & 0x0F].red << 16) | (_pal_colour_lookup[pixel1 >> 4][pixel1 & 0x0F].green << 8) | _pal_colour_lookup[pixel1 >> 4][pixel1 & 0x0F].blue;
			u32 pixel_colour2 = (_pal_colour_lookup[pixel2 >> 4][pixel2 & 0x0F].red << 16) | (_pal_colour_lookup[pixel2 >> 4][pixel2 & 0x0F].green << 8) | _pal_colour_lookup[pixel2 >> 4][pixel2 & 0x0F].blue;

			// if it goes past width * height, it overwrite some other memory or worse, crash the program!
			for (int h = 0; h < RENDER_SCALE_MULTIPLIER; ++h) {
				for (int w = 0; w < RENDER_SCALE_MULTIPLIER; ++w) {
					_frame.pixels[(y * RENDER_SCALE_MULTIPLIER + h) * _frame.width + (256 + x) * RENDER_SCALE_MULTIPLIER + w] = pixel_colour1;
					_frame.pixels[(y * RENDER_SCALE_MULTIPLIER + h) * _frame.width + (384 + x) * RENDER_SCALE_MULTIPLIER + w] = pixel_colour2;
				}
			}
		}
	}

#if NAMETABLE_TEST
	// Nametable
	_nametable = _ppu_instance->get_nametable(0);
	u8 value{ 0 };
	for (int y = 0; y < 30; ++y) { // Each Scanline
		for (int x = 0; x < 32; ++x) { // Each Pixel
			value = _nametable[(29 - y) * 32 + x];
			print_hex_value(value & 0x0F, 0, 0, 1, 0x00FFFFFF, 0, x * 2 + 1 , y * 2); // Low Hex
			value >>= 4;
			print_hex_value(value & 0x0F, 0, 0, 1, 0x00FFFFFF, 0, x * 2, y * 2); // Hi Hex
		}
	}
#elif NAMETABLE_PRINT_TEST
	_nametable = _ppu_instance->get_nametable(0);
	u8 value{ 0 };
	for (int j = 0; j < 30; ++j) { // Each Scanline
		for (int i = 0; i < 32; ++i) { // Each Pixel
			value = _nametable[(29 - j) * 32 + i];
			u32 _tablex = (value & 0x0F) * 8;
			u32 _tabley = ((value >> 4) & 0x0F) * 8 + 7;
			for (int y{ 0 }; y < 8; ++y) {
				for (int x{ 0 }; x < 8; ++x) {
					u8 pixel = _table1[_tabley - y][_tablex + x];
					u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

					u32 x_temp = (i * 8 + x) * RENDER_SCALE_MULTIPLIER;
					u32 y_temp = (j * 8 + y) * RENDER_SCALE_MULTIPLIER;

					for (int h = 0; h < RENDER_SCALE_MULTIPLIER; ++h) {
						for (int w = 0; w < RENDER_SCALE_MULTIPLIER; ++w) {
							_frame.pixels[(y_temp + h) * _frame.width + x_temp + w] = pixel_colour;
						}
					}
				}
			}
		}
	}
#else
	// Output Display
	_display = _ppu_instance->get_render_screen();
	for (int y = 0; y < 240; ++y) { // Each Scanline
		for (int x = 0; x < 256; ++x) { // Each Pixel
			u8 pixel = _display[239 - y][x];
			u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

			// if it goes past width * height, it overwrite some other memory or worse, crash the program!
			for (int h = 0; h < RENDER_SCALE_MULTIPLIER; ++h) {
				for (int w = 0; w < RENDER_SCALE_MULTIPLIER; ++w) {
					_frame.pixels[(y * RENDER_SCALE_MULTIPLIER + h) * _frame.width + x * RENDER_SCALE_MULTIPLIER + w] = pixel_colour;
				}
			}
		}
	}
#endif

	// Colour Palette
	u8 offset{ 0 };
	for (u8 x{ 0 }; x < 32; ++x) {
		if (x % 4 == 0) ++offset;

		u8 pixel = _palette[get_palette_ram_address(x)];
		u32 pixel_colour = (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].red << 16) | (_pal_colour_lookup[pixel >> 4][pixel & 0x0F].green << 8) | _pal_colour_lookup[pixel >> 4][pixel & 0x0F].blue;

		for (int h = 0; h < 3 * RENDER_SCALE_MULTIPLIER; ++h) {
			for (int w = 0; w < 3 * RENDER_SCALE_MULTIPLIER; ++w) {
				_frame.pixels[256 * RENDER_SCALE_MULTIPLIER + (((132 * RENDER_SCALE_MULTIPLIER) + h) * _frame.width) + (x * 3 * RENDER_SCALE_MULTIPLIER) + w + (offset * RENDER_SCALE_MULTIPLIER * 3)] = pixel_colour;
			}
		}
	}

	// Status Values
	print_cpu_status();

	print_acuumulator();
	print_x_register();
	print_y_register();
	print_stack_pointer();
	print_program_counter();

#endif

	// Render Next Frame
	InvalidateRect(window, NULL, FALSE);
	UpdateWindow(window);

	_ppu_instance->_frame_scan_complete = false;
	_dispatched = false;
}

// Subsystem Windows: Entry Point
int WINAPI WinMain(_In_ HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

#if _DEBUG | CONSOLE_DBG_OUT
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

		if (!createNES()) return 0;
		_nes_instance.get_ppu(_ppu_instance);
		_nes_instance.get_controllers(_controller);

		while (is_running) {
			// Engine's update function

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				// This loop reads, removes and dispatches messages from the mssg queue, till there are no messages left to process.
				TranslateMessage(&msg);
				DispatchMessage(&msg); // passed onto the window

				is_running &= (msg.message != WM_QUIT); // If a quit signal is not sent, the loop will continue
			}

			// TODO: dispatch to another thread later after testing if ppu works 
			//if (!_dispatched) {
			//	_dispatched = true;
			//	_nes = std::async(std::launch::async ,update_frame);
			//}

			update_frame();

			// Any edits to the frame buffer should be done here in the main loop [Not in the WM_PAINT window procedure]

			// TODO: find how to update the window title

			
		}
		destroyNES();
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

void print_status_value(u8 value, int x_pos, int y_pos, u8 scale, u32 colour, u32 bg_colour, int char_index, int line_index) {
	for (int y{ y_pos }; y < y_pos + 8; ++y) {
		for (int x{ x_pos }; x < x_pos + 8; ++x) {
			u8 pixel = _status_value_table[value][y_pos + 7 - y][x - x_pos];
			u32 pixel_colour = pixel > 0 ? colour : bg_colour;

			u32 x_temp = (char_index * 8 + x) * scale;
			u32 y_temp = (line_index * 8 + y) * scale;

			for (int h = 0; h < scale; ++h) {
				for (int w = 0; w < scale; ++w) {
					_frame.pixels[(y_temp + h) * _frame.width + x_temp + w] = pixel_colour;
				}
			}
		}
	}
}

void print_hex_value(u8 value, int x_pos, int y_pos, u8 scale, u32 colour, u32 bg_colour, int char_index, int line_index) {
	for (int y{ y_pos }; y < y_pos + 8; ++y) {
		for (int x{ x_pos }; x < x_pos + 8; ++x) {
			u8 pixel = _hex_table[value][y_pos + 7 - y][x - x_pos];
			u32 pixel_colour = pixel > 0 ? colour : bg_colour;

			u32 x_temp = (char_index * 8 + x) * scale;
			u32 y_temp = (line_index * 8 + y) * scale;

			for (int h = 0; h < scale; ++h) {
				for (int w = 0; w < scale; ++w) {
					_frame.pixels[(y_temp + h) * _frame.width + x_temp + w] = pixel_colour;
				}
			}
		}
	}
}

#ifndef CPU_STATUS_PRINT_OFFSET
#define CPU_STATUS_PRINT_OFFSET 256

void print_cpu_status() {
	u8 stats = _nes_instance.get_status_register();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 171, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 0);
	
	for (int i{ 0 }; i < 8; ++i) {
		flag_value = stats & 0x01;
		stats >>= 1;
		flag_value = flag_value ? 0x0000FF00 : 0x00FF0000;
		print_status_value(i, CPU_STATUS_PRINT_OFFSET, 171, RENDER_SCALE_MULTIPLIER, 0x00000000, flag_value, 8-i);
	}
}

void print_acuumulator() {
	u8 stats = _nes_instance.get_accumulator();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFFFF, 0, 2-i);
	}
}

void print_x_register() {
	u8 stats = _nes_instance.get_x_register();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 4);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFFFF, 0, 6-i);
	}
}

void print_y_register() {
	u8 stats = _nes_instance.get_y_register();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 8);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, CPU_STATUS_PRINT_OFFSET, 163, RENDER_SCALE_MULTIPLIER, 0x00FFFFFF, 0, 10-i);
	}
}

void print_stack_pointer() {
	u8 stats = _nes_instance.get_stack_pointer();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 155, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 0);

	for (int i{ 0 }; i < 2; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, CPU_STATUS_PRINT_OFFSET, 155, RENDER_SCALE_MULTIPLIER, 0x00FFFFFF, 0, 2 - i);
	}
}

void print_program_counter() {
	u16 stats = _nes_instance.get_program_counter();
	u32 flag_value = 0;

	print_hex_value(16, CPU_STATUS_PRINT_OFFSET, 155, RENDER_SCALE_MULTIPLIER, 0x00FFFF00, 0x00007878, 4);

	for (int i{ 0 }; i < 4; ++i) {
		flag_value = stats & 0x0F;
		stats >>= 4;
		print_hex_value(flag_value, CPU_STATUS_PRINT_OFFSET, 155, RENDER_SCALE_MULTIPLIER, 0x00FFFFFF, 0, 8 - i);
	}
}

#undef CPU_STATUS_PRINT_OFFSET
#endif // !CPU_STATUS PRINT_OFFSET

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

#elif GLFW
// Use Subsystem CONSOLE instead of WINDOWS

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <string>
#include <sstream>

struct ShaderProgramSource {
	std::string VertexSource;
	std::string FragmentSource;
};

static ShaderProgramSource parseShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	enum class ShaderType {
		NONE = -1,
		VERTEX = 0, 
		FRAGMENT = 1,
	};

	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;
	std::string line;
	while (getline(stream, line)) {
		if (line.find("#shader") != std::string::npos) {
			if (line.find("vertex") != std::string::npos) {
				type = ShaderType::VERTEX;

			} else if (line.find("fragment") != std::string::npos) {
				type = ShaderType::FRAGMENT;

			}
		} else {
			ss[(int)type] << line << '\n';
		}
	}

	return { ss[0].str(), ss[1].str() };
}

static unsigned int compileShader(unsigned int type, const std::string& source) {
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	// HANDLE ERRORS if compilation failed
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result); // i -> integer, v -> vector

	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

		// char message[length] -> error -> doesn't let you dynamically create an array on the stack
		char* message = (char*)alloca(length * sizeof(char)); // alloca() is one way that lets you overcome it.

		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to Compile " <<
			(type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
			<< " Shader!" << std::endl;
		std::cout << message << std::endl;
		
		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader) {
	unsigned int program = glCreateProgram();
	unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);
	glValidateProgram(program);

	// Should i call detach shader? it might be helpful to keep the source code of the shader alive for debug purposes
	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

// Subsystem Console: Entry Point
int main(void) {

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Google it, dammit
#endif
	
	GLFWwindow* window;

	// Initialize the library 
	if (!glfwInit()) return -1;

	// Create a windowed mode window and its OpenGL context 
	window = glfwCreateWindow(640, 480, "NES Emulator", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current 
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) { // Error handler
		std::cout << "Failed to Initialize GLEW" << std::endl;
		glfwTerminate();
		return -1;
	}

	std::cout << "OpenGl " << glGetString(GL_VERSION) << std::endl;

	float positions[] = { // array of contiguous memory -> therefore, it's also a buffer 
		-0.5f, -0.5f, // 0
		 0.5f, -0.5f, // 1
		 0.5f,  0.5f, // 2
		-0.5f,  0.5f  // 3
	};

	unsigned int indices[] = {
		0,1,2,
		2,3,0
	};

	unsigned int buffer_id; // id for the buffer [object in general]
	glGenBuffers(1, &buffer_id); // create a buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer_id); // select the buffer
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), positions, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0); // NOTE: Remember to enable the index of the array to use it, otherwise nothing will be displayed.
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)0);

	unsigned int ibo; // id for the buffer [object in general]
	glGenBuffers(1, &ibo); // create a buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // select the buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	ShaderProgramSource source = parseShader("Resources/Shaders/basic.shader");

	std::cout << "Vertex:" << std::endl;
	std::cout << source.VertexSource << std::endl;
	std::cout << "Fragment:" << std::endl;
	std::cout << source.FragmentSource << std::endl;

	unsigned int shader = createShader(source.VertexSource, source.FragmentSource);
	glUseProgram(shader);

	createNES();
	_nes_instance->get_ppu(_ppu_instance);

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window)) {
		_tick++;
		_ppu_instance->clock();
		if (_ppu_instance->_nmi_trigger) _nes_instance->nmi();

		if (_tick % 3 == 0) {
			_nes_instance->clock();
			_tick = 0;
		}

		// Render here 
		glClear(GL_COLOR_BUFFER_BIT);

		//glDrawArrays(GL_TRIANGLES, 0, 3); // if we don't have index buffers
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // used with an index buffer

		/* Drawn using Legacy OpenGL for immediate tests

		glBegin(GL_TRIANGLES);
		
		glVertex2f(-0.5f, -0.5f);
		glVertex2f(0.0f, 0.5f);
		glVertex2f(0.5f, -0.5f);
		
		glEnd();
		*/

		// Swap front and back buffers 
		glfwSwapBuffers(window);

		// Poll for and process events 
		glfwPollEvents();
	}

	glDeleteProgram(shader);

	glfwTerminate();
	destroyNES();
	return 0;
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

