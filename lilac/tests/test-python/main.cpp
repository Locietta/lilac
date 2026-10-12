#include <algorithm>
#include <iostream>
#include <string_view>

#include <lilac/ascii.h>
#include <lilac/document.h>
#include <lilac/language/python.h>
#include <lilac/role.h>

namespace {

using namespace lilac;

[[nodiscard]] bool has_role(const Document &document, std::string_view token, TokenRole role) {
    usize position = 0;
    while ((position = document.source.find(token, position)) != std::string::npos) {
        const bool left_boundary =
            !ascii_identifier_start(token.front()) || position == 0 || !ascii_identifier_continue(document.source[position - 1]);
        const usize token_end = position + token.size();
        const bool right_boundary = !ascii_identifier_continue(token.back()) || token_end == document.source.size() ||
                                    !ascii_identifier_continue(document.source[token_end]);
        if (left_boundary && right_boundary) break;
        ++position;
    }
    if (position == std::string::npos) {
        std::cerr << "missing token: " << token << '\n';
        return false;
    }
    for (usize offset = position; offset < position + token.size(); ++offset) {
        const TokenRole actual = role_for_style<PythonLexer::Style>(document.styles[offset]);
        if (actual != role) {
            std::cerr << "role mismatch for " << token << ": expected " << static_cast<int>(role) << ", got " << static_cast<int>(actual)
                      << '\n';
            return false;
        }
    }
    return true;
}

void lex(Document &document, LexRange range) {
    auto lex_context = context(document, range);
    PythonLexer{}.lex(lex_context);
}

bool test_semantic_styles() {
    Document document;
    assign(document, "from pathlib import Path\n@classmethod\nclass Widget:\n    def render(self, value: int) -> str:\n        "
                     "print(self.name, f\"value={value}\\n\")\n        "
                     "return str(value)\n");
    lex(document, {.begin = 0, .end = document.source.size()});
    return has_role(document, "from", TokenRole::KEYWORD) && has_role(document, "Path", TokenRole::TYPE) &&
           has_role(document, "classmethod", TokenRole::ATTRIBUTE) && has_role(document, "Widget", TokenRole::TYPE) &&
           has_role(document, "render", TokenRole::FUNCTION) && has_role(document, "int", TokenRole::TYPE) &&
           has_role(document, "print", TokenRole::FUNCTION) && has_role(document, "name", TokenRole::PROPERTY) &&
           has_role(document, "\\n", TokenRole::ESCAPE) && has_role(document, "return", TokenRole::KEYWORD);
}

bool test_comments_literals() {
    Document document;
    assign(document, "def greet(name=\"Ada\"):  # welcome\n    return 1\n");
    lex(document, {.begin = 0, .end = document.source.size()});
    return has_role(document, "def", TokenRole::KEYWORD) && has_role(document, "\"Ada\"", TokenRole::STRING) &&
           has_role(document, "# welcome", TokenRole::COMMENT) && has_role(document, "1", TokenRole::NUMBER);
}

bool test_triple_string_streaming() {
    Document document;
    assign(document, "prior = 1\nvalue = r'''first");
    lex(document, {.begin = 0, .end = document.source.size()});
    const LexRange dirty = append(document, "\nsecond'''\nprint(value)");
    if (dirty.begin != document.source.find("value")) return false;
    lex(document, dirty);
    return has_role(document, "first", TokenRole::STRING) && has_role(document, "second", TokenRole::STRING) &&
           has_role(document, "print", TokenRole::FUNCTION);
}

} // namespace

int main() { return test_semantic_styles() && test_comments_literals() && test_triple_string_streaming() ? 0 : 1; }
