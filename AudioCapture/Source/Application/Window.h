#pragma once

namespace Application
{
	class Window
	{
	public:
		static const char ClassName[];

		Window();
		Window(const std::string& title);
		~Window();

		void Show();
		void Hide();

		void SetTitle(const std::string& title);

	private:
		static bool RegisterWindowClass(HINSTANCE hInstance);

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_hWnd;
		std::string m_Title;
		
		static bool s_bRegistered;
	};
}
