#pragma once

#include <string>
#include <vector>

struct Node;
struct DeclInfo;


struct TypeInfo {
    std::string base;
    int ptr_depth = 0;
    std::vector<std::string> dims;          // only used by type_name (casts)
    std::vector<DeclInfo *> members;        // only for struct definitions
};

struct DeclInfo {
    std::string name;
    int  ptr_depth   = 0;
    bool is_function = false;
    std::vector<std::string> dims;          // array dims, outermost first
    TypeInfo *spec   = nullptr;             // the specifiers it was declared with
    std::vector<DeclInfo *> params;         // functions only
    Node *init       = nullptr;             // "= <initializer>", if any
    bool is_typedef  = false;
    std::vector<Node *> dim_exprs;          // [..] expressions, for semantic to fold
    int line = 0;
};

struct DeclList {
    std::vector<DeclInfo *> items;
};


// what kind of node is it

enum NodeKind {
    N_PROGRAM, N_FUNC, N_DECL,
    N_INT, N_FLOAT, N_CHAR, N_STR, N_ID,
    N_BIN, N_UN, N_ASSIGN, N_TERNARY,
    N_CALL, N_INDEX, N_MEMBER, N_CAST, N_SIZEOF,
    N_LIST, N_INITLIST,
    N_IF, N_WHILE, N_UNTIL, N_DO, N_FOR, N_BLOCK,
    N_EXPRSTMT, N_RETURN, N_BREAK, N_CONTINUE, N_GOTO, N_LABEL
};

// what semantic works out for an expression
struct Type {
    std::string base;                  // INT CHAR FLOAT VOID UNKNOWN, or "STRUCT tag"
    int ptr_depth = 0;
    std::vector<int> dims;             // resolved, outermost first
    bool is_struct() const { return base.size() > 7 && base.compare(0, 7, "STRUCT ") == 0; }
    bool is_float()  const { return base == "FLOAT" || base == "DOUBLE"; }
    std::string tag() const { return is_struct() ? base.substr(7) : ""; }
};

struct Node {
    NodeKind kind;
    int line = 0;

    std::string op;                    // operator text like "+", "&&", "=", ...
    std::string name;                  // identifier, label name, member name
    std::string value;                 // literal text

    std::vector<Node *> kids;          // meaning depends on type

    // declarations
    TypeInfo *spec = nullptr;
    DeclInfo *decl = nullptr;
    std::vector<DeclInfo *> decls;

    // filled by semantic
    Type *type = nullptr;
    bool  lvalue = false;
    int   const_val = 0;
    bool  has_const = false;
};


// builders
// line is almost always token_line at the point the rule reduced

inline Node *mk_node(NodeKind k, int line) {
    Node *n = new Node();
    n->kind = k;
    n->line = line;
    return n;
}

inline Node *mk_int(const std::string &text, int line) {
    Node *n = mk_node(N_INT, line);
    n->value = text;
    return n;
}
inline Node *mk_float(const std::string &text, int line) {
    Node *n = mk_node(N_FLOAT, line);
    n->value = text;
    return n;
}
inline Node *mk_char(const std::string &text, int line) {
    Node *n = mk_node(N_CHAR, line);
    n->value = text;
    return n;
}
inline Node *mk_str(const std::string &text, int line) {
    Node *n = mk_node(N_STR, line);
    n->value = text;
    return n;
}
inline Node *mk_id(const std::string &name, int line) {
    Node *n = mk_node(N_ID, line);
    n->name = name;
    return n;
}

inline Node *mk_bin(const char *op, Node *a, Node *b, int line) {
    Node *n = mk_node(N_BIN, line);
    n->op = op;
    n->kids.push_back(a);
    n->kids.push_back(b);
    return n;
}

// unary ops
inline Node *mk_un(char op, Node *a, int line) {
    Node *n = mk_node(N_UN, line);
    n->op = std::string(1, op);
    n->kids.push_back(a);
    return n;
}

inline Node *mk_assign(Node *lhs, Node *rhs, int line) {
    Node *n = mk_node(N_ASSIGN, line);
    n->op = "=";
    n->kids.push_back(lhs);
    n->kids.push_back(rhs);
    return n;
}

inline Node *mk_list(int line) { return mk_node(N_LIST, line); }

inline Node *mk_stmt(NodeKind k, int line) { return mk_node(k, line); }

// sequential list of statements
inline void append(Node *list, Node *item) {
    if (list && item) list->kids.push_back(item);
}


struct Symbol {
    std::string name;
    std::string type;   
    int scope_id;
    int depth;
    int line;
};

struct TokenRow {
    std::string lexeme;
    std::string type;
};

extern std::vector<TokenRow> token_rows;
extern std::vector<Symbol>   symbols;
extern int syntax_error_count;
extern int lexical_error_count;
extern int token_line;
extern int token_column;

// root of the tree the parser built
extern Node *g_root;

void print_table(const char *title, const std::vector<TokenRow> &rows);
void print_symbol_table();
void dump_ast(Node *n, int indent = 0);
const char *node_kind_name(NodeKind k);
