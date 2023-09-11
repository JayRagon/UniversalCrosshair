#include <iostream>
#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <fstream>
#include <filesystem>
#include "Screenshot.h"
#include "Inject.h"
#include <chrono>
#include <cmath>
#include <numbers>

using namespace std;
using namespace std::chrono;

#pragma warning (disable : 4244)
#pragma warning (disable : 4305)
#pragma warning (disable : 26451)
#pragma warning (disable : 28251)
#pragma warning (disable : 6387)
#pragma warning (disable : 4267)

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HDC hdcSource = GetDC(NULL);

int SW = GetDeviceCaps(hdcSource, HORZRES);
int SH = GetDeviceCaps(hdcSource, VERTRES);

double sensScale;

float HUE = 0;
float S = 1;
float V = 1;
float SWC = SW / 2;
float SHC = SH / 2;

// color settings
const float MIN_H = (174.0 / 240.0) * 360.0;
const float MAX_H = (218.0 / 240.0) * 360.0;
const float MIN_S = 0.45;
const float MAX_S = 1.01;
const float MIN_V = 0.8;
const float MAX_V = 1.01;

// make sure to put an = after the cfg, otherwise it will include = to be a number (not good)
// also cfg file should look like the one in the build folder

// cfg
string fovx = "FOV_X=";
string fovy = "FOV_Y=";
string sens = "SENS=";
string dot = "DOT=";
string box = "BOX=";

int FOVX = 75;
int FOVY = 40;
int DOT = 1;
int BOX = 1;
double SENS = 1;

int dbg = 0; //////////////////////////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\

// cfg paths
filesystem::path filePath = "C:\\_Rage\\Rage.cfg";
filesystem::path filePath2 = "C:\\_Rage\\Rage2.cfg";

/*! \brief Convert HSV to RGB color space

  Converts a given set of HSV values `h', `s', `v' into RGB
  coordinates. The output RGB values are in the range [0, 1], and
  the input HSV values are in the ranges h = [0, 360], and s, v =
  [0, 1], respectively.

  \param fR Red component, used as output, range: [0, 1]
  \param fG Green component, used as output, range: [0, 1]
  \param fB Blue component, used as output, range: [0, 1]
  \param fH Hue component, used as input, range: [0, 360]
  \param fS Hue component, used as input, range: [0, 1]
  \param fV Hue component, used as input, range: [0, 1]

*/
void HSVtoRGB(float& fR, float& fG, float& fB, float& fH, float& fS, float& fV) {
	float fC = fV * fS; // Chroma
	float fHPrime = fmod(fH / 60.0, 6);
	float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
	float fM = fV - fC;

	if (0 <= fHPrime && fHPrime < 1) {
		fR = fC;
		fG = fX;
		fB = 0;
	}
	else if (1 <= fHPrime && fHPrime < 2) {
		fR = fX;
		fG = fC;
		fB = 0;
	}
	else if (2 <= fHPrime && fHPrime < 3) {
		fR = 0;
		fG = fC;
		fB = fX;
	}
	else if (3 <= fHPrime && fHPrime < 4) {
		fR = 0;
		fG = fX;
		fB = fC;
	}
	else if (4 <= fHPrime && fHPrime < 5) {
		fR = fX;
		fG = 0;
		fB = fC;
	}
	else if (5 <= fHPrime && fHPrime < 6) {
		fR = fC;
		fG = 0;
		fB = fX;
	}
	else {
		fR = 0;
		fG = 0;
		fB = 0;
	}

	fR += fM;
	fG += fM;
	fB += fM;
}
void RGBtoHSV(float& fR, float& fG, float fB, float& fH, float& fS, float& fV) {
	float fCMax = max(max(fR, fG), fB);
	float fCMin = min(min(fR, fG), fB);
	float fDelta = fCMax - fCMin;

	if (fDelta > 0) {
		if (fCMax == fR) {
			fH = 60 * (fmod(((fG - fB) / fDelta), 6));
		}
		else if (fCMax == fG) {
			fH = 60 * (((fB - fR) / fDelta) + 2);
		}
		else if (fCMax == fB) {
			fH = 60 * (((fR - fG) / fDelta) + 4);
		}

		if (fCMax > 0) {
			fS = fDelta / fCMax;
		}
		else {
			fS = 0;
		}

		fV = fCMax;
	}
	else {
		fH = 0;
		fS = 0;
		fV = fCMax;
	}

	if (fH < 0) {
		fH = 360 + fH;
	}
}

// return value = how many it found, 
// farts[] is the array of the locations of where it found, 
// find is string to find and 
// current is string to search for
int find(string current, string find, LONG farts[])
{
	int found = 0;

	for (size_t i = 0; i < current.size(); i++)
	{
		int foundletters = 0;

		if (i + find.size() > current.size() + 1)
		{
			break;
		}

		for (size_t u = 0; u < find.size(); u++)
		{
			if (current[i + u] == find[u])
			{
				foundletters++;
			}
			else
			{
				break;
			}
		}

		if (foundletters == find.size()) // if current contains find for this iteration
		{
			farts[found] = static_cast<LONG>(i);
			found++;
		}
	}

	return found;
}

// return value is the value of the cfg
// string setting is the string of the setting e.g. "FOV_X=" to find
// one of my best methods yet lol
int readCfg(string setting)
{
	ifstream cfg(filePath);

	int readint = -1;

	string current((std::istreambuf_iterator<char>(cfg)), std::istreambuf_iterator<char>());
	string rawsetting = setting;

	if (cfg.is_open() && filesystem::exists(filePath))
	{
		LONG buffer[10] = {}; // why buffer array? because the find() function needs an array to be passed in. Why [10]? Because we have enough ram for that :dealwithit:

		find(current, rawsetting, buffer);
		buffer[0] += rawsetting.length(); // buffer 0 location is now the first char after =

		string integerstr;

		// read int until \n
		for (size_t i = 0; i < current.length() - buffer[0]; i++)
		{
			if (current[i + buffer[0]] == '\n')
			{
				break;
			}
			else
			{
				integerstr[i] = current[i + buffer[0]];
			}
		}

		// string to integer, very good
		readint = stoi(integerstr, 0, 10);
	}

	cfg.close();
	return readint;
}


LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param))
	{
		return 0L;
	}

	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0L;
	}

	return DefWindowProc(window, message, w_param, l_param);
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	// Get the current process handle
	//HANDLE hProcess = GetCurrentProcess();

	// Set the process priority class to real-time
	//BOOL success = SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = instance;
	wc.lpszClassName = L"Excternal Overay Class";

	RegisterClassExW(&wc);

	const HWND window = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"External Overlay",
		WS_POPUP,
		0,
		0,
		SW,
		SH,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	RECT client_area{};
	GetClientRect(window, &client_area);

	RECT window_area{};
	GetWindowRect(window, &window_area);

	POINT diff{};
	ClientToScreen(window, &diff);

	const MARGINS margins{
		window_area.left + (diff.x - window_area.left),
		window_area.top + (diff.y - window_area.top),
		client_area.right,
		client_area.bottom
	};

	DwmExtendFrameIntoClientArea(window, &margins);
	
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 144U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = window;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* device_context{ nullptr };
	IDXGISwapChain* swap_chain{ nullptr };
	ID3D11RenderTargetView* render_target_view{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&level,
		&device_context
	);

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
		back_buffer->Release();
	}
	else
	{
		return 1;
	}

	ShowWindow(window, cmd_show);
	UpdateWindow(window);

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, device_context);


	// READCFG METHOD WORKS NOW

	FOVX = readCfg(fovx);
	FOVY = readCfg(fovy);
	SENS = readCfg(sens);
	BOX = readCfg(box);
	DOT = readCfg(dot);

	// read config (method doesn't work for some reason) (method works now)

	/*
	* 
	* DUDE WHAT THE IF STATEMENT USED TO RUN ON SHEER LUCK BRO IT WAS if (current[i] + buffer[0] == '\n') INSTEAD OF if (current[i + buffer[0]] == '\n') HOOOOWWWWWWWWWWWWWWWWW
	* 
	ofstream outfile(filePath2);
	ifstream cfg(filePath);

	string current((std::istreambuf_iterator<char>(cfg)), std::istreambuf_iterator<char>());

	string rawsetting = fovx;
	if (cfg.is_open() && filesystem::exists(filePath))
	{
		LONG buffer[10] = {}; // why buffer array? because the find() function needs an array to be passed in

		find(current, rawsetting, buffer);
		buffer[0] += rawsetting.length(); // buffer 0 location is now the first char after =, read int until \n

		string integerstr;

		// read int until \n
		for (size_t i = 0; i < current.length() - buffer[0]; i++)
		{
			if (current[i + buffer[0]] == '\r' || current[i + buffer[0]] == '\n')
			{
				break;
			}
			else
			{
				integerstr[i] = current[i + buffer[0]];
			}
		}

		// string to integer, very good
		int readint = stoi(integerstr, 0, 10);
		FOVX = readint;
	}
	else
	{
		ofstream cfgout(filePath);
		cfgout << "FOV_X=100\nFOV_Y=60\nSENS=100";
	}

	rawsetting = fovy;

	if (cfg.is_open() && filesystem::exists(filePath))
	{
		LONG buffer[10] = {}; // why buffer array? because the find() function needs an array to be passed in

		find(current, rawsetting, buffer);
		buffer[0] += rawsetting.length(); // buffer 0 location is now the first char after =, read int until \n

		string integerstr;

		// read int until \n
		for (size_t i = 0; i < current.length() - buffer[0]; i++)
		{
			if (current[i + buffer[0]] == '\r' || current[i + buffer[0]] == '\n')
			{
				break;
			}
			else
			{
				integerstr[i] = current[i + buffer[0]];
			}
		}

		// string to integer, very good
		int readint = stoi(integerstr, 0, 10);
		FOVY = readint;
	}

	rawsetting = sens;

	if (cfg.is_open() && filesystem::exists(filePath))
	{
		LONG buffer[10] = {}; // why buffer array? because the find() function needs an array to be passed in

		find(current, rawsetting, buffer);
		buffer[0] += rawsetting.length(); // buffer 0 location is now the first char after =, read int until \n

		string integerstr;

		// read int until \n
		for (size_t i = 0; i < current.length() - buffer[0]; i++)
		{
			if (current[i + buffer[0]] == '\r' || current[i + buffer[0]] == '\n')
			{
				break;
			}
			else
			{
				integerstr[i] = current[i + buffer[0]];
			}
		}

		// string to integer, very good
		int readint = stoi(integerstr, 0, 10);
		SENS = readint;
	}


	outfile << "FOVX = " << FOVX << "\nFOVY = " << FOVY << "\nSENS = " << SENS;
	outfile.close();
	cfg.close();
	*/

	SENS /= 100;

	// init color and aim
	LONG arrSize = (FOVX * 4 * FOVY);

	sensScale = 1.074 * pow(SENS, -0.9936827126);

	//creating a bitmapheader for getting the dibits
	BITMAPINFOHEADER bminfoheader;
	::ZeroMemory(&bminfoheader, sizeof(BITMAPINFOHEADER));
	bminfoheader.biSize = sizeof(BITMAPINFOHEADER);
	bminfoheader.biWidth = FOVX;
	bminfoheader.biHeight = -FOVY;
	bminfoheader.biPlanes = 1;
	bminfoheader.biBitCount = 32;
	bminfoheader.biCompression = BI_RGB;
	bminfoheader.biSizeImage = FOVX * 4 * FOVY;
	bminfoheader.biClrUsed = 0;
	bminfoheader.biClrImportant = 0;

	bool running = true;

	int pixelsFound = 0;
	int xAvg;
	int yAvg;
	int xMove;
	int yMove;

	while (running) //hackloop
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				running = false;
			}
		}

		if (!running)
		{
			break;
		}

		auto duration = high_resolution_clock::now();

		HDC hdc = GetDC(NULL);

		xAvg = 0;
		yAvg = 0;

		pixelsFound = 0;

		// SCREENGRAB HERE

		HBITMAP hBitmap;

		unsigned char* pPixels = new unsigned char[(FOVX * 4 * FOVY)];

		ScreenCaptureNBM(SWC - (FOVX / 2), SHC - (FOVY / 2), FOVX, FOVY, hBitmap);

		GetDIBits(hdc, hBitmap, 0, FOVY, pPixels, (BITMAPINFO*)&bminfoheader, DIB_RGB_COLORS);

		// PXSEARCH HERE

		for (int y = 0; y < FOVY; y += 1)
		{
			for (int x = 0; x < FOVX; x += 1)
			{
				float r = ((float)pPixels[((int)FOVX * y + x) * 4 + 2]) / 255;
				float g = ((float)pPixels[(FOVX * y + x) * 4 + 1]) / 255;
				float b = ((float)pPixels[(FOVX * y + x) * 4 + 0]) / 255;

				float h = 13;
				float s = 12;
				float v = 15;

				//cout << "r = " << r << "\ng = " << g << "\nb = " << b << endl;

				RGBtoHSV(r, g, b, h, s, v);

				if (h > MIN_H && h < MAX_H &&
					s > MIN_S && s < MAX_S &&
					v > MIN_V && v < MAX_V)
				{
					if (pixelsFound == 0)//
					{
						xAvg += (double)(x + (SWC - (FOVX / 2)));
						yAvg += (double)(y + (SHC - (FOVY / 2)));

						pixelsFound++;
					}
					//cout << "h = " << h << "\ns = " << s << "\nv = " << v << endl;
				}



				/*
				if (MIN_R < pPixels[(FOVX * y + x) * 4 + 2] && MAX_R > pPixels[(FOVX * y + x) * 4 + 2]
					&& MIN_G < pPixels[(FOVX * y + x) * 4 + 1] && MAX_G > pPixels[(FOVX * y + x) * 4 + 1]
					&& MIN_B < pPixels[(FOVX * y + x) * 4 + 0] && MAX_B > pPixels[(FOVX * y + x) * 4 + 0])
				{
					if (pixelsFound == 0)
					{
						xAvg += x + (SWC - (FOVX / 2));
						yAvg += y + (SHC - (FOVY / 2));

						pixelsFound++;
					}
				}*/
			}
		}

		// NO MEMORY LEAKS YEAHHHHHH
		DeleteDC(hdc); // delete the DC
		DeleteObject(hBitmap); // delete the bitmap and all that
		delete[] pPixels; // delete the array of bgr

		//cout << pixelsFound << "\n";

		if (pixelsFound != 0) // if a pixel has been found
		{
			// avg out the results

			xAvg /= pixelsFound;
			yAvg /= pixelsFound;

			// AIM TIME! :3
			if (true)
			{
				// std instaflick (mostly works)
				
				xMove = xAvg - SWC;
				yMove = yAvg - SHC;

				yMove += 10; // offset
				
				// 3d camera scaling so that it doesn't overflick on very large flicks
				yMove *= 1.0 - (yMove * 0.000015);
				xMove *= 1.0 - (xMove * (xMove * 0.00000015));

				// this smoothing is smoother on the y-axis so that it will curve to make it look more legit
				xMove *= sensScale * 0.12; // smoothing & sens scaling
				yMove *= sensScale * 0.0714285;

				xMove *= 1.021;
				yMove *= 1.021;
				
				/*
				xMove = (double)(20 * ((double)1 - exp(((double)xMove / 120) * -1)));
				yMove = (double)(20 * ((double)1 - exp(((double)yMove / 120) * -1)));

				xMove *= sensScale * 1; // smoothing & sens scaling
				yMove *= sensScale * 1;
				*/

				if (!GetAsyncKeyState(VK_MENU)) // the "boss" key (VK_MENU is ALT)
				{
					if (abs(xAvg - SWC) > (0.003 * SW) || abs(yAvg - SHC) > (0.003 * SH))
					{
						Move((int)xMove, (int)yMove);
					}
					else
					{
						if (GetAsyncKeyState('T')) //for some reason, sometimes when i'm not holding T it thinks i am? No clue how to fix, GetAsyncKeyState has always worked for me
						{
							//Click(); // click is working well but i dont want it
						}
					}
				}
			}
		}
		else
		{
			// reset move to 0 so that next time you click it doesn't jump to a player that was seen 10 light years ago
			Move(0, 0);
		}


		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		// render here

		// shift hue
		HUE += 0.8;

		// skip purple color
		if (HUE > (170 * 360) / 240 && HUE < (220 * 360) / 240)
		{
			HUE = (220 / 240) * 360;
		}
		
		if (HUE == 361)
		{
			HUE = 0;
		}

		float R = 0;
		float G = 0;
		float B = 0;

		HSVtoRGB(R, G, B, HUE, S, V);

		//ImGui::GetBackgroundDrawList()->Add

		if (DOT == 1)
		{
			ImGui::GetBackgroundDrawList()->AddCircleFilled({ SWC, SHC }, 2.5f, ImColor(R, G, B), 0);
		}

		// dbg color detection
		//ImGui::GetBackgroundDrawList()->AddCircleFilled({ (float)xAvg, (float)yAvg }, 3.f, ImColor(1.f, 1.f, 1.f), 0);
		
		if (xAvg != 0 && dbg == 1)
		{
			ImGui::GetBackgroundDrawList()->AddRect({ (float)xAvg - 5, (float)yAvg - 5 }, { (float)xAvg + 5, (float)yAvg + 5 }, ImColor(1.f, 1.f, 1.f), 0.0f, 0, 1.7f);
		}

		// draw fov of aimbot
		if (BOX == 1)
		{
			ImGui::GetBackgroundDrawList()->AddRect(
				{ SWC - ((FOVX / 2) + 3), SHC - ((FOVY / 2) + 3) },
				{ SWC + ((FOVX / 2) + 3), SHC + ((FOVY / 2) + 3) },
				ImColor(1.f, 1.f, 1.f), 0.0f, 0, 1.5f);
		}
			
		ImGui::Render();

		constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
		device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
		device_context->ClearRenderTargetView(render_target_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		swap_chain->Present(1U, 0U);

		auto finish = chrono::high_resolution_clock::now();
		chrono::duration<double> elapsed = finish - duration;

		ofstream dbgo(filePath2);
		ofstream dbgi(filePath2);
		//dbgo << elapsed;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swap_chain){
		swap_chain->Release();
	}
	if (device_context) {
		device_context->Release();
	}
	if (device) {
		device->Release();
	}
	if (render_target_view) {
		render_target_view->Release();
	}

	DestroyWindow(window);

	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}