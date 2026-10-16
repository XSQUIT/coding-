// phoenix.c -- Phoenix (.phx) reference interpreter, v0.1
// A tree-walking interpreter for the .phx sandbox dialect of Phoenix.
// Implements: C-brace syntax, fixed-width integer wraparound, struct
// layout resolution (sizeof/offsetof parity with C), and a generational
// pointer model for alloc()/ptr()/free() inside unsafe { } blocks.
//
// Build:   gcc -O2 -o phoenix phoenix.c
// Run:     ./phoenix examples/overflow.phx

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

// ---------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------

static void die(const char *fmt, ...) __attribute__((noreturn));
static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "phoenix: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static char *xstrndup(const char *s, size_t n) {
    char *p = malloc(n + 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

// ---------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------

typedef enum {
    T_EOF, T_IDENT, T_INT, T_STRING,
    T_LET, T_FN, T_IF, T_ELSE, T_WHILE, T_RETURN, T_STRUCT, T_UNSAFE,
    T_PRINT, T_ALLOC, T_FREE, T_SIZEOF, T_OFFSETOF, T_TRUE, T_FALSE,
    T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_SEMI, T_COLON, T_COMMA, T_ARROW,
    T_ASSIGN, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_EQ, T_NEQ, T_LT, T_GT, T_LE, T_GE, T_AND, T_OR, T_NOT, T_DOT
} TokType;

typedef struct {
    TokType type;
    char *text;   // for T_IDENT (also holds type-keywords like i32, u8)
    int64_t ival;
    int line;
} Token;

typedef struct {
    const char *src;
    size_t pos, len;
    int line;
    bool pragma_unsafe_uninit;
    bool pragma_unsafe_deref;
} Lexer;

static Lexer g_lex;

static int peekc(void) { return g_lex.pos < g_lex.len ? g_lex.src[g_lex.pos] : -1; }
static int nextc(void) {
    if (g_lex.pos >= g_lex.len) return -1;
    int c = g_lex.src[g_lex.pos++];
    if (c == '\n') g_lex.line++;
    return c;
}

static const struct { const char *kw; TokType t; } keywords[] = {
    {"let", T_LET}, {"fn", T_FN}, {"if", T_IF}, {"else", T_ELSE},
    {"while", T_WHILE}, {"return", T_RETURN}, {"struct", T_STRUCT},
    {"unsafe", T_UNSAFE}, {"print", T_PRINT}, {"alloc", T_ALLOC},
    {"free", T_FREE}, {"sizeof", T_SIZEOF}, {"offsetof", T_OFFSETOF},
    {"true", T_TRUE}, {"false", T_FALSE}, {NULL, 0}
};

static Token lex_next(void) {
    for (;;) {
        int c = peekc();
        if (c == -1) return (Token){T_EOF, NULL, 0, g_lex.line};
        if (isspace(c)) { nextc(); continue; }
        if (c == '/' && g_lex.pos + 1 < g_lex.len && g_lex.src[g_lex.pos+1] == '/') {
            while (peekc() != -1 && peekc() != '\n') nextc();
            continue;
        }
        if (c == '#') { // #pragma line
            size_t start = g_lex.pos;
            while (peekc() != -1 && peekc() != '\n') nextc();
            char *line = xstrndup(g_lex.src + start, g_lex.pos - start);
            if (strstr(line, "unsafe_uninit")) g_lex.pragma_unsafe_uninit = true;
            if (strstr(line, "unsafe_deref")) g_lex.pragma_unsafe_deref = true;
            free(line);
            continue;
        }
        break;
    }
    int line = g_lex.line;
    int c = nextc();
    if (c == '"') {
        char *buf = malloc(g_lex.len - g_lex.pos + 1);
        size_t n = 0;
        for (;;) {
            int ch = nextc();
            if (ch == -1) die("line %d: unterminated string literal", line);
            if (ch == '"') break;
            if (ch == '\\') {
                int esc = nextc();
                switch (esc) {
                    case 'n': buf[n++] = '\n'; break;
                    case 't': buf[n++] = '\t'; break;
                    case '\\': buf[n++] = '\\'; break;
                    case '"': buf[n++] = '"'; break;
                    case '0': buf[n++] = '\0'; break;
                    default: buf[n++] = (char)esc; break;
                }
            } else {
                buf[n++] = (char)ch;
            }
        }
        buf[n] = 0;
        return (Token){T_STRING, buf, 0, line};
    }
    if (isalpha(c) || c == '_') {
        size_t start = g_lex.pos - 1;
        while (isalnum(peekc()) || peekc() == '_') nextc();
        char *text = xstrndup(g_lex.src + start, g_lex.pos - start);
        for (int i = 0; keywords[i].kw; i++)
            if (strcmp(text, keywords[i].kw) == 0) return (Token){keywords[i].t, text, 0, line};
        return (Token){T_IDENT, text, 0, line};
    }
    if (isdigit(c)) {
        size_t start = g_lex.pos - 1;
        while (isdigit(peekc())) nextc();
        char *text = xstrndup(g_lex.src + start, g_lex.pos - start);
        int64_t v = strtoll(text, NULL, 10);
        return (Token){T_INT, text, v, line};
    }
    switch (c) {
        case '{': return (Token){T_LBRACE, NULL, 0, line};
        case '}': return (Token){T_RBRACE, NULL, 0, line};
        case '(': return (Token){T_LPAREN, NULL, 0, line};
        case ')': return (Token){T_RPAREN, NULL, 0, line};
        case ';': return (Token){T_SEMI, NULL, 0, line};
        case ':': return (Token){T_COLON, NULL, 0, line};
        case ',': return (Token){T_COMMA, NULL, 0, line};
        case '.': return (Token){T_DOT, NULL, 0, line};
        case '+': return (Token){T_PLUS, NULL, 0, line};
        case '-':
            if (peekc() == '>') { nextc(); return (Token){T_ARROW, NULL, 0, line}; }
            return (Token){T_MINUS, NULL, 0, line};
        case '*': return (Token){T_STAR, NULL, 0, line};
        case '/': return (Token){T_SLASH, NULL, 0, line};
        case '%': return (Token){T_PERCENT, NULL, 0, line};
        case '!':
            if (peekc() == '=') { nextc(); return (Token){T_NEQ, NULL, 0, line}; }
            return (Token){T_NOT, NULL, 0, line};
        case '=':
            if (peekc() == '=') { nextc(); return (Token){T_EQ, NULL, 0, line}; }
            return (Token){T_ASSIGN, NULL, 0, line};
        case '<':
            if (peekc() == '=') { nextc(); return (Token){T_LE, NULL, 0, line}; }
            return (Token){T_LT, NULL, 0, line};
        case '>':
            if (peekc() == '=') { nextc(); return (Token){T_GE, NULL, 0, line}; }
            return (Token){T_GT, NULL, 0, line};
        case '&':
            if (peekc() == '&') { nextc(); return (Token){T_AND, NULL, 0, line}; }
            break;
        case '|':
            if (peekc() == '|') { nextc(); return (Token){T_OR, NULL, 0, line}; }
            break;
    }
    die("line %d: unexpected character '%c'", line, c);
}

// ---------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------

typedef enum {
    TY_I8, TY_U8, TY_I16, TY_U16, TY_I32, TY_U32, TY_I64, TY_U64,
    TY_BOOL, TY_VOID, TY_STRUCT, TY_PTR, TY_STRING
} BaseType;

typedef struct StructDef StructDef;

typedef struct Type {
    BaseType base;
    StructDef *sdef;     // for TY_STRUCT
    struct Type *pointee; // for TY_PTR
} Type;

typedef struct {
    char *name;
    Type type;
    int offset; // computed by layout resolver
} FieldDef;

struct StructDef {
    char *name;
    FieldDef *fields;
    int nfields;
    int size;
    int align;
    StructDef *next;
};

static StructDef *g_structs = NULL;

static int base_size(BaseType b) {
    switch (b) {
        case TY_I8: case TY_U8: case TY_BOOL: return 1;
        case TY_I16: case TY_U16: return 2;
        case TY_I32: case TY_U32: return 4;
        case TY_I64: case TY_U64: return 8;
        case TY_PTR: return 8;
        case TY_STRING: return 8;
        default: return 0;
    }
}
static int type_size(Type t) {
    if (t.base == TY_STRUCT) return t.sdef->size;
    return base_size(t.base);
}
static int type_align(Type t) {
    if (t.base == TY_STRUCT) return t.sdef->align;
    return base_size(t.base);
}

static BaseType parse_base_type_name(const char *name) {
    if (!strcmp(name, "i8")) return TY_I8;
    if (!strcmp(name, "u8")) return TY_U8;
    if (!strcmp(name, "i16")) return TY_I16;
    if (!strcmp(name, "u16")) return TY_U16;
    if (!strcmp(name, "i32")) return TY_I32;
    if (!strcmp(name, "u32")) return TY_U32;
    if (!strcmp(name, "i64")) return TY_I64;
    if (!strcmp(name, "u64")) return TY_U64;
    if (!strcmp(name, "bool")) return TY_BOOL;
    if (!strcmp(name, "void")) return TY_VOID;
    if (!strcmp(name, "ptr")) return TY_PTR;
    if (!strcmp(name, "string")) return TY_STRING;
    return TY_VOID; // caller checks structs separately
}

static StructDef *find_struct(const char *name) {
    for (StructDef *s = g_structs; s; s = s->next)
        if (!strcmp(s->name, name)) return s;
    return NULL;
}

// Resolve layout the same way a typical C compiler does: each field is
// aligned to its own natural alignment, and the struct's total size is
// padded up to a multiple of its largest member alignment.
static void resolve_layout(StructDef *s) {
    int offset = 0, max_align = 1;
    for (int i = 0; i < s->nfields; i++) {
        int a = type_align(s->fields[i].type);
        if (a > max_align) max_align = a;
        if (offset % a != 0) offset += a - (offset % a);
        s->fields[i].offset = offset;
        offset += type_size(s->fields[i].type);
    }
    if (max_align > 0 && offset % max_align != 0) offset += max_align - (offset % max_align);
    s->size = offset;
    s->align = max_align;
}

// ---------------------------------------------------------------------
// AST
// ---------------------------------------------------------------------

typedef enum {
    E_INT, E_BOOL, E_STRING, E_IDENT, E_BINOP, E_UNOP, E_CALL, E_FIELD, E_PTRVAL,
    E_STRUCTLIT, E_ALLOC, E_PTR, E_SIZEOF, E_OFFSETOF
} ExprKind;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef struct { char *name; Expr *value; } FieldInit;

struct Expr {
    ExprKind kind;
    int line;
    int64_t ival;
    bool bval;
    char *name;       // ident / field name / struct type name
    Expr *lhs, *rhs;  // binop/unop operands, field base
    TokType op;
    char *callee;     // function name
    Expr **args; int nargs;
    FieldInit *finits; int nfinits; // struct literal
    Type alloc_type;  // for E_ALLOC
    char *field2;     // offsetof's field name
};

typedef enum {
    S_LET, S_ASSIGN, S_FIELD_ASSIGN, S_PTR_ASSIGN, S_IF, S_WHILE, S_RETURN,
    S_EXPR, S_PRINT, S_BLOCK, S_UNSAFE, S_FREE
} StmtKind;

struct Stmt {
    StmtKind kind;
    int line;
    char *name;         // let/assign target
    Type let_type;
    Expr *expr, *cond, *rhs;
    Stmt **body; int nbody;      // block
    Stmt **elsebody; int nelse;  // if-else
    Stmt *next;
};

typedef struct {
    char *name; Type type;
} Param;

typedef struct FnDef {
    char *name;
    Param *params; int nparams;
    Type ret_type;
    Stmt **body; int nbody;
    struct FnDef *next;
} FnDef;

static FnDef *g_fns = NULL;

// ---------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------

static Token g_cur;
static int g_unsafe_depth = 0; // parse-time tracking of unsafe{} nesting
static void advance(void) { g_cur = lex_next(); }
static bool check(TokType t) { return g_cur.type == t; }
static Token expect(TokType t, const char *what) {
    if (g_cur.type != t) die("line %d: expected %s", g_cur.line, what);
    Token tok = g_cur;
    advance();
    return tok;
}

static Type parse_type(void) {
    Token tok = expect(T_IDENT, "type name");
    Type t = {0};
    StructDef *sd = find_struct(tok.text);
    if (sd) { t.base = TY_STRUCT; t.sdef = sd; return t; }
    t.base = parse_base_type_name(tok.text);
    return t;
}

static Expr *parse_expr(void);

static Expr *new_expr(ExprKind k) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = k;
    e->line = g_cur.line;
    return e;
}

static Expr *parse_args_call(char *name) {
    Expr *e = new_expr(E_CALL);
    e->callee = name;
    expect(T_LPAREN, "(");
    Expr **args = NULL; int n = 0;
    while (!check(T_RPAREN)) {
        args = realloc(args, sizeof(Expr*) * (n+1));
        args[n++] = parse_expr();
        if (check(T_COMMA)) advance(); else break;
    }
    expect(T_RPAREN, ")");
    e->args = args; e->nargs = n;
    return e;
}

static Expr *parse_primary(void) {
    int line = g_cur.line;
    if (check(T_INT)) {
        Expr *e = new_expr(E_INT); e->ival = g_cur.ival; advance(); return e;
    }
    if (check(T_TRUE)) { advance(); Expr *e = new_expr(E_BOOL); e->bval = true; return e; }
    if (check(T_FALSE)) { advance(); Expr *e = new_expr(E_BOOL); e->bval = false; return e; }
    if (check(T_STRING)) {
        Expr *e = new_expr(E_STRING); e->name = g_cur.text; advance(); return e;
    }
    if (check(T_LPAREN)) {
        advance();
        Expr *e = parse_expr();
        expect(T_RPAREN, ")");
        return e;
    }
    if (check(T_ALLOC)) {
        if (g_unsafe_depth == 0) die("line %d: alloc() is only allowed inside an unsafe { } block", g_cur.line);
        advance();
        expect(T_LPAREN, "(");
        Expr *e = new_expr(E_ALLOC);
        e->alloc_type = parse_type();
        expect(T_RPAREN, ")");
        return e;
    }
    if (check(T_SIZEOF)) {
        advance();
        expect(T_LPAREN, "(");
        Expr *e = new_expr(E_SIZEOF);
        Token tok = expect(T_IDENT, "type name");
        e->name = tok.text;
        expect(T_RPAREN, ")");
        return e;
    }
    if (check(T_OFFSETOF)) {
        advance();
        expect(T_LPAREN, "(");
        Expr *e = new_expr(E_OFFSETOF);
        Token tok = expect(T_IDENT, "struct name");
        e->name = tok.text;
        expect(T_COMMA, ",");
        Token f = expect(T_IDENT, "field name");
        e->field2 = f.text;
        expect(T_RPAREN, ")");
        return e;
    }
    if (check(T_IDENT)) {
        char *name = g_cur.text;
        advance();
        if (check(T_LPAREN)) {
            return parse_args_call(name);
        }
        if (check(T_LBRACE) && find_struct(name)) {
            // struct literal: Name { field: expr, ... }
            advance();
            Expr *e = new_expr(E_STRUCTLIT);
            e->name = name;
            FieldInit *fi = NULL; int n = 0;
            while (!check(T_RBRACE)) {
                Token fname = expect(T_IDENT, "field name");
                expect(T_COLON, ":");
                Expr *val = parse_expr();
                fi = realloc(fi, sizeof(FieldInit) * (n+1));
                fi[n].name = fname.text; fi[n].value = val; n++;
                if (check(T_COMMA)) advance(); else break;
            }
            expect(T_RBRACE, "}");
            e->finits = fi; e->nfinits = n;
            return e;
        }
        Expr *e = new_expr(E_IDENT);
        e->name = name;
        e->line = line;
        return e;
    }
    die("line %d: unexpected token in expression", g_cur.line);
}

static Expr *parse_postfix(void) {
    Expr *e = parse_primary();
    for (;;) {
        if (check(T_DOT)) {
            advance();
            if (check(T_IDENT) && !strcmp(g_cur.text, "value")) {
                if (g_unsafe_depth == 0) die("line %d: '.value' pointer access is only allowed inside an unsafe { } block", g_cur.line);
                advance();
                Expr *pv = new_expr(E_PTRVAL);
                pv->lhs = e;
                e = pv;
            } else {
                Token f = expect(T_IDENT, "field name");
                Expr *fe = new_expr(E_FIELD);
                fe->lhs = e; fe->name = f.text;
                e = fe;
            }
        } else break;
    }
    return e;
}

static Expr *parse_unary(void) {
    if (check(T_MINUS) || check(T_NOT)) {
        TokType op = g_cur.type; advance();
        Expr *e = new_expr(E_UNOP);
        e->op = op; e->lhs = parse_unary();
        return e;
    }
    return parse_postfix();
}

static Expr *parse_binop_level(int level);

static int prec(TokType t) {
    switch (t) {
        case T_STAR: case T_SLASH: case T_PERCENT: return 5;
        case T_PLUS: case T_MINUS: return 4;
        case T_LT: case T_GT: case T_LE: case T_GE: return 3;
        case T_EQ: case T_NEQ: return 2;
        case T_AND: return 1;
        case T_OR: return 0;
        default: return -1;
    }
}

static Expr *parse_binop_level(int level) {
    if (level < 0) return parse_unary();
    Expr *lhs = parse_binop_level(level - 1);
    while (prec(g_cur.type) == level) {
        TokType op = g_cur.type;
        advance();
        Expr *rhs = parse_binop_level(level - 1);
        Expr *e = new_expr(E_BINOP);
        e->op = op; e->lhs = lhs; e->rhs = rhs;
        lhs = e;
    }
    return lhs;
}

static Expr *parse_expr(void) { return parse_binop_level(5); }

static Stmt *new_stmt(StmtKind k) {
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = k;
    s->line = g_cur.line;
    return s;
}

static Stmt *parse_stmt(void);

static void parse_block_into(Stmt ***out, int *nout) {
    expect(T_LBRACE, "{");
    Stmt **body = NULL; int n = 0;
    while (!check(T_RBRACE)) {
        body = realloc(body, sizeof(Stmt*) * (n+1));
        body[n++] = parse_stmt();
    }
    expect(T_RBRACE, "}");
    *out = body; *nout = n;
}

static Stmt *parse_stmt(void) {
    if (check(T_LET)) {
        advance();
        Token name = expect(T_IDENT, "variable name");
        expect(T_COLON, ":");
        Type t = parse_type();
        Stmt *s = new_stmt(S_LET);
        s->name = name.text;
        s->let_type = t;
        if (check(T_ASSIGN)) { advance(); s->expr = parse_expr(); }
        expect(T_SEMI, ";");
        return s;
    }
    if (check(T_IF)) {
        advance();
        expect(T_LPAREN, "(");
        Expr *cond = parse_expr();
        expect(T_RPAREN, ")");
        Stmt *s = new_stmt(S_IF);
        s->cond = cond;
        parse_block_into(&s->body, &s->nbody);
        if (check(T_ELSE)) {
            advance();
            parse_block_into(&s->elsebody, &s->nelse);
        }
        return s;
    }
    if (check(T_WHILE)) {
        advance();
        expect(T_LPAREN, "(");
        Expr *cond = parse_expr();
        expect(T_RPAREN, ")");
        Stmt *s = new_stmt(S_WHILE);
        s->cond = cond;
        parse_block_into(&s->body, &s->nbody);
        return s;
    }
    if (check(T_RETURN)) {
        advance();
        Stmt *s = new_stmt(S_RETURN);
        if (!check(T_SEMI)) s->expr = parse_expr();
        expect(T_SEMI, ";");
        return s;
    }
    if (check(T_PRINT)) {
        advance();
        expect(T_LPAREN, "(");
        Stmt *s = new_stmt(S_PRINT);
        s->expr = parse_expr();
        expect(T_RPAREN, ")");
        expect(T_SEMI, ";");
        return s;
    }
    if (check(T_FREE)) {
        advance();
        if (g_unsafe_depth == 0) die("line %d: free() is only allowed inside an unsafe { } block", g_cur.line);
        expect(T_LPAREN, "(");
        Stmt *s = new_stmt(S_FREE);
        s->expr = parse_expr();
        expect(T_RPAREN, ")");
        expect(T_SEMI, ";");
        return s;
    }
    if (check(T_UNSAFE)) {
        advance();
        Stmt *s = new_stmt(S_UNSAFE);
        g_unsafe_depth++;
        parse_block_into(&s->body, &s->nbody);
        g_unsafe_depth--;
        return s;
    }
    if (check(T_LBRACE)) {
        Stmt *s = new_stmt(S_BLOCK);
        parse_block_into(&s->body, &s->nbody);
        return s;
    }
    // assignment or expr-statement or field/ptr assignment
    if (check(T_IDENT)) {
        char *name = g_cur.text;
        int line = g_cur.line;
        advance();
        if (check(T_ASSIGN)) {
            advance();
            Stmt *s = new_stmt(S_ASSIGN);
            s->name = name; s->line = line;
            s->expr = parse_expr();
            expect(T_SEMI, ";");
            return s;
        }
        if (check(T_DOT)) {
            // could be name.value = expr  or name.field = expr (not needed for v1 assign,
            // but support pointer-value assignment which is the important quirk case)
            advance();
            if (check(T_IDENT) && !strcmp(g_cur.text, "value")) {
                if (g_unsafe_depth == 0) die("line %d: '.value' pointer access is only allowed inside an unsafe { } block", g_cur.line);
                advance();
                expect(T_ASSIGN, "=");
                Stmt *s = new_stmt(S_PTR_ASSIGN);
                s->name = name; s->line = line;
                s->expr = parse_expr();
                expect(T_SEMI, ";");
                return s;
            } else {
                Token f = expect(T_IDENT, "field name");
                expect(T_ASSIGN, "=");
                Stmt *s = new_stmt(S_FIELD_ASSIGN);
                s->name = name; s->line = line;
                s->expr = parse_expr();
                // stash the field name via a synthetic E_IDENT node in 'cond'
                Expr *fname = new_expr(E_IDENT); fname->name = f.text;
                s->cond = fname;
                expect(T_SEMI, ";");
                return s;
            }
        }
        // plain expr statement (function call etc) - rebuild an ident-led expr then continue postfix/binop
        Expr *base = new_expr(E_IDENT); base->name = name; base->line = line;
        // handle call
        Expr *e;
        if (check(T_LPAREN)) {
            e = parse_args_call(name);
        } else {
            e = base;
        }
        // allow postfix/binops after in case of bare expr statement
        Stmt *s = new_stmt(S_EXPR);
        s->expr = e;
        expect(T_SEMI, ";");
        return s;
    }
    die("line %d: unexpected token starting statement", g_cur.line);
}

static void parse_struct_decl(void) {
    expect(T_STRUCT, "struct");
    Token name = expect(T_IDENT, "struct name");
    expect(T_LBRACE, "{");
    FieldDef *fields = NULL; int n = 0;
    while (!check(T_RBRACE)) {
        Token fname = expect(T_IDENT, "field name");
        expect(T_COLON, ":");
        Type t = parse_type();
        fields = realloc(fields, sizeof(FieldDef) * (n+1));
        fields[n].name = fname.text; fields[n].type = t; fields[n].offset = 0;
        n++;
        if (check(T_COMMA)) advance();
    }
    expect(T_RBRACE, "}");
    StructDef *sd = calloc(1, sizeof(StructDef));
    sd->name = name.text; sd->fields = fields; sd->nfields = n;
    resolve_layout(sd);
    sd->next = g_structs;
    g_structs = sd;
}

static void parse_fn_decl(void) {
    expect(T_FN, "fn");
    Token name = expect(T_IDENT, "function name");
    expect(T_LPAREN, "(");
    Param *params = NULL; int n = 0;
    while (!check(T_RPAREN)) {
        Token pname = expect(T_IDENT, "param name");
        expect(T_COLON, ":");
        Type t = parse_type();
        params = realloc(params, sizeof(Param) * (n+1));
        params[n].name = pname.text; params[n].type = t; n++;
        if (check(T_COMMA)) advance();
    }
    expect(T_RPAREN, ")");
    Type ret = {TY_VOID};
    if (check(T_ARROW)) { advance(); ret = parse_type(); }
    FnDef *fn = calloc(1, sizeof(FnDef));
    fn->name = name.text; fn->params = params; fn->nparams = n; fn->ret_type = ret;
    parse_block_into(&fn->body, &fn->nbody);
    fn->next = g_fns;
    g_fns = fn;
}

static void parse_program(const char *src) {
    g_lex.src = src; g_lex.len = strlen(src); g_lex.pos = 0; g_lex.line = 1;
    g_lex.pragma_unsafe_uninit = false; g_lex.pragma_unsafe_deref = false;
    advance();
    while (!check(T_EOF)) {
        if (check(T_STRUCT)) parse_struct_decl();
        else if (check(T_FN)) parse_fn_decl();
        else die("line %d: expected 'struct' or 'fn' at top level", g_cur.line);
    }
}

// ---------------------------------------------------------------------
// Runtime values
// ---------------------------------------------------------------------

typedef struct StructVal StructVal;

typedef struct {
    uint64_t slot_id;
    uint32_t generation;
} PtrVal;

typedef struct Value {
    Type type;
    int64_t i;          // integer/bool storage (truncated per type)
    StructVal *sv;       // struct storage
    PtrVal ptr;          // pointer storage
    char *sval;          // string storage
} Value;

struct StructVal {
    StructDef *def;
    Value *fields; // parallel to def->fields
};

// generational memory slot table
typedef struct Slot {
    uint64_t id;
    uint32_t generation;
    Type elem_type;
    Value data;
    bool freed;
    bool in_use;
    int alloc_line;
    int free_line;
} Slot;

#define MAX_SLOTS 4096
static Slot g_slots[MAX_SLOTS];
static uint64_t g_next_slot = 1;

static Slot *slot_lookup(uint64_t id) {
    for (int i = 0; i < MAX_SLOTS; i++)
        if (g_slots[i].in_use && g_slots[i].id == id) return &g_slots[i];
    return NULL;
}

static Value make_int(Type t, int64_t v) {
    Value val = {0};
    val.type = t;
    int sz = base_size(t.base);
    uint64_t mask = (sz >= 8) ? ~0ULL : ((1ULL << (sz*8)) - 1);
    uint64_t truncated = (uint64_t)v & mask;
    bool is_signed = (t.base == TY_I8 || t.base == TY_I16 || t.base == TY_I32 || t.base == TY_I64);
    if (is_signed && sz < 8) {
        uint64_t signbit = 1ULL << (sz*8 - 1);
        if (truncated & signbit) truncated |= ~mask; // sign-extend into full 64 bits for storage
    }
    val.i = (int64_t)truncated;
    return val;
}

static Value make_bool(bool b) { Value v = {0}; v.type.base = TY_BOOL; v.i = b; return v; }

// ---------------------------------------------------------------------
// Environment
// ---------------------------------------------------------------------

typedef struct VarBinding { char *name; Value val; struct VarBinding *next; } VarBinding;
typedef struct Env { VarBinding *vars; struct Env *parent; } Env;

static Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent;
    return e;
}
static VarBinding *env_find_local(Env *e, const char *name) {
    for (VarBinding *b = e->vars; b; b = b->next) if (!strcmp(b->name, name)) return b;
    return NULL;
}
static VarBinding *env_find(Env *e, const char *name) {
    for (; e; e = e->parent) {
        VarBinding *b = env_find_local(e, name);
        if (b) return b;
    }
    return NULL;
}
static void env_define(Env *e, const char *name, Value v) {
    VarBinding *b = calloc(1, sizeof(VarBinding));
    b->name = strdup(name); b->val = v; b->next = e->vars;
    e->vars = b;
}

// ---------------------------------------------------------------------
// Interpreter
// ---------------------------------------------------------------------

typedef struct { bool returning; Value retval; } ExecCtl;

static Value eval_expr(Expr *e, Env *env);
static ExecCtl exec_block(Stmt **body, int n, Env *env);

static FnDef *find_fn(const char *name) {
    for (FnDef *f = g_fns; f; f = f->next) if (!strcmp(f->name, name)) return f;
    return NULL;
}

static void print_value(Value v) {
    switch (v.type.base) {
        case TY_BOOL: printf("%s", v.i ? "true" : "false"); break;
        case TY_STRING: printf("%s", v.sval); break;
        case TY_PTR: printf("ptr(slot=%llu, gen=%u)", (unsigned long long)v.ptr.slot_id, v.ptr.generation); break;
        case TY_STRUCT: {
            printf("%s { ", v.sv->def->name);
            for (int i = 0; i < v.sv->def->nfields; i++) {
                printf("%s: ", v.sv->def->fields[i].name);
                print_value(v.sv->fields[i]);
                if (i + 1 < v.sv->def->nfields) printf(", ");
            }
            printf(" }");
            break;
        }
        default: {
            bool is_signed = (v.type.base == TY_I8 || v.type.base == TY_I16 || v.type.base == TY_I32 || v.type.base == TY_I64);
            if (is_signed) printf("%lld", (long long)v.i);
            else {
                int sz = base_size(v.type.base);
                uint64_t mask = (sz >= 8) ? ~0ULL : ((1ULL << (sz*8)) - 1);
                printf("%llu", (unsigned long long)((uint64_t)v.i & mask));
            }
        }
    }
}

static int64_t as_i64(Value v) { return v.i; }

static Value eval_binop(TokType op, Value a, Value b, int line) {
    if (op == T_AND) return make_bool(as_i64(a) && as_i64(b));
    if (op == T_OR) return make_bool(as_i64(a) || as_i64(b));
    if (a.type.base == TY_STRING || b.type.base == TY_STRING) {
        if (a.type.base != TY_STRING || b.type.base != TY_STRING)
            die("line %d: cannot mix string and non-string operands", line);
        if (op == T_PLUS) {
            size_t la = strlen(a.sval), lb = strlen(b.sval);
            char *out = malloc(la + lb + 1);
            memcpy(out, a.sval, la);
            memcpy(out + la, b.sval, lb + 1);
            Value v = {0}; v.type.base = TY_STRING; v.sval = out; return v;
        }
        if (op == T_EQ) return make_bool(strcmp(a.sval, b.sval) == 0);
        if (op == T_NEQ) return make_bool(strcmp(a.sval, b.sval) != 0);
        die("line %d: unsupported operator on strings", line);
    }
    Type rt = a.type; // result takes lhs's declared width (mirrors typical C promotion enough for our purposes)
    int64_t x = as_i64(a), y = as_i64(b);
    switch (op) {
        case T_PLUS: return make_int(rt, x + y);
        case T_MINUS: return make_int(rt, x - y);
        case T_STAR: return make_int(rt, x * y);
        case T_SLASH:
            if (y == 0) die("line %d: division by zero", line);
            return make_int(rt, x / y);
        case T_PERCENT:
            if (y == 0) die("line %d: division by zero", line);
            return make_int(rt, x % y);
        case T_EQ: return make_bool(x == y);
        case T_NEQ: return make_bool(x != y);
        case T_LT: return make_bool(x < y);
        case T_GT: return make_bool(x > y);
        case T_LE: return make_bool(x <= y);
        case T_GE: return make_bool(x >= y);
        default: die("line %d: unsupported operator", line);
    }
}

static Value call_fn(FnDef *fn, Value *args, int nargs, int line) {
    if (nargs != fn->nparams) die("line %d: %s() expects %d args, got %d", line, fn->name, fn->nparams, nargs);
    Env *fenv = env_new(NULL);
    for (int i = 0; i < nargs; i++) env_define(fenv, fn->params[i].name, args[i]);
    ExecCtl ctl = exec_block(fn->body, fn->nbody, fenv);
    if (ctl.returning) return ctl.retval;
    Value v = {0}; v.type.base = TY_VOID; return v;
}

static Value eval_expr(Expr *e, Env *env) {
    switch (e->kind) {
        case E_INT: return make_int((Type){.base=TY_I32}, e->ival);
        case E_BOOL: return make_bool(e->bval);
        case E_STRING: {
            Value v = {0}; v.type.base = TY_STRING; v.sval = e->name; return v;
        }
        case E_IDENT: {
            VarBinding *b = env_find(env, e->name);
            if (!b) die("line %d: undefined variable '%s'", e->line, e->name);
            return b->val;
        }
        case E_UNOP: {
            Value v = eval_expr(e->lhs, env);
            if (e->op == T_MINUS) return make_int(v.type, -as_i64(v));
            if (e->op == T_NOT) return make_bool(!as_i64(v));
            die("line %d: bad unary op", e->line);
        }
        case E_BINOP: {
            Value a = eval_expr(e->lhs, env);
            Value b = eval_expr(e->rhs, env);
            return eval_binop(e->op, a, b, e->line);
        }
        case E_CALL: {
            FnDef *fn = find_fn(e->callee);
            if (!fn) die("line %d: undefined function '%s'", e->line, e->callee);
            Value args[16];
            for (int i = 0; i < e->nargs && i < 16; i++) args[i] = eval_expr(e->args[i], env);
            return call_fn(fn, args, e->nargs, e->line);
        }
        case E_STRUCTLIT: {
            StructDef *sd = find_struct(e->name);
            if (!sd) die("line %d: unknown struct '%s'", e->line, e->name);
            StructVal *sv = calloc(1, sizeof(StructVal));
            sv->def = sd;
            sv->fields = calloc(sd->nfields, sizeof(Value));
            for (int i = 0; i < e->nfinits; i++) {
                int idx = -1;
                for (int j = 0; j < sd->nfields; j++) if (!strcmp(sd->fields[j].name, e->finits[i].name)) idx = j;
                if (idx < 0) die("line %d: struct '%s' has no field '%s'", e->line, sd->name, e->finits[i].name);
                sv->fields[idx] = eval_expr(e->finits[i].value, env);
            }
            Value v = {0}; v.type.base = TY_STRUCT; v.type.sdef = sd; v.sv = sv;
            return v;
        }
        case E_FIELD: {
            Value base = eval_expr(e->lhs, env);
            if (base.type.base != TY_STRUCT) die("line %d: '.%s' on a non-struct value", e->line, e->name);
            for (int i = 0; i < base.sv->def->nfields; i++)
                if (!strcmp(base.sv->def->fields[i].name, e->name)) return base.sv->fields[i];
            die("line %d: struct '%s' has no field '%s'", e->line, base.sv->def->name, e->name);
        }
        case E_ALLOC: {
            if (g_next_slot >= MAX_SLOTS) die("line %d: out of virtual memory slots", e->line);
            int idx = -1;
            for (int i = 0; i < MAX_SLOTS; i++) if (!g_slots[i].in_use) { idx = i; break; }
            if (idx < 0) die("line %d: out of virtual memory slots", e->line);
            Slot *s = &g_slots[idx];
            s->id = g_next_slot++;
            s->generation = 1;
            s->elem_type = e->alloc_type;
            s->in_use = true;
            s->freed = false;
            s->alloc_line = e->line;
            s->free_line = 0;
            if (g_lex.pragma_unsafe_uninit) {
                s->data = make_int(e->alloc_type, (int64_t)(rand() ^ (rand() << 16)));
            } else {
                s->data = make_int(e->alloc_type, 0);
            }
            Value v = {0}; v.type.base = TY_PTR; v.type.pointee = calloc(1,sizeof(Type)); *v.type.pointee = e->alloc_type;
            v.ptr.slot_id = s->id; v.ptr.generation = s->generation;
            return v;
        }
        case E_PTRVAL: {
            Value pv = eval_expr(e->lhs, env);
            if (pv.type.base != TY_PTR) die("line %d: '.value' used on a non-pointer", e->line);
            Slot *s = slot_lookup(pv.ptr.slot_id);
            if (!s || s->generation != pv.ptr.generation) {
                if (g_lex.pragma_unsafe_deref) {
                    // simulate corruption: read whatever now occupies (or garbage if gone)
                    if (s) return s->data;
                    Value garbage = {0}; garbage.type = *pv.type.pointee;
                    garbage.i = (int64_t)(rand() ^ (rand() << 16));
                    return garbage;
                }
                die("line %d: LifetimeError: use-after-free on slot %llu\n"
                    "  allocated at line %d\n"
                    "  freed at line %d\n"
                    "  accessed (use-after-free) at line %d",
                    e->line, (unsigned long long)pv.ptr.slot_id,
                    s ? s->alloc_line : -1, s ? s->free_line : -1, e->line);
            }
            return s->data;
        }
        case E_SIZEOF: {
            StructDef *sd = find_struct(e->name);
            int sz;
            if (sd) sz = sd->size;
            else sz = base_size(parse_base_type_name(e->name));
            return make_int((Type){.base=TY_I32}, sz);
        }
        case E_OFFSETOF: {
            StructDef *sd = find_struct(e->name);
            if (!sd) die("line %d: unknown struct '%s'", e->line, e->name);
            for (int i = 0; i < sd->nfields; i++)
                if (!strcmp(sd->fields[i].name, e->field2))
                    return make_int((Type){.base=TY_I32}, sd->fields[i].offset);
            die("line %d: struct '%s' has no field '%s'", e->line, sd->name, e->field2);
        }
        default: die("line %d: cannot evaluate expression", e->line);
    }
}

static ExecCtl exec_stmt(Stmt *s, Env *env);

static ExecCtl exec_block(Stmt **body, int n, Env *env) {
    ExecCtl ctl = {0};
    for (int i = 0; i < n; i++) {
        ctl = exec_stmt(body[i], env);
        if (ctl.returning) return ctl;
    }
    return ctl;
}

static ExecCtl exec_stmt(Stmt *s, Env *env) {
    ExecCtl none = {0};
    switch (s->kind) {
        case S_LET: {
            Value v;
            if (s->expr) {
                v = eval_expr(s->expr, env);
                if (s->let_type.base == TY_STRUCT || s->let_type.base == TY_PTR || s->let_type.base == TY_STRING) {
                    /* keep struct/pointer/string value as-is */
                } else {
                    v = make_int(s->let_type, as_i64(v));
                }
            } else {
                v = (s->let_type.base == TY_STRING) ? (Value){.type={.base=TY_STRING}, .sval=""} : make_int(s->let_type, 0);
            }
            env_define(env, s->name, v);
            return none;
        }
        case S_ASSIGN: {
            VarBinding *b = env_find(env, s->name);
            if (!b) die("line %d: undefined variable '%s'", s->line, s->name);
            Value v = eval_expr(s->expr, env);
            if (b->val.type.base == TY_STRUCT || b->val.type.base == TY_PTR || b->val.type.base == TY_STRING) b->val = v;
            else b->val = make_int(b->val.type, as_i64(v));
            return none;
        }
        case S_FIELD_ASSIGN: {
            VarBinding *b = env_find(env, s->name);
            if (!b || b->val.type.base != TY_STRUCT) die("line %d: '%s' is not a struct variable", s->line, s->name);
            const char *fname = s->cond->name;
            Value v = eval_expr(s->expr, env);
            for (int i = 0; i < b->val.sv->def->nfields; i++) {
                if (!strcmp(b->val.sv->def->fields[i].name, fname)) {
                    b->val.sv->fields[i] = make_int(b->val.sv->fields[i].type, as_i64(v));
                    return none;
                }
            }
            die("line %d: struct has no field '%s'", s->line, fname);
        }
        case S_PTR_ASSIGN: {
            VarBinding *b = env_find(env, s->name);
            if (!b || b->val.type.base != TY_PTR) die("line %d: '%s' is not a pointer", s->line, s->name);
            Slot *slot = slot_lookup(b->val.ptr.slot_id);
            Value v = eval_expr(s->expr, env);
            if (!slot || slot->generation != b->val.ptr.generation) {
                if (g_lex.pragma_unsafe_deref) {
                    if (slot) slot->data = make_int(slot->elem_type, as_i64(v));
                    return none;
                }
                die("line %d: LifetimeError: use-after-free on slot %llu (write)\n"
                    "  allocated at line %d\n  freed at line %d\n  accessed at line %d",
                    s->line, (unsigned long long)b->val.ptr.slot_id,
                    slot ? slot->alloc_line : -1, slot ? slot->free_line : -1, s->line);
            }
            slot->data = make_int(slot->elem_type, as_i64(v));
            return none;
        }
        case S_FREE: {
            Value pv = eval_expr(s->expr, env);
            if (pv.type.base != TY_PTR) die("line %d: free() expects a pointer", s->line);
            Slot *slot = slot_lookup(pv.ptr.slot_id);
            if (!slot || slot->generation != pv.ptr.generation || slot->freed) {
                die("line %d: DoubleFreeError: slot %llu already freed at line %d",
                    s->line, (unsigned long long)pv.ptr.slot_id, slot ? slot->free_line : -1);
            }
            slot->freed = true;
            slot->free_line = s->line;
            slot->generation++;
            return none;
        }
        case S_IF: {
            Value c = eval_expr(s->cond, env);
            Env *ie = env_new(env);
            if (as_i64(c)) return exec_block(s->body, s->nbody, ie);
            else if (s->nelse) return exec_block(s->elsebody, s->nelse, ie);
            return none;
        }
        case S_WHILE: {
            while (as_i64(eval_expr(s->cond, env))) {
                Env *we = env_new(env);
                ExecCtl ctl = exec_block(s->body, s->nbody, we);
                if (ctl.returning) return ctl;
            }
            return none;
        }
        case S_RETURN: {
            ExecCtl ctl = {0};
            ctl.returning = true;
            if (s->expr) ctl.retval = eval_expr(s->expr, env);
            return ctl;
        }
        case S_PRINT: {
            Value v = eval_expr(s->expr, env);
            print_value(v);
            printf("\n");
            return none;
        }
        case S_BLOCK: return exec_block(s->body, s->nbody, env_new(env));
        case S_UNSAFE: return exec_block(s->body, s->nbody, env_new(env));
        case S_EXPR: eval_expr(s->expr, env); return none;
        default: die("line %d: cannot execute statement", s->line);
    }
}

static void report_leaks(void) {
    bool any = false;
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (g_slots[i].in_use && !g_slots[i].freed) {
            if (!any) { printf("\n--- Memory Leak Report ---\n"); any = true; }
            printf("leaked slot %llu (allocated at line %d, never freed)\n",
                   (unsigned long long)g_slots[i].id, g_slots[i].alloc_line);
        }
    }
}

// ---------------------------------------------------------------------
// main
// ---------------------------------------------------------------------

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open file '%s'", path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(n + 1);
    size_t got = fread(buf, 1, n, f);
    buf[got] = 0;
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) die("usage: %s <file.phx>", argv[0]);
    srand((unsigned)time(NULL));
    char *src = read_file(argv[1]);
    parse_program(src);
    FnDef *main_fn = find_fn("main");
    if (!main_fn) die("no main() function found");
    call_fn(main_fn, NULL, 0, 0);
    report_leaks();
    return 0;
}
