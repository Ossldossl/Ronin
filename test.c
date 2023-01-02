#include <stdio.h>
#include <Windows.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>

#define _UNICODE

    //SetConsoleCP(CP_UTF7);
    //_setmode(_fileno(stdout), _O_U16TEXT);
    /*const char mb_str[] = { '\xC3', '\xBC'};

    wchar_t value = 0;
    int mask_1 = 0xf;
    int mask_2 = (-0xc0 - 1);

    if ((mb_str[0] & 0xe0) == 0xc0) { 
        char second_byte = mb_str[1];
        // checks if second byte starts with 10xxxxxx; 10000000 => 0x80
        //                                 & 11000000 => 0xc0
        if ((second_byte & 0xc0) == 0x80) {
            value = mb_str[0] & mask_1;
            value <<= 6;
            value += second_byte & mask_2; // two's complement
            if (value <= 0x7FF) {
            }
            wprintf(L"%c", value);
        }
    }
    wprintf(L"%d", value);*/
int main(int argc, char** argv) {
    wchar_t value = 252; // ü
    //SetConsoleOutputCP(CP_UTF7);
    //system("chcp 65001");
    
    //setvbuf(stdout, NULL, _IOFBF, 1000);

    wprintf(L"\xC3\xBC\n", value);
    return 0;
}