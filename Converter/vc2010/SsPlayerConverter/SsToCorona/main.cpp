
#ifdef _DEBUG
#pragma comment(lib, "../debug/StaticLib.lib")
#pragma comment(lib, "../lib/libboost_filesystem-vc100-mt-gd-1_51.lib")
#pragma comment(lib, "../lib/libboost_program_options-vc100-mt-gd-1_51.lib")
#pragma comment(lib, "../lib/libboost_system-vc100-mt-gd-1_51.lib")
#else
#pragma comment(lib, "../release/StaticLib.lib")
#pragma comment(lib, "../lib/libboost_filesystem-vc100-mt-1_51.lib")
#pragma comment(lib, "../lib/libboost_program_options-vc100-mt-1_51.lib")
#pragma comment(lib, "../lib/libboost_system-vc100-mt-1_51.lib")
#endif

extern "C" int Converter_SsToCorona(int argc, const char *argv[]);

int main(int argc, const char *argv[])
{
	return Converter_SsToCorona(argc, argv);
}

