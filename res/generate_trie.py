keywords = [
    "int",
    "let",
    "mut",
    "bool",
    "true",
    "false"
]

tab = "    "


def main(keywords: list[str] = keywords, letter_index: int = 0, indent: str = ""):
    keywords.sort()
    print(f"{indent}switch(lexer_advance(self)) {{" if letter_index > 0 else f"{indent}switch(lexer_prev(self)) {{")
    for n in range(0, 27):
        letter = chr(ord('a') + n)
        if n < 26:
            kw = [keyword for keyword in keywords if letter_index < len(keyword) and keyword[letter_index] == letter]
            if len(kw) > 0:
                print(f"{indent}case '{letter}':")
                if len(kw) == 1:
                    print(f'{indent + tab}if(lexer_add_keyword_token(self, {len(kw[0]) - letter_index - 1}, "{kw[0][letter_index + 1:]}", {"TOKEN_KEYWORD_" + kw[0].upper()})) return;')
                    print(f"{indent + tab}break;")
                else:
                    main(kw, letter_index + 1, indent + tab)
        else:
            kw = [keyword for keyword in keywords if letter_index >= len(keyword)]
            if len(kw) == 1:
                print(f"{indent}default:")
                print(f'{indent + tab}if(lexer_add_keyword_token(self, 0, "", {"TOKEN_KEYWORD_" + kw[0].upper()})) return;')
                print(f"{indent + tab}break;")
    print(f"{indent}}}")


if __name__ == "__main__":
    main()