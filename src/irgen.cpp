#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "irgen.h"
#include "semantic.h"

namespace {

// which label a break/continue inside the current loop should jump to
struct LoopCtx {
    std::string break_label;
    std::string cont_label;
};


class Gen {
public:
    std::vector<std::string> out;

    int temp_count  = 0;
    int label_count = 0;
    int str_count   = 0;

    std::vector<LoopCtx> loops;

    // string literals get pulled out and named so IR keeps them readable
    std::vector<std::pair<std::string, std::string> > strings;


    void emit(const std::string &s) { out.push_back("  " + s); }
    void label(const std::string &s) { out.push_back(s + ":"); }
    void blank() { out.push_back(""); }
    void comment(const std::string &s) { out.push_back("  // " + s); }

    std::string new_temp() {
        char buf[16];
        sprintf(buf, "t%d", ++temp_count);
        return buf;
    }

    std::string new_label() {
        char buf[16];
        sprintf(buf, "L%d", ++label_count);
        return buf;
    }

    // user written labels get a prefix so they can never collide with L1, L2
    std::string user_label(const std::string &name) { return "L_" + name; }


    // helpers

    // for index scaling
    int elem_size(const Type *t) {
        if (!t) return 4;
        if (!t->dims.empty()) {
            Type elem = *t;
            elem.dims.erase(elem.dims.begin());
            return semantic_type_size(elem);
        }
        if (t->ptr_depth > 0) return 4;
        if (t->base == "CHAR") return 1;
        if (t->base == "DOUBLE") return 8;
        return 4;
    }

    bool is_float(const Type *t) { return t && t->is_float(); }

    // operand text for a node
    std::string place(Node *n) {
        if (!n) return "0";

        switch (n->kind) {
        case N_INT:
        case N_CHAR:
            return literal(n);
        case N_FLOAT:
            return n->value;
        case N_STR:
            return string_operand(n);
        case N_SIZEOF:
            return literal(n);
        case N_ID:
            return n->name;
        case N_INDEX:
            return lvalue(n);         
        case N_MEMBER:
            return member_text(n);
        case N_EXPRSTMT:
            return n->kids.empty() ? "0" : place(n->kids[n->kids.size() - 1]);
        default:
            break;
        }

        std::string t = new_temp();
        gen_expr(n, t);
        return t;
    }

    std::string literal(Node *n) {
        char buf[32];
        if (n->kind == N_CHAR) sprintf(buf, "%d", n->const_val);
        else                   sprintf(buf, "%s", n->value.c_str());
        return buf;
    }

    std::string string_operand(Node *n) {
        for (size_t i = 0; i < strings.size(); i++)
            if (strings[i].second == n->value) return strings[i].first;

        char buf[16];
        sprintf(buf, "s%d", ++str_count);
        std::string name = buf;
        strings.push_back(std::make_pair(name, n->value));
        return name;
    }

    std::string member_text(Node *n) {
        if (n->kids.empty()) return n->name;
        std::string base = place(n->kids[0]);
        if (n->op == "->") return base + "->" + n->name;
        return base + "." + n->name;
    }


    std::string lvalue(Node *n) {
        if (!n) return "0";

        if (n->kind == N_ID) return n->name;

        if (n->kind == N_MEMBER) {
            std::string s = member_text(n);
            return s;
        }

        if (n->kind == N_INDEX) {
            std::vector<Node *> subs;
            Node *cur = n;
            while (cur && cur->kind == N_INDEX && cur->kids.size() >= 2) {
                subs.insert(subs.begin(), cur->kids[1]);
                cur = cur->kids[0];
            }
            std::string base = place(cur);

            Type *bt = cur ? cur->type : NULL;

            std::string idx = "0";
            if (subs.size() == 1) {
                idx = place(subs[0]);
            } else {
                std::string acc;
                for (size_t k = 0; k < subs.size(); k++) {
                    std::string cur_idx = place(subs[k]);
                    if (acc.empty()) {
                        acc = cur_idx;
                    } else {
                        acc = acc + " + " + cur_idx;
                        std::string t = new_temp();
                        emit(t + " = " + acc);
                        acc = t;
                    }
                    if (k + 1 < subs.size()) {
                        int stride = stride_at(bt, k);
                        if (stride != 1) {
                            std::string t = new_temp();
                            emit(t + " = " + acc + " * " + itos(stride));
                            acc = t;
                        }
                    }
                }
                idx = acc;
            }

            int esz = elem_size(bt);
            if (esz != 1) {
                std::string t = new_temp();
                emit(t + " = " + idx + " * " + itos(esz));
                idx = t;
            }
            return base + "[" + idx + "]";
        }

        if (n->kind == N_UN && n->op == "*") {
            std::string p = place(n->kids.size() ? n->kids[0] : NULL);
            std::string t = new_temp();
            emit(t + " = *" + p);
            return "*" + t;
        }

        return place(n);
    }

    std::string itos(int v) {
        char buf[32];
        sprintf(buf, "%d", v);
        return buf;
    }

    // element count of dimension k of array type t
    int stride_at(Type *t, size_t k) {
        if (!t) return 1;
        int n = 1;
        for (size_t i = k + 1; i < t->dims.size(); i++)
            n *= (t->dims[i] > 0 ? t->dims[i] : 1);
        return n;
    }


    // expressions

    // evaluates n and leaves the result in dest
    void gen_expr(Node *n, const std::string &dest) {
        if (!n) { emit(dest + " = 0"); return; }

        switch (n->kind) {

        case N_INT:
        case N_CHAR:
        case N_FLOAT:
            emit(dest + " = " + placeholder(n));
            return;

        case N_SIZEOF:
            comment("sizeof -> " + itos(n->const_val));
            emit(dest + " = " + itos(n->const_val));
            return;

        case N_STR:
            emit(dest + " = " + string_operand(n));
            return;

        case N_ID:
        case N_INDEX:
        case N_MEMBER:
            emit(dest + " = " + lvalue(n));
            return;

        case N_CAST: {
            if (n->kids.empty()) return;
            Node *src = n->kids[0];
            std::string s = place(src);
            bool from_f = is_float(src->type);
            bool to_f   = is_float(n->type);
            if (from_f && !to_f)      emit(dest + " = ftoi " + s);
            else if (!from_f && to_f) emit(dest + " = itof " + s);
            else                      emit(dest + " = " + s);
            return;
        }

        case N_UN:
            gen_unary(n, dest);
            return;

        case N_BIN:
            gen_binary(n, dest);
            return;

        case N_TERNARY: {
            if (n->kids.size() < 3) { emit(dest + " = 0"); return; }
            std::string c  = place(n->kids[0]);
            std::string le = new_label(), end = new_label();
            emit("IFNOT " + c + " GOTO " + le);
            emit(dest + " = " + place(n->kids[1]));
            emit("GOTO " + end);
            label(le);
            emit(dest + " = " + place(n->kids[2]));
            label(end);
            return;
        }

        case N_ASSIGN:
            gen_assign(n, dest);
            return;

        case N_CALL: {
            if (n->kids.empty()) { emit(dest + " = 0"); return; }
            emit_call(n, dest);
            return;
        }

        case N_LIST:
            for (size_t i = 0; i < n->kids.size(); i++) place(n->kids[i]);
            if (!n->kids.empty()) emit(dest + " = " + place(n->kids.back()));
            return;

        default:
            emit(dest + " = 0");
            return;
        }
    }

    // literals keep their source spelling, char constants become their number
    std::string placeholder(Node *n) {
        if (n->kind == N_CHAR) return itos(n->const_val);
        return n->value;
    }

    void gen_unary(Node *n, const std::string &dest) {
        if (n->kids.empty()) { emit(dest + " = 0"); return; }
        Node *a = n->kids[0];

        if (n->op == "&") {
            if (a->kind == N_INDEX || a->kind == N_MEMBER)
                emit(dest + " = &" + lvalue(a));
            else
                emit(dest + " = &" + place(a));
            return;
        }
        if (n->op == "*") {
            std::string p = place(a);
            emit(dest + " = *" + p);
            return;
        }
        if (n->op == "++" || n->op == "--") {
            std::string lv = lvalue(a);
            std::string one = (n->op == "++") ? "1" : "-1";
            std::string t = new_temp();
            emit(t + " = " + lv + " + " + one);
            emit(lv + " = " + t);
            emit(dest + " = " + t);
            return;
        }

        std::string s = place(a);
        emit(dest + " = " + n->op + s);
    }

    void gen_binary(Node *n, const std::string &dest) {
        if (n->kids.size() < 2) { emit(dest + " = 0"); return; }
        Node *a = n->kids[0], *b = n->kids[1];

        if (n->op == ",") {
            place(a);
            gen_expr(b, dest);
            return;
        }

        std::string x = place(a);
        std::string y = place(b);

        bool xf = is_float(a->type), yf = is_float(b->type);
        if (xf != yf) {
            if (!xf) { std::string t = new_temp(); emit(t + " = itof " + x); x = t; }
            else     { std::string t = new_temp(); emit(t + " = itof " + y); y = t; }
        }

        if (n->op == "+" || n->op == "-") {
            if (a->type && a->type->ptr_depth > 0 && b->type && b->type->ptr_depth == 0) {
                int sz = semantic_type_size(*a->type) / 4;
                if (sz > 1) {
                    std::string t = new_temp();
                    emit(t + " = " + y + " * " + itos(sz));
                    y = t;
                }
            }
        }

        emit(dest + " = " + x + " " + n->op + " " + y);
    }

    void gen_assign(Node *n, const std::string &dest) {
        if (n->kids.size() < 2) { emit(dest + " = 0"); return; }
        Node *lhs = n->kids[0], *rhs = n->kids[1];

        if (n->op == "=") {
            std::string lv = lvalue(lhs);
            std::string rv = place(rhs);
            if (is_float(rhs->type) && lhs->type && !is_float(lhs->type)) {
                std::string t = new_temp();
                emit(t + " = ftoi " + rv);
                rv = t;
            }
            emit(lv + " = " + rv);
            if (!dest.empty()) emit(dest + " = " + lv);
            return;
        }

        std::string base_op = n->op.substr(0, n->op.size() - 1);
        std::string lv = lvalue(lhs);
        std::string rv = place(rhs);
        std::string t = new_temp();
        emit(t + " = " + lv + " " + base_op + " " + rv);
        emit(lv + " = " + t);
        if (!dest.empty()) emit(dest + " = " + t);
    }

    void emit_call(Node *n, const std::string &dest) {
        Node *callee = n->kids[0];
        std::string name = (callee->kind == N_ID) ? callee->name : place(callee);

        std::vector<Node *> args;
        if (n->kids.size() > 1 && n->kids[1]->kind == N_LIST)
            args = n->kids[1]->kids;

        for (size_t i = 0; i < args.size(); i++) {
            std::string a = place(args[i]);
            emit("PARAM " + a);
        }

        if (dest.empty()) emit("CALL " + name + ", " + itos((int)args.size()));
        else              emit(dest + " = CALL " + name + ", " + itos((int)args.size()));
    }


    // statements

    void gen_stmt(Node *n) {
        if (!n) return;

        switch (n->kind) {

        case N_BLOCK:
            for (size_t i = 0; i < n->kids.size(); i++) gen_stmt(n->kids[i]);
            return;

        case N_DECL:
            for (size_t i = 0; i < n->decls.size(); i++) {
                DeclInfo *d = n->decls[i];
                if (d->is_typedef || d->is_function || !d->init) continue;

                std::string ty = d->spec ? d->spec->base : "INT";
                comment("declare " + ty + " " + d->name);

                if (d->init->kind == N_INITLIST) {
                    std::string base = d->name;
                    for (size_t k = 0; k < d->init->kids.size(); k++) {
                        std::string v = place(d->init->kids[k]);
                        emit(base + "[" + itos((int)k * 4) + "] = " + v);
                    }
                } else {
                    Node tmp;
                    tmp.kind = N_ASSIGN;
                    tmp.op = "=";
                    tmp.line = n->line;
                    Node *id = mk_id(d->name, n->line);
                    Type *t = new Type();
                    t->base = ty;
                    t->ptr_depth = d->ptr_depth;
                    for (size_t k = 0; k < d->dims.size(); k++)
                        t->dims.push_back(atoi(d->dims[k].c_str()));
                    id->type = t;
                    id->lvalue = true;
                    tmp.kids.push_back(id);
                    tmp.kids.push_back(d->init);
                    gen_assign(&tmp, "");
                }
            }
            return;

        case N_EXPRSTMT:
            for (size_t i = 0; i < n->kids.size(); i++) {
                Node *e = n->kids[i];
                if (e && e->kind == N_CALL) emit_call(e, "");
                else if (e) place(e);
            }
            return;

        case N_IF: {
            std::string c = place(n->kids.size() > 0 ? n->kids[0] : NULL);
            std::string lelse = new_label();
            std::string lend  = new_label();

            bool has_else = n->kids.size() > 2;

            emit("IFNOT " + c + " GOTO " + (has_else ? lelse : lend));
            if (n->kids.size() > 1) gen_stmt(n->kids[1]);
            if (has_else) {
                emit("GOTO " + lend);
                label(lelse);
                gen_stmt(n->kids[2]);
            }
            label(lend);
            return;
        }

        case N_WHILE: {
            std::string lstart = new_label();
            std::string lend   = new_label();
            label(lstart);
            std::string c = place(n->kids.size() > 0 ? n->kids[0] : NULL);
            emit("IFNOT " + c + " GOTO " + lend);
            loops.push_back(make_ctx(lend, lstart));
            if (n->kids.size() > 1) gen_stmt(n->kids[1]);
            loops.pop_back();
            emit("GOTO " + lstart);
            label(lend);
            return;
        }

        case N_UNTIL: {
            std::string lstart = new_label();
            std::string lend   = new_label();
            label(lstart);
            std::string c = place(n->kids.size() > 0 ? n->kids[0] : NULL);
            emit("IF " + c + " GOTO " + lend);
            loops.push_back(make_ctx(lend, lstart));
            if (n->kids.size() > 1) gen_stmt(n->kids[1]);
            loops.pop_back();
            emit("GOTO " + lstart);
            label(lend);
            return;
        }

        case N_DO: {
            std::string lstart = new_label();
            std::string lcont  = new_label();
            std::string lend   = new_label();
            label(lstart);
            loops.push_back(make_ctx(lend, lcont));
            if (n->kids.size() > 0) gen_stmt(n->kids[0]);
            loops.pop_back();
            label(lcont);
            std::string c = place(n->kids.size() > 1 ? n->kids[1] : NULL);
            emit("IF " + c + " GOTO " + lstart);
            label(lend);
            return;
        }

        case N_FOR: {
            std::string lstart = new_label();
            std::string lcont  = new_label();
            std::string lend   = new_label();

            if (n->kids.size() > 0) {
                Node *init = n->kids[0];
                if (init->kind == N_DECL) gen_stmt(init);
                else if (init->kind == N_EXPRSTMT) gen_stmt(init);
                else if (init->kind != N_BLOCK) place(init);
            }

            label(lstart);
            if (n->kids.size() > 1) {
                Node *cond = n->kids[1];
                if (cond && cond->kind == N_EXPRSTMT)
                    cond = cond->kids.empty() ? NULL : cond->kids[0];
                else if (cond && cond->kind == N_BLOCK)
                    cond = NULL;                 

                if (cond) {
                    std::string c = place(cond);
                    emit("IFNOT " + c + " GOTO " + lend);
                }
            }

            loops.push_back(make_ctx(lend, lcont));
            if (n->kids.size() > 3) gen_stmt(n->kids[3]);
            loops.pop_back();

            label(lcont);
            if (n->kids.size() > 2 && n->kids[2]->kind != N_BLOCK) {
                Node *step = n->kids[2];
                if (step->kind == N_EXPRSTMT) gen_stmt(step);
                else place(step);
            }
            emit("GOTO " + lstart);
            label(lend);
            return;
        }

        case N_RETURN:
            if (n->kids.empty() || !n->kids[0] || n->kids[0]->kind == N_BLOCK)
                emit("RETURN");
            else
                emit("RETURN " + place(n->kids[0]));
            return;

        case N_BREAK:
            if (!loops.empty()) emit("GOTO " + loops.back().break_label);
            return;

        case N_CONTINUE:
            if (!loops.empty()) emit("GOTO " + loops.back().cont_label);
            return;

        case N_GOTO:
            emit("GOTO " + user_label(n->name));
            return;

        case N_LABEL:
            label(user_label(n->name));
            if (!n->kids.empty()) gen_stmt(n->kids[0]);
            return;

        default:
            place(n);
            return;
        }
    }

    LoopCtx make_ctx(const std::string &b, const std::string &c) {
        LoopCtx l;
        l.break_label = b;
        l.cont_label  = c;
        return l;
    }


    // top level

    void gen_function(Node *n) {
        std::string name = n->decl ? n->decl->name : "?";
        std::string ret  = n->spec ? n->spec->base : "INT";

        temp_count = 0;
        label_count = 0;
        loops.clear();
        strings.clear();
        out.clear();

        std::string sig = name + "(";
        for (size_t i = 0; i < n->decl->params.size(); i++) {
            if (i) sig += ", ";
            DeclInfo *p = n->decl->params[i];
            std::string ty = p->spec ? p->spec->base : "INT";
            for (int k = 0; k < p->ptr_depth; k++) ty += "*";
            sig += ty + " " + (p->name.empty() ? std::string("_") : p->name);
        }
        sig += ") -> " + ret;

        out.push_back("FUNC " + sig + ":");
        blank();

        for (size_t i = 0; i < n->kids.size(); i++) gen_stmt(n->kids[i]);

        blank();
        out.push_back("END FUNC " + name);
    }

    // globals and structs get a header block instead of instructions
    void gen_global(Node *n) {
        if (!n->decls.empty()) {
            for (size_t i = 0; i < n->decls.size(); i++) {
                DeclInfo *d = n->decls[i];
                if (d->is_function) continue;
                std::string ty = d->spec ? d->spec->base : "INT";
                for (int k = 0; k < d->ptr_depth; k++) ty += "*";
                for (size_t k = 0; k < d->dims.size(); k++)
                    ty += "[" + d->dims[k] + "]";
                std::string line = "GLOBAL " + ty + " " + d->name;
                if (d->init && d->init->kind != N_INITLIST) line += " = " + d->init->value;
                out.push_back(line);
            }
        }
        if (n->spec && !n->spec->members.empty()) {
            out.push_back("STRUCT " + n->spec->base.substr(7) + " {");
            for (size_t i = 0; i < n->spec->members.size(); i++) {
                DeclInfo *m = n->spec->members[i];
                std::string ty = m->spec ? m->spec->base : "INT";
                for (size_t k = 0; k < m->dims.size(); k++) ty += "[" + m->dims[k] + "]";
                out.push_back("  " + ty + " " + m->name + ";");
            }
            out.push_back("}");
        }
    }
};

}


std::string generate_ir(Node *root) {
    Gen g;
    if (!root) return "";

    g.out.push_back("// ---- intermediate code (3-address) ----");
    g.out.push_back("// generated from the source program");
    g.blank();

    // globals and struct definitions
    bool any_global = false;
    for (size_t i = 0; i < root->kids.size(); i++) {
        Node *n = root->kids[i];
        if (n->kind == N_DECL) { g.gen_global(n); any_global = true; }
    }
    if (any_global) g.blank();

    // each function
    for (size_t i = 0; i < root->kids.size(); i++) {
        Node *n = root->kids[i];
        if (n->kind != N_FUNC) continue;

        Gen f;
        f.gen_function(n);
        for (size_t k = 0; k < f.out.size(); k++) g.out.push_back(f.out[k]);
        g.blank();
    }

    std::ostringstream ss;
    for (size_t i = 0; i < g.out.size(); i++) ss << g.out[i] << "\n";
    return ss.str();
}
