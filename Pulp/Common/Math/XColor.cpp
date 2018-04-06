#include "EngineCommon.h"

#include "String\Lexer.h"

template<>
bool ColorT<float>::fromString(const char* pBegin, const char* pEnd, ColorT<float>& out, bool Slient)
{
    Color col;
    int i;

    core::XLexer lex(pBegin, pEnd);
    core::XLexToken token;

    for (i = 0; i < 4; i++) {
        if (lex.ReadToken(token)) {
            if (token.GetType() == core::TokenType::NUMBER) {
                col[i] = token.GetFloatValue();
            }
            else {
                X_ERROR_IF(!Slient, "Color", "failed to set color, invalid input");
                return false;
            }
        }
        else {
            if (i == 1) {
                // we allow all 4 colors to be set with 1 color.
                // could cap here for a saving if you really wish :P
                col.g = col.r;
                col.b = col.r;
                col.a = col.r;
                break;
            }
            if (i == 3) {
                // solid alpha.
                col.a = 1.f;
            }
            else {
                X_ERROR_IF(!Slient, "Color", "failed to set color, require either 1 or 4 real numbers");
                return false;
            }
        }
    }

    // cap any values.
    col.r = math<float>::clamp(col.r, 0.f, 1.f);
    col.g = math<float>::clamp(col.g, 0.f, 1.f);
    col.b = math<float>::clamp(col.b, 0.f, 1.f);
    col.a = math<float>::clamp(col.a, 0.f, 1.f);

    out = col;
    return true;
}
