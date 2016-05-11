#include "WWC_Win32.h"

LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);

WWC_Win32::WWC_Win32(WCore* core) : WWindowComponent(core) {
	m_isMinimized = true;
}
WError WWC_Win32::Initialize(int width, int height) {
	m_mainWindow = nullptr;
	m_hInstance = GetModuleHandleA(nullptr);
	m_minWindowX = 640 / 4;
	m_minWindowY = 480 / 4;
	m_maxWindowX = 640 * 20;
	m_maxWindowY = 480 * 20;
	m_isMinimized = false;

	//do not initialize if the window is already there
	if (m_mainWindow) {
		//build window rect from client rect (client rect = directx rendering rect)
		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = width;
		rc.bottom = height;
		AdjustWindowRectEx(&rc, (DWORD)m_core->engineParams["windowStyle"],
			(m_core->engineParams["windowMenu"] == nullptr) ? FALSE : TRUE,
			(DWORD)m_core->engineParams["windowStyleEx"]);

		//adjust window position (no window re-creation)
		SetWindowPos(m_mainWindow, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);

		return WError(W_SUCCEEDED);
	}

	//create window class
	WNDCLASSEXA wcex;

	wcex.cbSize = sizeof WNDCLASSEXA;
	wcex.style = (DWORD)m_core->engineParams["classStyle"];
	wcex.lpfnWndProc = hMainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = (HICON)m_core->engineParams["classIcon"];
	wcex.hCursor = (HCURSOR)m_core->engineParams["classCursor"];
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCSTR)m_core->engineParams["menuName"];
	wcex.lpszClassName = "WWndClasse";
	wcex.hIconSm = (HICON)m_core->engineParams["classIcon_sm"];

	//keep trying to register the class until a valid name exist (for multiple cores)
	if (!RegisterClassExA(&wcex))
		return WError(W_CLASSREGISTERFAILED);

	//adjust the size, the provided sizes (width,height) should be the client area sizes, so build the window size from the client size
	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = width;
	rc.bottom = height;
	AdjustWindowRectEx(&rc, (DWORD)m_core->engineParams["windowStyle"],
		(m_core->engineParams["windowMenu"] == nullptr) ? FALSE : TRUE,
		(DWORD)m_core->engineParams["windowStyleEx"]);

	//set to default positions if specified
	int x, y;
	if ((int)m_core->engineParams["defWndX"] == -1 && (int)m_core->engineParams["defWndY"] == -1) {
		x = (GetSystemMetrics(SM_CXSCREEN) / 2) - ((rc.right - rc.left) / 2);
		y = (GetSystemMetrics(SM_CYSCREEN) / 2) - ((rc.bottom - rc.top) / 2);
	}
	else {
		x = (int)m_core->engineParams["defWndX"];
		y = (int)m_core->engineParams["defWndY"];
	}

	//
	//Create the main window
	//
	m_mainWindow = CreateWindowExA((DWORD)m_core->engineParams["windowStyleEx"], wcex.lpszClassName,
		(LPCSTR)m_core->engineParams["appName"],
		(DWORD)m_core->engineParams["windowStyle"],
		x, y, rc.right - rc.left, rc.bottom - rc.top,
		(HWND)m_core->engineParams["windowParent"], (HMENU)m_core->engineParams["windowMenu"], m_hInstance, nullptr);

	GetWindowRect(m_mainWindow, &rc);
	if (!m_mainWindow) //error creating the window
	{
		return WError(W_WINDOWNOTCREATED);
	}

	//give the main window's procedure an instance of the core
	SetWindowLongA(m_mainWindow, GWL_USERDATA, (LONG)(void*)m_core);

	return WError(W_SUCCEEDED);
}
bool WWC_Win32::Loop() {
	return !m_isMinimized;
}
void WWC_Win32::Cleanup() {
	if (m_mainWindow)
		DestroyWindow(m_mainWindow);
}

void WWC_Win32::SetWindowTitle(const char* const title) {
	//set the title of the main window
	SetWindowTextA(m_mainWindow, title);
}
void WWC_Win32::SetWindowPosition(int x, int y) {
	//set the position of the main window
	SetWindowPos(m_mainWindow, nullptr, x, y, 0, 0, SWP_NOSIZE);
}
void WWC_Win32::SetWindowSize(int width, int height) {
	//manual window resizing

	//save old sizes
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	int oldSizeX = rc.right;
	int oldSizeY = rc.bottom;

	//change main window size
	SetWindowPos(m_mainWindow, nullptr, 0, 0, width, height, SWP_NOMOVE);
	GetClientRect(m_mainWindow, &rc);

	//re-initialize the core to fit the new size
	// TODO: resize engine
	//m_core->Init(rc.right, rc.bottom, m_core->IsVSyncEnabled());
}
void WWC_Win32::MaximizeWindow(void) {
	//show window as maximized
	ShowWindow(m_mainWindow, SW_SHOWMAXIMIZED);
	m_isMinimized = false; //mark not minimized
}
void WWC_Win32::MinimizeWindow(void) {
	//show window as minimized
	ShowWindow(m_mainWindow, SW_SHOWMINIMIZED);
	m_isMinimized = true; //mark minimized
}
uint WWC_Win32::RestoreWindow(void) {
	//restore window
	int result = ShowWindow(m_mainWindow, SW_RESTORE);
	m_isMinimized = false; //mark not minimized
	return result;
}
uint WWC_Win32::GetWindowWidth(void) const {
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	return rc.right;
}
uint WWC_Win32::GetWindowHeight(void) const {
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	return rc.bottom;
}
int WWC_Win32::GetWindowPositionX(void) const {
	//return the left coordinate of the window rect
	RECT rc;
	GetWindowRect(m_mainWindow, &rc);
	return rc.left;
}
int WWC_Win32::GetWindowPositionY(void) const {
	//return the top coordinate of the window rect
	RECT rc;
	GetWindowRect(m_mainWindow, &rc);
	return rc.top;
}
HWND WWC_Win32::GetWindow(void) const {
	//return the API HWND
	return m_mainWindow;
}
HINSTANCE WWC_Win32::GetInstance(void) const {
	//return the instance
	return m_hInstance;
}
void WWC_Win32::SetFullScreenState(bool bFullScreen) {
	//set the fullscreen state of the window
	// TODO: implement this
	//m_core->GetSwapChain()->SetFullscreenState(bFullScreen, nullptr);
}
bool WWC_Win32::GetFullScreenState(void) const {
	BOOL bf = false;
	// TODO: implement this
	//m_core->GetSwapChain()->GetFullscreenState(&bf, nullptr);
	return (bool)bf;
}
void WWC_Win32::SetWindowMinimumSize(int minX, int minY) {
	m_minWindowX = minX;
	m_minWindowY = minY;
}
void WWC_Win32::SetWindowMaximumSize(int maxX, int maxY) {
	m_maxWindowX = maxX;
	m_maxWindowY = maxY;
}

//window procedure
LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam) {
	//get an inctance of the core
	WCore* coreInst = (WCore*)GetWindowLong(hWnd, GWL_USERDATA);

	//return default behavior if the instance is not set yet
	if (!coreInst) {
		if (msg == WM_GETMINMAXINFO) {
			//set the min/max window size
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 10;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 10;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.x = 99999;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 99999;
			return TRUE;
		}
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	LRESULT result = TRUE;

	switch (msg) {
		//quit
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		coreInst->__EXIT = true;
		break;

	/*case WM_KEYDOWN:
		//ESCAPE KEY will close the application if m_escapeE is true
		if (wParam == VK_ESCAPE && coreInst->InputDevice->__Impl__->_escapeE)
			coreInst->__Impl__->EXIT = true;
		coreInst->InputDevice->InsertRawInput(wParam, true);
		if (coreInst->__Impl__->HX11->curState)
			coreInst->__Impl__->HX11->curState->OnKeydown(wParam);
		break;
	case WM_KEYUP:
		coreInst->InputDevice->InsertRawInput(wParam, false);
		if (coreInst->__Impl__->HX11->curState)
			coreInst->__Impl__->HX11->curState->OnKeyup(wParam);
		break;
	case WM_CHAR:
		if (coreInst->__Impl__->HX11->curState)
			coreInst->__Impl__->HX11->curState->OnInput(wParam);
		break;*/
	case WM_GETMINMAXINFO:
		//set the min/max window size
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = ((WWC_Win32*)coreInst->WindowComponent)->m_minWindowX;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = ((WWC_Win32*)coreInst->WindowComponent)->m_minWindowY;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.x = ((WWC_Win32*)coreInst->WindowComponent)->m_maxWindowX;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.y = ((WWC_Win32*)coreInst->WindowComponent)->m_maxWindowY;
		break;
	case WM_SIZE:
	{
		//flag the core as minimized to prevent rendering
		if (wParam == SIZE_MINIMIZED)
			((WWC_Win32*)coreInst->WindowComponent)->m_isMinimized = true;
		if (true /* TODO: block resizing while resizing */) {
			((WWC_Win32*)coreInst->WindowComponent)->m_isMinimized = false; //flag core as not minimized to allow rendering
			RECT rc;
			GetClientRect(hWnd, &rc); //get window client dimensions

										//re-initialize the core to fit the new size
			if (!coreInst->Init(rc.right - rc.left, rc.bottom - rc.top))
				return TRUE;
		}
		break;
	}
	//mouse input update
	/*case WM_LBUTTONDOWN:
		coreInst->InputDevice->__Impl__->_leftClick = true;
		break;
	case WM_LBUTTONUP:
		coreInst->InputDevice->__Impl__->_leftClick = false;
		break;
	case WM_RBUTTONDOWN:
		coreInst->InputDevice->__Impl__->_rightClick = true;
		break;
	case WM_RBUTTONUP:
		coreInst->InputDevice->__Impl__->_rightClick = false;
		break;
	case WM_MBUTTONDOWN:
		coreInst->InputDevice->__Impl__->_middleClick = true;
		break;
	case WM_MBUTTONUP:
		coreInst->InputDevice->__Impl__->_middleClick = false;
		break;
	case WM_MOUSEMOVE:
		coreInst->InputDevice->__Impl__->_mouseX = LOWORD(lParam);
		coreInst->InputDevice->__Impl__->_mouseY = HIWORD(lParam);
		break;
	case WM_MOUSEWHEEL:
	{
		if (HIWORD(wParam) == 120)
			coreInst->InputDevice->__Impl__->_mouseZ++;
		else
			coreInst->InputDevice->__Impl__->_mouseZ--;
		break;
	}*/
	case WM_COMMAND:
	{
		//send messages to child windows
		if (HIWORD(wParam) == 0)
			if (coreInst->engineParams["windowMenu"] != nullptr && coreInst->engineParams["menuPorc"] != nullptr)
				((void(*)(HMENU, DWORD))coreInst->engineParams["menuProc"])(GetMenu(hWnd), LOWORD(wParam));
		break;
	}
	default: //default behavior for other messages
		result = DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	return result;
}