from enum import Enum
import sys
from lark import Lark

version = "0.0.1"

class TokenKind(Enum):
    ERROR            = 0
    UINT             = 1
    SINT             = 2
    FLOAT            = 3
    PLUS             = 4
    MINUS            = 5
    ASTERISK         = 6
    DIV              = 7
    AMPERSAND        = 8
    LET              = 9
    IF               = 10
    FOR              = 11
    WHILE            = 12
    END              = 13
    DO               = 14
    IMPORT           = 15
    CONST            = 16
    MACRO            = 17
    MATCH            = 18
    FN               = 19
    LPAREN           = 20
    RPAREN           = 21
    LBRACKET         = 22
    RBRACKET         = 23
    LBRACE           = 24
    RBRACE           = 25
    COLON            = 26
    SEMICOLON        = 27
    QUESTION_MARK    = 28
    EXCLAMATION_MARK = 29
    PERIOD           = 30
    COMMA            = 31
    LANGLE           = 32
    RANGLE           = 33
    IDENT            = 34
    TYPE             = 35
    STRUCT           = 36
    STRING           = 37
    TRUE             = 38
    FALSE            = 39
    NULL             = 40
    EQUALS           = 41 #  = 
    GEQ              = 42
    LEQ              = 43
    IS_EQ            = 44 # == 

class Span: 
    col = 0
    line = 0
    file = ""
    def __init__(self, file, line, col):
        self.col = col
        self.line = line
        self.file = file

class Token:
    def __init__(self, kind, file, line, col):
        self.kind = kind
        self.loc = Span(file, line, col)
    
cur_line = 0 
cur_col = 0
cur_file = ""

def lex_file(file):
    with open(file, "r", encoding="utf-8") as f:
        content = f.read()
    cur_file = file
    cur_line = 1
    cur_col = 1
    tokens = []
    cur = 0
    while cur < len(content):
        

def parse(tokens: list[Token]):
    pass

def eval_macros(ast):
    pass

def codegen(ast):
    pass

def print_usage():
    print(f"Ronin compiler {version} by Oskar Behzadpour.\nusage: ronin [compile|test|run] <file>\n\npositional arguments:\n    compile    compile the file to a binary\n    run        interpret the file\n    test       run tests\noptional_arguments:\n    --help     print this message and exit")

def main():
    # Ablauf:
    #   Generate AST
    #   Evaluate Macros
    #   Compile Rest
    if len(sys.argv) < 3 or ("--help" in sys.argv):
        print_usage()
        exit()
    
    mode = sys.argv[1]
    file = sys.argv[2]
    
    tokens = lex_file(file)
    ast = parse(tokens)
    new_ast = eval_macros(ast)
    ir = codegen(new_ast)
        
    if mode == "compile":
        print("Running is not implemented yet!")
        exit()
    elif mode == "run":
        print("Running is not implemented yet!")
        exit()
    elif mode == "test":
        print("Testing is not implemented yet!")
        exit()
    else:
        print_usage()
        exit()

if __name__ == "__main__": 
    main()