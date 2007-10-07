// main.cpp
// 

#include <Windows.h>
#include <string>


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {
	char buf[4096] = {'\0'};

	size_t sz = GetModuleFileNameA(hInstance, buf, 4096);
	std::string path(buf);
	path.erase(path.find_last_of("/\\"));

	std::string gtk_base   = path + "\\environ\\gtk";
	std::string pygtk_path = path + "\\environ\\pylibs";
	std::string sys_path   = gtk_base + "\\bin;%PATH%";

	::SetEnvironmentVariableA("GTK_BASE",   gtk_base.c_str());
	::SetEnvironmentVariableA("GTKMM_BASE", gtk_base.c_str());
	::SetEnvironmentVariableA("PATH",       sys_path.c_str());
	::SetEnvironmentVariableA("PYTHONPATH", pygtk_path.c_str());

	::ShellExecuteA(0, "open", "_ljedit.exe", 0, path.c_str(), SW_SHOWNORMAL);

	return 0;
}

