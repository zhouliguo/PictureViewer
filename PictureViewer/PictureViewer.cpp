#include <Windows.h>
#include <SDL.h>
#include <SDL_image.h>

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//打开文件对话框
int OpenDialog(HWND hwnd, TCHAR* fullPath, TCHAR* fileName);

//设置窗口中显示图像的具体区域
SDL_Rect SetShowRect(HWND hwnd, int imageWidth, int imageHeight);

//打开文件
int OpenFile(HWND hwnd, TCHAR* fullPath, TCHAR* fileName, SDL_Rect *showRect);

//关闭文件
int CloseFile();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	SDL_Init(SDL_INIT_EVERYTHING);

	static TCHAR windowClassName[] = TEXT("ViewerWindow");
	HWND mainHwnd;
	MSG message;
	WNDCLASS windowClass;

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	windowClass.lpfnWndProc = WindowProcedure;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = windowClassName;

	if (!RegisterClass(&windowClass)) {
		MessageBox(0, TEXT("This program requires Windows NT !"), windowClassName, MB_ICONERROR);
		return 0;
	}

	mainHwnd = CreateWindow(windowClassName, TEXT("图片浏览器"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
	ShowWindow(mainHwnd, iCmdShow);
	UpdateWindow(mainHwnd);

	while (GetMessage(&message, 0, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return message.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static int width;
	static int height;
	static int open = 0;

	static TCHAR fullPath[MAX_PATH];
	static TCHAR fileName[MAX_PATH];
	static char u8FullPath[MAX_PATH];

	static RECT windowRect;
	static RECT deskRect;
	static HWND deskHwnd;

	static int fullScreen = 0;
	static long windowStyle;

	static SDL_Window *sdlWindow = 0;
	static SDL_Renderer *sdlRenderer = 0;
	static SDL_Surface *sdlSurface = 0;
	static SDL_Texture *sdlTexture = 0;
	static SDL_Rect sdlShowRect;	//窗口中显示图像的具体区域

	switch (message) {
	case WM_CREATE:
		sdlWindow = SDL_CreateWindowFrom(hwnd);
		sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		sdlSurface = SDL_LoadBMP("PictureViewer.bmp");
		sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlSurface);
		SDL_FreeSurface(sdlSurface);
		break;
	case WM_SIZE:
		if (open == 0) {
			SDL_RenderClear(sdlRenderer);
			SDL_RenderCopy(sdlRenderer, sdlTexture, 0, 0);
			SDL_RenderPresent(sdlRenderer);
		}
		else {
			sdlShowRect = SetShowRect(hwnd, width, height);
			SDL_RenderClear(sdlRenderer);
			SDL_RenderCopy(sdlRenderer, sdlTexture, 0, &sdlShowRect);
			SDL_RenderPresent(sdlRenderer);
		}
		break;
	case WM_LBUTTONDBLCLK:
		if (!fullScreen) {
			windowStyle = GetWindowLong(hwnd, GWL_STYLE);
			GetWindowRect(hwnd, &windowRect);
			deskHwnd = GetDesktopWindow();
			GetWindowRect(deskHwnd, &deskRect);
			SetWindowLong(hwnd, GWL_STYLE, WS_CHILD);
			SetWindowPos(hwnd, HWND_TOP, 0, 0, deskRect.right, deskRect.bottom, SWP_SHOWWINDOW);
			fullScreen = 1;
		}
		else {
			SetWindowLong(hwnd, GWL_STYLE, windowStyle);
			MoveWindow(hwnd, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, true);
			fullScreen = 0;
		}
		break;
	case WM_LBUTTONDOWN:
		open = 1;
		SDL_DestroyTexture(sdlTexture);
		if (!OpenFile(hwnd, fullPath, fileName,&sdlShowRect)) {
			return -1;
		}
		WideCharToMultiByte(CP_UTF8,0, fullPath,-1, u8FullPath, sizeof(u8FullPath),0,0);
		sdlSurface = IMG_Load(u8FullPath);
		width = sdlSurface->w;
		height = sdlSurface->h;
		sdlShowRect = SetShowRect(hwnd, width, height);
		sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlSurface);
		SDL_FreeSurface(sdlSurface);
		SDL_RenderClear(sdlRenderer);
		SDL_RenderCopy(sdlRenderer, sdlTexture, 0, &sdlShowRect);
		SDL_RenderPresent(sdlRenderer);
		break;
	case WM_CLOSE:
		SDL_DestroyTexture(sdlTexture);
		SDL_DestroyRenderer(sdlRenderer);
		SDL_DestroyWindow(sdlWindow);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		SDL_Quit();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

int OpenDialog(HWND hwnd, TCHAR* fullPath, TCHAR* fileName) {
	static OPENFILENAME openFileName;
	static TCHAR fileFormat[] = TEXT("图片文件(*.jpg;*.bmp;*.png)\0*.jpg;*.bmp;*.png\0所有文件(*.*)\0*.*\0\0");
	openFileName.lStructSize = sizeof(OPENFILENAME);
	openFileName.hwndOwner = hwnd;
	openFileName.hInstance = 0;
	openFileName.lpstrFilter = fileFormat;
	openFileName.lpstrCustomFilter = 0;
	openFileName.nMaxCustFilter = 0;
	openFileName.nFilterIndex = 0;
	openFileName.lpstrFile = fullPath;
	openFileName.nMaxFile = MAX_PATH;
	openFileName.lpstrFileTitle = fileName;
	openFileName.nMaxFileTitle = MAX_PATH;
	openFileName.lpstrInitialDir = 0;
	openFileName.lpstrTitle = 0;
	openFileName.Flags = OFN_HIDEREADONLY;
	openFileName.nFileOffset = 0;
	openFileName.nFileExtension = 0;
	openFileName.lpstrDefExt = TEXT("mp4");
	openFileName.lCustData = 0;
	openFileName.lpfnHook = 0;
	openFileName.lpTemplateName = 0;
	return GetOpenFileName(&openFileName);
}

SDL_Rect SetShowRect(HWND hwnd, int imageWidth, int imageHeight) {
	SDL_Rect showRect;
	RECT dstRect;

	double dstW;	//显示区宽
	double dstH;	//显示区高
	double srcWH;	//图像宽高比
	double dstWH;	//显示区宽高比
	double dstSrcW;	//显示区宽/图像宽
	double dstSrcH;	//显示区高/图像高

	GetClientRect(hwnd, &dstRect);
	dstW = dstRect.right - dstRect.left;
	dstH = dstRect.bottom - dstRect.top;
	srcWH = (double)imageWidth / (double)imageHeight;
	dstWH = dstW / dstH;
	dstSrcW = dstW / imageWidth;
	dstSrcH = dstH / imageHeight;
	if (srcWH > dstWH) {
		showRect.x = 0;
		showRect.y = dstH / 2 - dstSrcW*imageHeight / 2;
		showRect.w = dstRect.right - dstRect.left;
		showRect.h = dstSrcW*imageHeight;
	}
	else if (srcWH < dstWH) {
		showRect.x = dstW / 2 - dstSrcH*imageWidth / 2;
		showRect.y = 0;
		showRect.w = dstSrcH*imageWidth;
		showRect.h = dstRect.bottom - dstRect.top;
	}
	else {
		showRect.x = 0;
		showRect.y = 0;
		showRect.w = dstRect.right - dstRect.left;
		showRect.h = dstRect.bottom - dstRect.top;
	}
	return showRect;
}

int OpenFile(HWND hwnd, TCHAR* fullPath, TCHAR* fileName,SDL_Rect *showRect) {
	if (!OpenDialog(hwnd, fullPath, fileName)) {
		MessageBox(0, TEXT("无法打开文件！"), 0, MB_ICONERROR);
		return 0;
	}
	SetWindowText(hwnd, fileName);
	return 1;
}

int CloseFile() {
	return 1;
}