### Grammar
program = expr

expr = value bin_op value
bin_op = '+' | '-' | '*' | '/' | '%'
value = INT

### Operator precedence
1. *, /, %
2. +, -