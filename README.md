### Grammar
program = decl

decl = var_decl <br>
var_decl = ('let' | 'mut') IDENTIFIER ':' type '=' expr ';'

type = (type_keyword | IDENTIFIER) ('[' type? ']')* <br>
type_keyword = 'int'

expr = value bin_op expr <br>
bin_op = '+' | '-' | '*' | '/' | '%' <br>
value = (INT | array_literal | '(' expr ')') ('[' expr ']')* <br>
array_literal = '[' (expr ((',' expr)* | ':' expr))? ']'

### Operator precedence
1. *, /, %
2. +, -