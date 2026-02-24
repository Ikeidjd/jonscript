### Grammar
program = decl

decl = var_decl
var_decl = type IDENTIFIER '=' expr ';'

type = (type_keyword | IDENTIFIER) ('[' type? ']')*
type_keyword = 'int'

expr = value bin_op value
bin_op = '+' | '-' | '*' | '/' | '%'
value = INT

### Operator precedence
1. *, /, %
2. +, -