#include <stdio.h>
#include <Windows.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <locale.h>

int main(int argc, char** argv) {
    wchar_t value = 252; // ü
    //SetConsoleOutputCP(CP_UTF7);
    //system("chcp 65001");
    
    //setvbuf(stdout, NULL, _IOFBF, 1000);

    setlocale(LC_CTYPE, "");
    wprintf(L"+%lc\n", value);
    return 0;
}