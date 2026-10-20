#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "semantic.h"

namespace {

// what a name means

struct Sym {
    Type *type = nullptr;
    bool  is_function = false;
};

struct StructInfo {
    std::vector<std::pair<std::string, Type *> > members;
    int size = 0;
};

// what a function signature looks like, for call checking
struct FuncSig {
    Type *ret = nullptr;
    std::vector<Type *> params;
    bool varargs = false;
    bool defined = false;
};

// typedef
struct Typedef {
    std::string base = "INT";
    int ptr_depth = 0;
    std::vector<int> dims;
};


class Sem {
public:
    const char *file;
    int errors = 0;

    std::vector<std::map<std::string, Sym> > scopes;
    std::map<std::string, StructInfo>        structs;
    std::map<std::string, FuncSig>           funcs;
    std::vector<std::string>                 builtins;   

    // innermost loop, for break/continue, -1 means not in a loop
    int loop_depth = 0;

    // labels of function being checked rn
    std::set<std::string> labels;

    Type *cur_ret = nullptr;


    Sem(const char *f) : file(f) { scopes.push_back(std::map<std::string, Sym>()); }

    void err(int line, const std::string &msg) {
        fprintf(stderr, "%s:%d: error: %s\n", file, line, msg.c_str());
        errors++;
    }

    // types

    Type *T(const std::string &base, int ptr = 0) {
        Type *t = new Type();
        t->base = base;
        t->ptr_depth = ptr;
        return t;
    }

    Type *copy(const Type *t) {
        Type *n = new Type(*t);
        return n;
    }


    Type *type_from(TypeInfo *spec, DeclInfo *d) {
        Type *t = new Type();
        t->base = spec ? spec->base : "INT";
        if (spec && !spec->members.empty()) register_struct(spec);

        if (spec) {
            Typedef td = resolve_typedef(t->base);
            t->base = td.base;
            for (size_t i = 0; i < td.dims.size(); i++)
                t->dims.push_back(td.dims[i]);
            t->ptr_depth = td.ptr_depth + (d ? d->ptr_depth : 0);
        } else {
            t->ptr_depth = d ? d->ptr_depth : 0;
        }

        if (d) {
            for (size_t i = 0; i < d->dims.size(); i++) {
                int n = 0;
                if (i < d->dim_exprs.size() && d->dim_exprs[i] &&
                    d->dim_exprs[i]->has_const)
                    n = d->dim_exprs[i]->const_val;
                else if (!d->dims[i].empty() && d->dims[i] != "_")
                    n = atoi(d->dims[i].c_str());
                t->dims.push_back(n);
            }
        }
        return t;
    }

    void collect_params(const std::vector<DeclInfo *> &ps, std::vector<Type *> &out) {
        out.clear();
        if (ps.size() == 1) {
            DeclInfo *p = ps[0];
            Type *t = type_from(p->spec, p);
            if (p->name.empty() && t->base == "VOID" &&
                t->ptr_depth == 0 && t->dims.empty())
                return;                       
            out.push_back(t);
            return;
        }
        for (size_t i = 0; i < ps.size(); i++)
            out.push_back(type_from(ps[i]->spec, ps[i]));
    }

    void record_typedef(const std::string &name, TypeInfo *spec, DeclInfo *d) {
        if (name.empty()) return;
        Typedef td;
        td.base = spec ? spec->base : "INT";
        td.ptr_depth = d ? d->ptr_depth : 0;
        std::map<std::string, Typedef>::iterator prev = typedef_map.find(td.base);
        if (prev != typedef_map.end()) {
            td.base      = prev->second.base;
            td.ptr_depth += prev->second.ptr_depth;
            td.dims      = prev->second.dims;
        }
        if (d) {
            for (size_t i = 0; i < d->dim_exprs.size(); i++) {
                if (d->dim_exprs[i] && d->dim_exprs[i]->has_const)
                    td.dims.push_back(d->dim_exprs[i]->const_val);
                else if (i < d->dims.size() && !d->dims[i].empty() && d->dims[i] != "_")
                    td.dims.push_back(atoi(d->dims[i].c_str()));
                else
                    td.dims.push_back(0);
            }
        }
        typedef_map[name] = td;
    }

    // typedef name
    Typedef resolve_typedef(const std::string &base) {
        std::map<std::string, Typedef>::iterator it = typedef_map.find(base);
        if (it != typedef_map.end()) return it->second;
        Typedef plain;
        plain.base = base.empty() ? "INT" : base;
        return plain;
    }

    std::map<std::string, Typedef> typedef_map;

    void register_struct(TypeInfo *spec) {
        if (!spec || spec->members.empty()) return;
        if (spec->members.size() == 0) return;

        if (spec->base == "STRUCT <anonymous>" || spec->base == "STRUCT") {
            static int anon = 0;
            char buf[32];
            sprintf(buf, "STRUCT $anon%d", ++anon);
            spec->base = buf;
        }

        std::string tag = spec->base.substr(7);
        if (structs.count(tag)) return;      

        StructInfo si;
        int off = 0;
        for (size_t i = 0; i < spec->members.size(); i++) {
            DeclInfo *m = spec->members[i];
            Type *mt = type_from(m->spec, m);
            si.members.push_back(std::make_pair(m->name, mt));
            off += size_of(mt);
        }
        si.size = off > 0 ? off : 4;
        structs[tag] = si;
    }

    int size_of(const Type *t) {
        if (!t) return 4;
        if (!t->dims.empty()) {
            int n = 1;
            for (size_t i = 0; i < t->dims.size(); i++)
                n *= (t->dims[i] > 0 ? t->dims[i] : 1);
            Type elem = *t;
            elem.dims.clear();
            return n * size_of(&elem);
        }
        if (t->ptr_depth > 0) return 4;
        if (t->base == "CHAR")   return 1;
        if (t->base == "DOUBLE") return 8;
        if (t->base == "VOID")   return 0;
        if (t->is_struct()) {
            std::map<std::string, StructInfo>::iterator it = structs.find(t->tag());
            if (it != structs.end()) return it->second.size;
            return 4;
        }
        return 4;
    }

    // scopes

    void push_scope() { scopes.push_back(std::map<std::string, Sym>()); }
    void pop_scope()  { if (scopes.size() > 1) scopes.pop_back(); }

    Sym *lookup(const std::string &name) {
        for (int i = (int)scopes.size() - 1; i >= 0; i--) {
            std::map<std::string, Sym>::iterator it = scopes[i].find(name);
            if (it != scopes[i].end()) return &it->second;
        }
        return NULL;
    }

    // true if name already exists in innermost scope only
    bool declared_here(const std::string &name) {
        return scopes.back().count(name) != 0;
    }

    void declare(const std::string &name, Type *t, int line, bool is_func) {
        if (name.empty()) return;
        if (scopes.back().count(name)) {
            err(line, "'" + name + "' is already declared in this scope");
            return;
        }
        Sym s;
        s.type = t;
        s.is_function = is_func;
        scopes.back()[name] = s;
    }

    void declare_func(const std::string &name, Type *t, int line) {
        if (name.empty()) return;

        std::map<std::string, Sym>::iterator it = scopes.back().find(name);
        if (it != scopes.back().end()) {
            if (!it->second.is_function)
                err(line, "'" + name + "' is already declared in this scope");
            else {
                it->second.type = t;   
            }
            return;
        }

        Sym s;
        s.type = t;
        s.is_function = true;
        scopes.back()[name] = s;
    }


    void add_builtin(const char *name, const char *ret) {
        Type *rt = T(ret, strchr(ret, '*') ? 1 : 0);
        Sym s;
        s.type = rt;
        s.is_function = true;
        scopes[0][name] = s;
        FuncSig fs;
        fs.ret = rt;
        fs.varargs = true;
        fs.defined = true;
        funcs[name] = fs;
        builtins.push_back(name);
    }

    void seed_builtins() {
        add_builtin("printf",  "INT");
        add_builtin("fprintf", "INT");
        add_builtin("sprintf", "INT");
        add_builtin("scanf",   "INT");
        add_builtin("fscanf",  "INT");
        add_builtin("sscanf",  "INT");
        add_builtin("fopen",   "INT*");
        add_builtin("fclose",  "INT");
        add_builtin("fgets",   "INT*");     
        add_builtin("fputs",   "INT");
        add_builtin("fgetc",   "INT");
        add_builtin("fputc",   "INT");
        add_builtin("malloc",  "INT*");
        add_builtin("calloc",  "INT*");
        add_builtin("realloc", "INT*");
        add_builtin("free",    "VOID");
        add_builtin("strlen",  "INT");
        add_builtin("strcpy",  "INT*");
        add_builtin("strcmp",  "INT");
        add_builtin("atoi",    "INT");
        add_builtin("abs",     "INT");
        add_builtin("exit",    "VOID");
        add_builtin("rand",    "INT");
        add_builtin("srand",   "VOID");
        add_builtin("puts",    "INT");
        add_builtin("putchar", "INT");
        add_builtin("getchar", "INT");
    }


    void hoist(Node *n) {
        if (!n) return;

        if (n->kind == N_FUNC) {
            Type *ret = type_from(n->spec, NULL);
            cur_ret = ret;

            Type *ft = copy(ret);
            declare_func(n->decl->name, ft, n->line);

            FuncSig &fs = funcs[n->decl->name];
            fs.ret = ret;
            fs.defined = true;
            fs.varargs = false;
            fs.params.clear();
            collect_params(n->decl->params, fs.params);
            return;
        }

        if (n->kind == N_DECL) {
            if (n->spec && !n->spec->members.empty()) register_struct(n->spec);
            for (size_t i = 0; i < n->decls.size(); i++) {
                DeclInfo *d = n->decls[i];
                if (d->name.empty()) continue;

                if (d->is_typedef) {
                    record_typedef(d->name, n->spec, d);
                    continue;
                }

                Type *t = type_from(n->spec, d);
                if (d->is_function) {
                    declare_func(d->name, t, n->line);
                    FuncSig &fs = funcs[d->name];
                    fs.ret = t;
                    if (!fs.defined) collect_params(d->params, fs.params);
                } else {
                    declare(d->name, t, n->line, false);
                }
            }
            return;
        }

        if (n->kind == N_PROGRAM) return;   
    }


    Type *check_expr(Node *n) {
        if (!n) return T("INT");
        Type *t = expr(n);
        n->type = t;
        return t;
    }

    bool is_arith(const Type *t) {
        return t && t->ptr_depth == 0 && t->dims.empty() &&
               !t->is_struct() && t->base != "VOID";
    }

    Type *usual(const Type *a, const Type *b) {
        if ((a && a->is_float()) || (b && b->is_float())) return T("FLOAT");
        return T("INT");
    }

    Type *decay(const Type *t) {
        Type *r = copy(t);
        if (!r->dims.empty()) {
            r->dims.erase(r->dims.begin());
            r->ptr_depth++;
        }
        return r;
    }


    void try_fold(Node *n) {
        if (!n || n->kind != N_BIN) return;
        if (n->kids.size() < 2) return;
        Node *a = n->kids[0], *b = n->kids[1];
        if (!a->has_const || !b->has_const) return;
        int x = a->const_val, y = b->const_val;
        if (n->op == "+") { n->const_val = x + y; n->has_const = true; }
        else if (n->op == "-") { n->const_val = x - y; n->has_const = true; }
        else if (n->op == "*") { n->const_val = x * y; n->has_const = true; }
        else if (n->op == "/") { if (y) { n->const_val = x / y; n->has_const = true; } }
        else if (n->op == "%") { if (y) { n->const_val = x % y; n->has_const = true; } }
    }

    Type *expr(Node *n) {
        if (!n) return T("INT");

        switch (n->kind) {

        case N_INT:
            n->has_const = true;
            n->const_val = (int)strtol(n->value.c_str(), NULL, 0);
            return T("INT");

        case N_FLOAT:
            n->has_const = true;
            n->const_val = (int)atof(n->value.c_str());
            return T("FLOAT");

        case N_CHAR: {
            int c = 0;
            if (n->value.size() >= 3) {
                if (n->value[1] == '\\') {
                    switch (n->value[2]) {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case '0': c = '\0'; break;
                        case '\\': c = '\\'; break;
                        case '\'': c = '\''; break;
                        default:  c = n->value[2]; break;
                    }
                } else c = n->value[1];
            }
            n->has_const = true;
            n->const_val = c;
            return T("INT");
        }

        case N_STR: {
            Type *t = T("CHAR");
            t->dims.push_back((int)n->value.size());
            n->lvalue = true;
            return t;
        }

        case N_ID: {
            Sym *s = lookup(n->name);
            if (!s) {
                err(n->line, "'" + n->name + "' was not declared");
                return T("INT");
            }
            n->lvalue = !s->is_function;
            Type *t = copy(s->type);
            if (s->is_function) {
                n->lvalue = false;
            }
            return t;
        }

        case N_INDEX: {
            if (n->kids.size() < 2) return T("INT");
            Type *base  = check_expr(n->kids[0]);
            Type *index = check_expr(n->kids[1]);
            if (!index || index->ptr_depth > 0)
                err(n->line, "array subscript is not an integer");

            if (!base) return T("INT");
            if (!base->dims.empty()) {
                Type *elem = copy(base);
                elem->dims.erase(elem->dims.begin());
                n->lvalue = true;
                return elem;
            }
            if (base->ptr_depth > 0) {
                Type *elem = copy(base);
                elem->ptr_depth--;
                n->lvalue = true;
                return elem;
            }
            err(n->line, "subscripted value is neither an array nor a pointer");
            return T("INT");
        }

        case N_MEMBER: {
            if (n->kids.empty()) return T("INT");
            Type *base = check_expr(n->kids[0]);
            if (!base) return T("INT");

            bool arrow = (n->op == "->");
            if (arrow) {
                if (base->ptr_depth == 0) {
                    err(n->line, "'->' needs a pointer on the left, got '" +
                                 base->base + "'");
                    return T("INT");
                }
                Type tmp = *base;
                tmp.ptr_depth--;
                base = &tmp;
            }
            if (!base->is_struct()) {
                err(n->line, "'.' needs a struct on the left, got '" +
                             base->base + "'");
                return T("INT");
            }

            std::map<std::string, StructInfo>::iterator it = structs.find(base->tag());
            if (it == structs.end()) {
                err(n->line, "struct '" + base->tag() + "' is not defined");
                return T("INT");
            }
            for (size_t i = 0; i < it->second.members.size(); i++) {
                if (it->second.members[i].first == n->name) {
                    n->lvalue = true;
                    return copy(it->second.members[i].second);
                }
            }
            err(n->line, "struct '" + base->tag() + "' has no member '" +
                         n->name + "'");
            return T("INT");
        }

        case N_UN: {
            if (n->kids.empty()) return T("INT");
            Type *a = check_expr(n->kids[0]);

            if (n->op == "&") {
                if (!n->kids[0]->lvalue && n->kids[0]->kind != N_FUNC)
                    err(n->line, "cannot take the address of this expression");
                Type *t = copy(a);
                t->dims.clear();
                t->ptr_depth++;
                return t;
            }
            if (n->op == "*") {
                if (!a) return T("INT");
                if (!a->dims.empty()) {
                    Type *t = copy(a);
                    t->dims.erase(t->dims.begin());
                    n->lvalue = true;
                    return t;
                }
                if (a->ptr_depth == 0) {
                    err(n->line, "cannot dereference '" + a->base +
                                 "', it is not a pointer");
                    return T("INT");
                }
                Type *t = copy(a);
                t->ptr_depth--;
                n->lvalue = true;
                return t;
            }
            if (n->op == "++" || n->op == "--") {
                if (!n->kids[0]->lvalue)
                    err(n->line, "'" + n->op + "' needs something assignable");
                n->lvalue = n->kids[0]->lvalue;
                return copy(a);
            }
            if (!is_arith(a))
                err(n->line, "operator '" + n->op + "' does not work on '" +
                             (a ? a->base : "?") + "'");
            return a && a->is_float() ? T("FLOAT") : T("INT");
        }

        case N_BIN: {
            if (n->kids.size() < 2) return T("INT");
            if (n->op == ",") return check_expr(n->kids[1]);

            Type *a = check_expr(n->kids[0]);
            Type *b = check_expr(n->kids[1]);

            bool cmp = (n->op == "<" || n->op == ">" || n->op == "<=" ||
                        n->op == ">=" || n->op == "==" || n->op == "!=");
            bool logi = (n->op == "&&" || n->op == "||");

            if (a && !a->dims.empty()) a = decay(a);
            if (b && !b->dims.empty()) b = decay(b);

            if (is_arith(a) && is_arith(b)) {
                if (cmp || logi) return T("INT");
                return usual(a, b);
            }

            if ((a && a->ptr_depth > 0) || (b && b->ptr_depth > 0)) {
                if (cmp || logi) return T("INT");
                if (n->op == "+" || n->op == "-") return a && a->ptr_depth > 0 ? copy(a) : copy(b);
            }

            if (!is_arith(a) || !is_arith(b))
                err(n->line, "operator '" + n->op + "' does not work on '" +
                             (a ? a->base : "?") + "' and '" +
                             (b ? b->base : "?") + "'");
            try_fold(n);
            return usual(a, b);
        }

        case N_TERNARY: {
            if (n->kids.size() < 3) return T("INT");
            check_expr(n->kids[0]);
            Type *a = check_expr(n->kids[1]);
            Type *b = check_expr(n->kids[2]);
            return usual(a, b);
        }

        case N_ASSIGN: {
            if (n->kids.size() < 2) return T("INT");
            Node *lhs = n->kids[0];
            Type *lt = check_expr(lhs);
            Type *rt = check_expr(n->kids[1]);

            if (!lhs->lvalue) {
                err(n->line, "the left side of '" + n->op + "' is not assignable");
            } else if (!lt->dims.empty()) {
                err(n->line, "cannot assign to an array");
            }

            if (n->op != "=") {
                if (!is_arith(lt))
                    err(n->line, "compound assignment needs a number on the left");
                if (!is_arith(rt))
                    err(n->line, "compound assignment needs a number on the right");
            }
            n->type = lt;
            return lt;
        }

        case N_CAST: {
            if (n->kids.empty()) return T("INT");
            check_expr(n->kids[0]);
            Type *t = T(n->spec ? n->spec->base : "INT",
                        n->spec ? n->spec->ptr_depth : 0);
            if (n->spec) {
                for (size_t i = 0; i < n->spec->dims.size(); i++)
                    t->dims.push_back(atoi(n->spec->dims[i].c_str()));
            }
            return t;
        }

        case N_SIZEOF: {
            if (n->spec) {
                Type *t = T(n->spec->base, n->spec->ptr_depth);
                for (size_t i = 0; i < n->spec->dims.size(); i++)
                    t->dims.push_back(atoi(n->spec->dims[i].c_str()));
                if (n->spec->base == "STRUCT" || n->spec->base.empty())
                    t = T("INT");
                n->has_const = true;
                n->const_val = size_of(t);
                return T("INT");
            }
            if (n->kids.empty()) return T("INT");
            Type *t = check_expr(n->kids[0]);
            n->has_const = true;
            n->const_val = size_of(t);
            return T("INT");
        }

        case N_CALL: {
            if (n->kids.empty()) return T("INT");
            Node *callee = n->kids[0];

            if (callee->kind != N_ID) {
                err(n->line, "only plain functions can be called for now");
                return T("INT");
            }

            Sym *s = lookup(callee->name);
            if (!s) {
                err(n->line, "'" + callee->name + "' was not declared");
                return T("INT");
            }
            if (!s->is_function) {
                err(n->line, "'" + callee->name + "' is not a function");
                return T("INT");
            }

            std::vector<Node *> args;
            if (n->kids.size() > 1 && n->kids[1]->kind == N_LIST)
                args = n->kids[1]->kids;

            for (size_t i = 0; i < args.size(); i++) check_expr(args[i]);

            std::map<std::string, FuncSig>::iterator it = funcs.find(callee->name);
            if (it != funcs.end() && !it->second.varargs) {
                if (!it->second.defined) {
                    err(n->line, "'" + callee->name + "' is declared but never defined");
                } else if (args.size() != it->second.params.size()) {
                    char buf[128];
                    sprintf(buf, "'%s' takes %d argument(s), %d given",
                            callee->name.c_str(),
                            (int)it->second.params.size(), (int)args.size());
                    err(n->line, buf);
                }
            }

            callee->type = copy(s->type);
            n->type = copy(s->type);
            return copy(s->type);
        }

        case N_LIST: {
            Type *last = T("INT");
            for (size_t i = 0; i < n->kids.size(); i++) last = check_expr(n->kids[i]);
            return last;
        }

        case N_INITLIST:
            for (size_t i = 0; i < n->kids.size(); i++) check_expr(n->kids[i]);
            return T("INT");

        default:
            break;
        }
        return T("INT");
    }


    void collect_labels(Node *n) {
        if (!n) return;
        if (n->kind == N_LABEL) labels.insert(n->name);
        for (size_t i = 0; i < n->kids.size(); i++) collect_labels(n->kids[i]);
    }

    void stmt(Node *n) {
        if (!n) return;

        switch (n->kind) {

        case N_BLOCK:
            push_scope();
            for (size_t i = 0; i < n->kids.size(); i++) stmt(n->kids[i]);
            pop_scope();
            return;

        case N_DECL:
            if (n->spec && !n->spec->members.empty()) register_struct(n->spec);
            for (size_t i = 0; i < n->decls.size(); i++) {
                DeclInfo *d = n->decls[i];
                if (d->name.empty()) continue;

                if (d->is_typedef) {
                    record_typedef(d->name, n->spec, d);
                    continue;
                }
                Type *t = type_from(n->spec, d);
                if (d->is_function) {
                    declare_func(d->name, t, n->line);
                    FuncSig &fs = funcs[d->name];
                    fs.ret = t;
                    if (!fs.defined) collect_params(d->params, fs.params);
                } else {
                    declare(d->name, t, n->line, false);
                }
                if (d->init) {
                    if (d->init->kind == N_INITLIST) {
                        for (size_t k = 0; k < d->init->kids.size(); k++)
                            check_expr(d->init->kids[k]);
                    } else {
                        check_expr(d->init);
                    }
                }
            }
            return;

        case N_EXPRSTMT:
            for (size_t i = 0; i < n->kids.size(); i++) check_expr(n->kids[i]);
            return;

        case N_IF:
            check_expr(n->kids.size() > 0 ? n->kids[0] : NULL);
            if (n->kids.size() > 1) stmt(n->kids[1]);
            if (n->kids.size() > 2) stmt(n->kids[2]);
            return;

        case N_WHILE:
        case N_UNTIL:
            check_expr(n->kids.size() > 0 ? n->kids[0] : NULL);
            loop_depth++;
            if (n->kids.size() > 1) stmt(n->kids[1]);
            loop_depth--;
            return;

        case N_DO:
            loop_depth++;
            if (n->kids.size() > 0) stmt(n->kids[0]);
            loop_depth--;
            check_expr(n->kids.size() > 1 ? n->kids[1] : NULL);
            return;

        case N_FOR: {
            push_scope();
            if (n->kids.size() > 0) {
                if (n->kids[0]->kind == N_DECL) stmt(n->kids[0]);
                else check_expr(n->kids[0]);
            }
            if (n->kids.size() > 1) check_expr(n->kids[1]);
            loop_depth++;
            if (n->kids.size() > 3) stmt(n->kids[3]);
            loop_depth--;
            if (n->kids.size() > 2) check_expr(n->kids[2]);
            pop_scope();
            return;
        }

        case N_BREAK:
        case N_CONTINUE:
            if (loop_depth == 0)
                err(n->line, std::string(n->kind == N_BREAK ? "'break'" : "'continue'") +
                             " is only allowed inside a loop");
            return;

        case N_RETURN:
            if (cur_ret && cur_ret->base == "VOID" && !n->kids.empty() &&
                n->kids[0] && n->kids[0]->kind != N_BLOCK) {
                err(n->line, "returning a value from a void function");
            } else if (!n->kids.empty()) {
                check_expr(n->kids[0]);
            }
            return;

        case N_GOTO:
            if (!labels.count(n->name))
                err(n->line, "goto to an undefined label '" + n->name + "'");
            return;

        case N_LABEL:
            if (!n->kids.empty()) stmt(n->kids[0]);
            return;

        default:
            check_expr(n);
            return;
        }
    }


    int run(Node *root) {
        if (!root) return 0;

        seed_builtins();

        for (size_t i = 0; i < root->kids.size(); i++)
            hoist(root->kids[i]);

        for (size_t i = 0; i < root->kids.size(); i++) {
            Node *n = root->kids[i];
            if (n->kind == N_FUNC) {
                Type *ret = type_from(n->spec, NULL);
                cur_ret = ret;
                labels.clear();
                collect_labels(n);

                push_scope();
                for (size_t k = 0; k < n->decl->params.size(); k++) {
                    DeclInfo *p = n->decl->params[k];
                    if (p->name.empty()) continue;
                    Type *pt = type_from(p->spec, p);
                    if (pt->base == "VOID" && pt->ptr_depth == 0 && pt->dims.empty())
                        continue;
                    declare(p->name, pt, n->line, false);
                }
                loop_depth = 0;
                for (size_t k = 0; k < n->kids.size(); k++) stmt(n->kids[k]);
                pop_scope();
            }
        }
        return errors;
    }
};

}  


int semantic_check(Node *root, const char *filename) {
    Sem s(filename);
    return s.run(root);
}

int semantic_type_size(const Type &t) {
    Sem s("<sizeof>");
    return s.size_of(&t);
}
