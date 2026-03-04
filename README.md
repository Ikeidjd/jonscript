### Grammar
program = (decl | stat)*

decl = var_decl <br>
var_decl = ('let' | 'mut') IDENTIFIER ':' type '=' expr ';'

stat = print_stat <br>
print_stat = ('print' | 'println') expr ';'

type = (type_keyword | IDENTIFIER) ('[' type? ']')* <br>
type_keyword = 'int' | 'bool' | 'str'

expr = value bin_op expr <br>
bin_op = '+' | '-' | '*' | '/' | '%' <br>
value = (INT | BOOL | STR | array_literal | '(' expr ')') ('[' expr ']')* <br>
array_literal = '[' (expr ((',' expr)* | ':' expr))? ']'

### Operator precedence
1. *, /, %
2. +, -