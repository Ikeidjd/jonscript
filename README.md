### Grammar
program = decl

decl = var_decl <br>
var_decl = type IDENTIFIER '=' expr ';'

type = (type_keyword | IDENTIFIER) ('[' type? ']')* <br>
type_keyword = 'int'

expr = value bin_op value <br>
bin_op = '+' | '-' | '*' | '/' | '%' <br>
value = INT

### Operator precedence
1. *, /, %
2. +, -