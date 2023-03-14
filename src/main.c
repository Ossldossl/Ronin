#include "include/misc.h"
#include <stdint.h>
#include <stdlib.h>


void print_usage(void) 
{
	print_message(COLOR_BLUE, "=== PLANG COMPILER ===\n");
	print_message(COLOR_BLUE, "Usage: ronin <FILE>* \n");
}

vector_t* parse_arguments(int argc, char*** argv) // pointer to array of pointers
{
    if (argc == 1) {
        print_usage();
        panic("\n");
    }
    vector_t* result_vector = vector_new();
    for (int i = 1; i < argc; ++i) {
        vector_push(result_vector, (*argv)[i]);
    }

    return result_vector;
}

static arena_allocator_t* parse_file_content_to_utf8(char* raw_file_content, int file_size)
{
    arena_allocator_t* result = arena_new(sizeof(rune), file_size * 0.7);
    
    int i = 0;
    char c = -1;
    
    int mask_1 = 0xf;
    int mask_2 = (-0xc0 - 1);

    // skip utf-8 sequence
    bool had_seq = false;
    char fst_byte = raw_file_content[0];
    char snd_byte = raw_file_content[1];
    char trd_byte = raw_file_content[2];
    if (fst_byte == '\xEF' && snd_byte == '\xBB' && trd_byte == '\xBF')
	{
		i += 3;
        had_seq = true;
	}

    while (true) {
        c = raw_file_content[i];
        if (c == '\0') break;

        if ((c & 0x80) == 0) {
            rune* value = arena_alloc(result);
            *value = c;
            i++;
            continue;
        }
        
        // checks if byte starts with 110xxxxx; 11000000 => 0xc0
        //                          & 11100000 => 0xe0
        if ((c & 0xe0) == 0xc0) { 
            rune* value = arena_alloc(result);
            *value = 0;
            uint8_t temp = 0;
            i++;
            char second_byte = raw_file_content[i];
            // checks if second byte starts with 10xxxxxx; 10000000 => 0x80
            //                                 & 11000000 => 0xc0
            if ((second_byte & 0xc0) == 0x80) {
                *value = c & mask_1;
                *value <<= 6;
                temp = second_byte & mask_2; // needed because of negative numbers exploding on unsigned 16 bit integers
                *value += temp;
                if (*value <= 0x7FF - 0x80) { i++; continue; }
            }
        }

        // checks if byte starts with 1110xxxx; 11100000 => 0xe0
        //                          & 11110000 => 0xf0
        if ((c & 0xf0) == 0xe0) { 
            print_message(COLOR_RED, "Fatal error: Unicode symbols over U+ 07FF are not supported!\n");
            /*rune* value = arena_alloc(result);
            i++;
            char second_byte = raw_file_content[i];
            i++;
            char third_byte = raw_file_content[i];
            // checks if second byte starts with 10xxxxxx; 10 => 0x2
            //                                 & 11000000 => 0xc0
            if ((second_byte & 0xc0) != 2 && (third_byte & 0xc0) != 2) {
                *value = c & mask_1;
                *value <<= 6;
                uint8_t temp = second_byte & mask_2; // s.o.
                *value += temp;
                *value <<= 6;
                temp = 0;
                temp = third_byte & mask_2; // s.o.
                *value += temp;
                if (*value <= 0xFFFF - 0x800) { i++; continue; }
            }*/
        }
        print_message(COLOR_RED, "error: Invalid utf8 character at %d! This shouldn't happen!", had_seq ? i - 3 : i);
        panic("\n");
    }

#ifdef PRINT_DEBUGS_
    for (int i = 0; i < result->index; i++) {
        printf("%lc : %d \n", *(rune*)arena_get(result, i), *(rune*)arena_get(result, i));
    }
#endif
    rune* escape = arena_alloc(result);
    *escape = '\0';

    return result;
}

int main(int argc, char* argv[]) 
{
    compiler_t* com = malloc(sizeof(compiler_t));
    com->utf8_file_content = null;
    com->token_t_allocator = null;
    com->node_allocator    = null;
    com->index             = 0;
    com->line              = 1;
    com->col               = 0;
    com->enable_colors     = init_console();
    com->errors            = vector_new();

    vector_t* file_names       = parse_arguments(argc, &argv);
              com->file_path   = file_names->data[0];
    char*     raw_file_content = null;
    int       raw_file_size    = open_file(file_names->data[0], &raw_file_content);

    com->utf8_file_content = parse_file_content_to_utf8(raw_file_content, raw_file_size);

    lexer_lex_file(com);

    if (com->errors->used > 0) {
        print_errors(com);
    } else {
        parser_parse_tokens(com);
        print_errors(com);
    }
    return 0;
}