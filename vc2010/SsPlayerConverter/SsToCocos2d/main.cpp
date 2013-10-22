
#ifdef _DEBUG
#pragma comment(lib, "../debug/StaticLib.lib")
#pragma comment(lib, "../../lib-debug/libboost_filesystem-vc100-mt-gd-1_51.lib")
#pragma comment(lib, "../../lib-debug/libboost_program_options-vc100-mt-gd-1_51.lib")
#pragma comment(lib, "../../lib-debug/libboost_system-vc100-mt-gd-1_51.lib")
#else
#pragma comment(lib, "../release/StaticLib.lib")
#pragma comment(lib, "../../lib-release/libboost_filesystem-vc100-mt-1_51.lib")
#pragma comment(lib, "../../lib-release/libboost_program_options-vc100-mt-1_51.lib")
#pragma comment(lib, "../../lib-release/libboost_system-vc100-mt-1_51.lib")
#endif

extern "C" int Converter_SsToCocos2d(int argc, const char *argv[]);

int main(int argc, const char *argv[])
{
	return Converter_SsToCocos2d(argc, argv);
}

