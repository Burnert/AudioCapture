#include "ACPCH.h"

#include "Window.h"

namespace Application
{
	const char Window::ClassName[] = "AudioCaptureWindow";

	Window::Window() 
		: Window(ClassName) 
	{ }

	Window::Window(const std::string& title)
		: m_hWnd(NULL)
	{
		HINSTANCE hInstance = GetModuleHandleA(NULL);

		if (!s_bRegistered)
		{
			if (RegisterWindowClass(hInstance))
				s_bRegistered = true;
		}

		m_Title = title;

		DWORD style = WS_OVERLAPPEDWINDOW;

		m_hWnd = CreateWindowExA(
			NULL,
			ClassName,
			m_Title.c_str(),
			style,
			CW_USEDEFAULT, CW_USEDEFAULT, // position
			960, 480,                     // dimensions
			NULL,
			NULL,
			hInstance,
			NULL
		);
	}

	Window::~Window()
	{

	}

	void Window::Show()
	{
		ShowWindow(m_hWnd, SW_SHOW);
	}

	void Window::Hide()
	{
		ShowWindow(m_hWnd, SW_HIDE);
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Title = title;
		SetWindowTextA(m_hWnd, title.c_str());
	}

	bool Window::RegisterWindowClass(HINSTANCE hInstance)
	{
		WNDCLASS wc { };
		wc.style = 0;
		wc.lpfnWndProc = Window::WindowProc;
		wc.hInstance = hInstance;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
		wc.lpszClassName = ClassName;

		return RegisterClassA(&wc);
	}

	LRESULT Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			
		}
		return DefWindowProcA(hwnd, uMsg, wParam, lParam);
	}

	bool Window::s_bRegistered = false;
}
