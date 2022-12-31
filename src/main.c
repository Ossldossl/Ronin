#include "include/misc.h"

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

int main(int argc, char* argv[]) 
{
    compiler_t* com = malloc(sizeof(compiler_t));
    com->file_content   = null;
    com->file_size      = 0;
    com->index          = 0;
    com->line           = 1;
    com->col            = 1;
    com->enable_colors  = init_console();
    com->errors         = vector_new();

    vector_t* file_names = parse_arguments(argc, &argv);
    com->file_path = file_names->data[0];
    com->file_size = open_file(file_names->data[0], &com->file_content);

    lexer_lex_file(com);
    print_errors(com);
    return 0;
}