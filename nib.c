#define NIB_IMPLEMENTATION
#include "external/nib.h"

#define COMPILE_FLAGS		"-Wall", "-Wextra", "-O2"

int main(void)
{
	CMD cmd = { 0 };

	#ifdef _WIN32
	#define WINDOWS_LIBS 	"-lopengl32", "-lgdi32", "-lwinmm"
	nib_cmd_append(&cmd, "gcc", COMPILE_FLAGS, "-I./external/raylib-5.5_win64_mingw-w64/include/", "-o", "minivix.exe", "./src/miniviz.c", "./external/raylib-5.5_win64_mingw-w64/lib/libraylib.a", WINDOWS_LIBS);
	#else
	#define LD_FLAGS			"-lm", "-lasound", "-lpthread", "-ldl"
	nib_cmd_append(&cmd, "gcc", COMPILE_FLAGS, "-I./external/raylib-5.5_linux_amd64/include/", "-o", "miniviz", "./src/miniviz.c", "./external/raylib-5.5_linux_amd64/lib/libraylib.a", LD_FLAGS);
	#endif

	if(!nib_cmd_run(cmd))		return 1;
	return 0;
}
