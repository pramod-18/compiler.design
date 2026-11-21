#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include "semantic.h"
#include "targetgen.h"
#include "target_runtime.h"

namespace {

Type integer_type() { Type t; t.base = "INT"; return t; }
Type double_type() { Type t; t.base = "DOUBLE"; return t; }
bool floating(const Type &t) { return !t.ptr_depth && t.dims.empty() && t.is_float(); }
bool aggregate(const Type &t) { return !t.dims.empty() || (!t.ptr_depth && t.is_struct()); }
Type node_type(Node *n) { return n && n->type ? *n->type : integer_type(); }
std::string number(int n) { return std::to_string(n); }

struct Storage {
    std::string label;
    int offset = 0;
    bool array_parameter = false;
};

class Target {
    std::ostringstream data, text;
    std::map<DeclInfo *, Storage> storage;
    std::map<Node *, int> result_offsets;
    std::map<std::string, Node *> functions;
    std::map<std::string, std::string> strings;
    std::vector<std::pair<std::string, std::string>> loops;
    std::set<std::string> user_labels;
    Node *current = nullptr;
    std::string epilogue;
    int serial = 0, frame_used = 0;
    int current_frame = 0;

    void emit(const std::string &s) { text << "    " << s << '\n'; }
    void label(const std::string &s) { text << s << ":\n"; }
    std::string fresh(const std::string &s = "label") { return "__cg_" + s + "_" + number(++serial); }
    std::string function_label(const std::string &s) { return "__fn_" + s; }
    void jump(const std::string &s) { emit("b " + s); emit("nop"); }
    void call(const std::string &s) { emit("jal " + s); emit("nop"); }
    void branch(const std::string &s) { emit(s); emit("nop"); }
    void fail(Node *n, const std::string &s) {
        throw std::runtime_error("target generation at line " + number(n ? n->line : 0) + ": " + s);
    }

    void address_offset(const std::string &dest, const std::string &base, int offset) {
        if (offset >= -32768 && offset <= 32767)
            emit("addiu " + dest + ", " + base + ", " + number(offset));
        else {
            emit("li $t9, " + number(offset));
            emit("addu " + dest + ", " + base + ", $t9");
        }
    }

    int size(const Type &t) {
        long long count = 1;
        for (int d : t.dims) {
            if (d <= 0) throw std::runtime_error("target requires complete constant array bounds");
            count *= d;
            if (count > 16777216) throw std::runtime_error("array exceeds target storage limit");
        }
        int result = semantic_type_size(t);
        if (result <= 0 || result > 67108864) throw std::runtime_error("invalid target object size");
        return result;
    }

    int stride(Type t) {
        if (!t.dims.empty()) t.dims.erase(t.dims.begin());
        else if (t.ptr_depth) t.ptr_depth--;
        return t.base == "VOID" && !t.ptr_depth && t.dims.empty() ? 1 : size(t);
    }

    void reserve(DeclInfo *d, bool parameter = false) {
        if (!d || d->is_function || d->is_typedef || d->name.empty() || storage.count(d)) return;
        if (!d->type) throw std::runtime_error("missing declaration type: " + d->name);
        int bytes = parameter && !d->type->dims.empty() ? 4 : size(*d->type);
        int alignment = parameter ? 8 : semantic_type_alignment(*d->type);
        frame_used = (frame_used + alignment - 1) / alignment * alignment;
        if (frame_used > 67108864 - bytes) throw std::runtime_error("target stack frame too large");
        storage[d] = {"", frame_used, parameter && !d->type->dims.empty()};
        frame_used += bytes;
    }

    void collect_locals(Node *n) {
        if (!n) return;
        if (n->kind == N_DECL) for (DeclInfo *d : n->decls) { reserve(d); collect_locals(d->init); }
        if (n->kind == N_CALL && node_type(n).is_struct() && !node_type(n).ptr_depth) {
            frame_used = (frame_used + 7) / 8 * 8;
            result_offsets[n] = frame_used;
            frame_used += size(node_type(n));
        }
        if (n->kind == N_LABEL && !user_labels.insert(n->name).second) fail(n, "duplicate label '" + n->name + "'");
        for (Node *k : n->kids) collect_locals(k);
    }

    std::string string_label(const std::string &bytes) {
        auto found = strings.find(bytes);
        if (found != strings.end()) return found->second;
        std::string name = fresh("string");
        strings[bytes] = name;
        data << name << ":\n    .byte ";
        for (unsigned char c : bytes) data << (unsigned)c << ", ";
        data << "0\n";
        return name;
    }

    void object_address(DeclInfo *d) {
        auto it = storage.find(d);
        if (it == storage.end()) throw std::runtime_error("unresolved target variable");
        if (!it->second.label.empty()) emit("la $v0, " + it->second.label);
        else address_offset("$v0", "$fp", it->second.offset);
    }

    void address(Node *n) {
        if (!n) fail(n, "missing lvalue");
        if (n->kind == N_ID) { object_address(n->decl); return; }
        if (n->kind == N_UN && n->op == "*") { expression(n->kids[0]); return; }
        if (n->kind == N_INDEX) {
            expression(n->kids[0]); push(integer_type());
            expression(n->kids[1]); convert(node_type(n->kids[1]), integer_type());
            emit("li $t1, " + number(size(node_type(n))));
            emit("mul $v0, $v0, $t1");
            pop(integer_type(), "$t0"); emit("addu $v0, $t0, $v0");
            return;
        }
        if (n->kind == N_MEMBER) {
            if (n->op == "->") expression(n->kids[0]);
            else address(n->kids[0]);
            int offset = semantic_member_offset(node_type(n->kids[0]), n->name);
            if (offset < 0) fail(n, "unknown structure member");
            address_offset("$v0", "$v0", offset);
            return;
        }
        if (n->kind == N_CALL && aggregate(node_type(n))) { expression(n); return; }
        fail(n, "expression has no target address");
    }

    void load(const Type &t) {
        if (aggregate(t)) return;
        if (floating(t)) emit(std::string(t.base == "DOUBLE" ? "l.d" : "l.s") + " $f0, 0($v0)");
        else emit(std::string(!t.ptr_depth && t.base == "CHAR" ? "lb" : "lw") + " $v0, 0($v0)");
    }

    void store(const Type &t, const std::string &address_reg = "$t0") {
        if (floating(t)) emit(std::string(t.base == "DOUBLE" ? "s.d" : "s.s") + " $f0, 0(" + address_reg + ")");
        else emit(std::string(!t.ptr_depth && t.dims.empty() && t.base == "CHAR" ? "sb" : "sw") + " $v0, 0(" + address_reg + ")");
    }

    void push(const Type &t) {
        emit("addiu $sp, $sp, -8");
        if (floating(t)) emit(std::string(t.base == "DOUBLE" ? "s.d" : "s.s") + " $f0, 0($sp)");
        else emit("sw $v0, 0($sp)");
    }

    void pop(const Type &t, const std::string &reg = "$v0") {
        if (floating(t)) emit(std::string(t.base == "DOUBLE" ? "l.d" : "l.s") + " $f0, 0($sp)");
        else emit("lw " + reg + ", 0($sp)");
        emit("addiu $sp, $sp, 8");
    }

    void convert(const Type &from, const Type &to) {
        if (floating(from) && floating(to)) {
            if (from.base != to.base) emit(std::string(to.base == "DOUBLE" ? "cvt.d.s" : "cvt.s.d") + " $f0, $f0");
        } else if (floating(from)) {
            emit(std::string(from.base == "DOUBLE" ? "trunc.w.d" : "trunc.w.s") + " $f0, $f0");
            emit("mfc1 $v0, $f0");
        } else if (floating(to)) {
            emit("mtc1 $v0, $f0");
            emit(std::string(to.base == "DOUBLE" ? "cvt.d.w" : "cvt.s.w") + " $f0, $f0");
        }
        if (!to.ptr_depth && to.dims.empty() && to.base == "CHAR") {
            emit("sll $v0, $v0, 24"); emit("sra $v0, $v0, 24");
        }
    }

    void truth(const Type &t) {
        if (!floating(t)) { emit("sltu $v0, $zero, $v0"); return; }
        emit("mtc1 $zero, $f2"); emit("mtc1 $zero, $f3");
        emit(std::string(t.base == "DOUBLE" ? "c.eq.d" : "c.eq.s") + " $f0, $f2");
        std::string end = fresh(); emit("li $v0, 0"); branch("bc1t " + end);
        emit("li $v0, 1"); label(end);
    }

    void integer_binary(const std::string &op) {
        if (op == "+") emit("addu $v0, $t0, $t1");
        else if (op == "-") emit("subu $v0, $t0, $t1");
        else if (op == "*") emit("mul $v0, $t0, $t1");
        else if (op == "/" || op == "%") {
            emit("div $t0, $t1"); emit(op == "/" ? "mflo $v0" : "mfhi $v0");
        } else if (op == "&") emit("and $v0, $t0, $t1");
        else if (op == "|") emit("or $v0, $t0, $t1");
        else if (op == "^") emit("xor $v0, $t0, $t1");
        else if (op == "<<") emit("sllv $v0, $t0, $t1");
        else if (op == ">>") emit("srav $v0, $t0, $t1");
        else if (op == "<") emit("slt $v0, $t0, $t1");
        else if (op == ">") emit("slt $v0, $t1, $t0");
        else if (op == "<=") { emit("slt $v0, $t1, $t0"); emit("xori $v0, $v0, 1"); }
        else if (op == ">=") { emit("slt $v0, $t0, $t1"); emit("xori $v0, $v0, 1"); }
        else if (op == "==" || op == "!=") {
            emit("xor $v0, $t0, $t1"); emit("sltu $v0, $zero, $v0");
            if (op == "==") emit("xori $v0, $v0, 1");
        } else throw std::runtime_error("unsupported target operator " + op);
    }

    void float_binary(const std::string &op, const Type &t) {
        std::string suffix = t.base == "DOUBLE" ? ".d" : ".s";
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            std::string mnemonic = op == "+" ? "add" : op == "-" ? "sub" : op == "*" ? "mul" : "div";
            emit(mnemonic + suffix + " $f0, $f0, $f2"); return;
        }
        std::string cmp = op == "==" || op == "!=" ? "eq" : op == "<=" || op == ">=" ? "le" : "lt";
        if (op != "==" && op != "!=" && op != "<" && op != ">" && op != "<=" && op != ">=")
            throw std::runtime_error("integer operator applied to floating point");
        emit("c." + cmp + suffix + (op == ">" || op == ">=" ? " $f2, $f0" : " $f0, $f2"));
        std::string end = fresh(); emit("li $v0, 0");
        branch(std::string(op == "!=" ? "bc1t " : "bc1f ") + end);
        emit("li $v0, 1"); label(end);
    }

    void binary(Node *n) {
        Node *a = n->kids[0], *b = n->kids[1];
        if (n->op == ",") { expression(a); expression(b); return; }
        if (n->op == "&&" || n->op == "||") {
            std::string end = fresh(); expression(a); truth(node_type(a));
            branch(std::string(n->op == "&&" ? "beq" : "bne") + " $v0, $zero, " + end);
            expression(b); truth(node_type(b)); label(end); return;
        }
        Type at = node_type(a), bt = node_type(b);
        Type common = floating(at) || floating(bt) ? (at.base == "DOUBLE" || bt.base == "DOUBLE" ? double_type() : at.base == "FLOAT" ? at : bt) : integer_type();
        expression(a); convert(at, common); push(common);
        expression(b); convert(bt, common);
        if (floating(common)) {
            emit(std::string(common.base == "DOUBLE" ? "mov.d" : "mov.s") + " $f2, $f0");
            pop(common); float_binary(n->op, common);
        } else {
            emit("move $t1, $v0"); pop(common, "$t0");
            bool ap = at.ptr_depth || !at.dims.empty(), bp = bt.ptr_depth || !bt.dims.empty();
            if (n->op == "+" || n->op == "-") {
                if (ap && !bp) { emit("li $t2, " + number(stride(at))); emit("mul $t1, $t1, $t2"); }
                else if (bp && !ap && n->op == "+") { emit("li $t2, " + number(stride(bt))); emit("mul $t0, $t0, $t2"); }
            }
            integer_binary(n->op);
            if (ap && bp && n->op == "-") {
                emit("li $t2, " + number(stride(at))); emit("div $v0, $t2"); emit("mflo $v0");
            }
        }
    }

    void assignment(Node *n) {
        Node *lhs = n->kids[0], *rhs = n->kids[1];
        Type lt = node_type(lhs), rt = node_type(rhs);
        address(lhs); push(integer_type());
        if (n->op == "=") {
            expression(rhs);
            if (aggregate(lt)) {
                if (lt.base != rt.base || !rt.dims.empty()) fail(n, "incompatible aggregate assignment");
                emit("move $a1, $v0"); pop(integer_type(), "$a0");
                emit("li $a2, " + number(size(lt))); call("__rt_copy"); return;
            }
            convert(rt, lt);
        } else {
            load(lt);
            Type common = floating(lt) || floating(rt) ? (lt.base == "DOUBLE" || rt.base == "DOUBLE" ? double_type() : floating(lt) ? lt : rt) : integer_type();
            convert(lt, common); push(common); expression(rhs); convert(rt, common);
            if (floating(common)) {
                emit(std::string(common.base == "DOUBLE" ? "mov.d" : "mov.s") + " $f2, $f0");
                pop(common); float_binary(n->op.substr(0, n->op.size() - 1), common);
            } else {
                emit("move $t1, $v0"); pop(common, "$t0");
                if (lt.ptr_depth) { emit("li $t2, " + number(stride(lt))); emit("mul $t1, $t1, $t2"); }
                integer_binary(n->op.substr(0, n->op.size() - 1));
            }
            convert(common, lt);
        }
        pop(integer_type(), "$t0"); store(lt);
    }

    void unary(Node *n) {
        Node *a = n->kids[0]; Type t = node_type(a);
        if (n->op == "&") { address(a); return; }
        if (n->op == "*") { expression(a); load(node_type(n)); return; }
        if (n->op == "++" || n->op == "--") {
            address(a); push(integer_type()); load(t);
            if (n->postfix) push(t);
            if (floating(t)) {
                emit("li $t0, 1"); emit("mtc1 $t0, $f2");
                emit(std::string(t.base == "DOUBLE" ? "cvt.d.w" : "cvt.s.w") + " $f2, $f2");
                emit(std::string(n->op == "++" ? "add" : "sub") + (t.base == "DOUBLE" ? ".d" : ".s") + " $f0, $f0, $f2");
            } else {
                int step = t.ptr_depth ? stride(t) : 1;
                address_offset("$v0", "$v0", n->op == "++" ? step : -step);
                convert(t, t);
            }
            emit(std::string("lw $t0, ") + (n->postfix ? "8" : "0") + "($sp)"); store(t);
            if (n->postfix) pop(t);
            emit("addiu $sp, $sp, 8"); return;
        }
        expression(a);
        if (n->op == "!") { truth(t); emit("xori $v0, $v0, 1"); }
        else if (n->op == "-") {
            if (floating(t)) emit(std::string(t.base == "DOUBLE" ? "neg.d" : "neg.s") + " $f0, $f0");
            else emit("subu $v0, $zero, $v0");
        } else if (n->op == "~") {
            if (floating(t)) fail(n, "bitwise complement requires an integer");
            emit("nor $v0, $v0, $zero");
        }
    }

    void invocation(Node *n) {
        std::string name = n->kids[0]->name;
        std::vector<Node *> args = n->kids.size() > 1 ? n->kids[1]->kids : std::vector<Node *>{};
        bool user = functions.count(name);
        if (!user) validate_builtin(n, name, args);
        bool struct_return = user && aggregate(*functions[name]->type);
        int prefix = struct_return ? 8 : 0;
        int slots = (int)args.size() * 8 + prefix;
        int bytes = slots;
        if (user) for (DeclInfo *p : functions[name]->decl->params)
            if (p->type && p->type->dims.empty() && !p->type->ptr_depth && p->type->is_struct()) bytes += (size(*p->type) + 7) / 8 * 8;
        if (bytes) address_offset("$sp", "$sp", -bytes);
        if (struct_return) {
            address_offset("$v0", "$fp", result_offsets.at(n)); emit("sw $v0, 0($sp)");
        }
        int copy_offset = slots;
        for (size_t i = 0; i < args.size(); i++) {
            Type to = node_type(args[i]);
            if (user) {
                DeclInfo *p = functions[name]->decl->params[i];
                to = *p->type;
                if (!to.dims.empty()) { to.dims.clear(); to.ptr_depth++; }
            } else if (floating(to)) to = double_type();
            if (!to.dims.empty()) { to.dims.clear(); to.ptr_depth++; }
            expression(args[i]);
            if (aggregate(to)) {
                if (node_type(args[i]).base != to.base || !node_type(args[i]).dims.empty()) fail(n, "incompatible structure argument");
                emit("move $a1, $v0"); address_offset("$a0", "$sp", copy_offset);
                emit("li $a2, " + number(size(to))); call("__rt_copy");
                address_offset("$t0", "$sp", prefix + (int)i * 8); emit("sw $v0, 0($t0)");
                copy_offset += (size(to) + 7) / 8 * 8;
            } else {
                convert(node_type(args[i]), to);
                address_offset("$t0", "$sp", prefix + (int)i * 8); store(to);
            }
        }
        call(user ? function_label(name) : "__rt_" + name);
        if (bytes) address_offset("$sp", "$sp", bytes);
    }

    void validate_builtin(Node *n, const std::string &name, const std::vector<Node *> &args) {
        static const std::map<std::string, int> arities = {
            {"malloc",1},{"calloc",2},{"realloc",2},{"free",1},{"fopen",2},{"fclose",1},
            {"fgets",3},{"fputs",2},{"fgetc",1},{"fputc",2},{"strlen",1},{"strcpy",2},
            {"strcmp",2},{"atoi",1},{"abs",1},{"exit",1},{"rand",0},{"srand",1},
            {"puts",1},{"putchar",1},{"getchar",0}
        };
        bool output = name == "printf" || name == "fprintf" || name == "sprintf";
        bool input = name == "scanf" || name == "fscanf" || name == "sscanf";
        if (!output && !input) {
            auto it = arities.find(name);
            if (it == arities.end()) fail(n, "no SPIM runtime for '" + name + "'");
            if ((int)args.size() != it->second) fail(n, "wrong argument count for '" + name + "'");
            static const std::map<std::string, std::string> shapes = {
                {"malloc","i"},{"calloc","ii"},{"realloc","pi"},{"free","p"},
                {"fopen","ss"},{"fclose","p"},{"fgets","sip"},{"fputs","sp"},
                {"fgetc","p"},{"fputc","ip"},{"strlen","s"},{"strcpy","ss"},
                {"strcmp","ss"},{"atoi","s"},{"abs","i"},{"exit","i"},
                {"srand","i"},{"puts","s"},{"putchar","i"},{"rand",""},{"getchar",""}
            };
            for (size_t i = 0; i < args.size(); i++) {
                Type t = node_type(args[i]);
                bool ptr = t.ptr_depth || !t.dims.empty();
                bool null = args[i]->kind == N_INT && args[i]->const_val == 0;
                char shape = shapes.at(name)[i];
                bool valid = shape == 'p' ? ptr || null : shape == 's' ? ptr && t.base == "CHAR" :
                    !ptr && !floating(t) && !aggregate(t) && t.base != "VOID";
                if (!valid) fail(n, "wrong argument type for '" + name + "'");
            }
            if (name == "fopen") {
                if (args[1]->kind != N_STR) fail(n, "fopen mode must be a literal r, w or a");
                std::string mode = decode_string_literal(args[1]->value);
                if (mode != "r" && mode != "w" && mode != "a" && mode != "rb" && mode != "wb" && mode != "ab")
                    fail(n, "fopen supports r, w and a modes");
            }
            return;
        }
        int format = name == "printf" || name == "scanf" ? 0 : 1;
        if ((int)args.size() <= format || args[format]->kind != N_STR) fail(n, "formatted I/O requires a literal format string");
        if (format) {
            Type handle = node_type(args[0]);
            bool pointer = handle.ptr_depth || !handle.dims.empty();
            if (!pointer || ((name == "sprintf" || name == "sscanf") && handle.base != "CHAR"))
                fail(n, "invalid formatted I/O stream or buffer");
        }
        std::string s = decode_string_literal(args[format]->value);
        size_t arg = format + 1;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] != '%') continue;
            if (++i == s.size()) fail(n, "incomplete I/O format");
            if (s[i] == '%') continue;
            bool wide = false;
            if (input && s[i] == 'l' && i + 1 < s.size()) { wide = true; i++; }
            if (std::string("diucsf").find(s[i]) == std::string::npos || (wide && s[i] != 'f'))
                fail(n, "supported formats are %d, %i, %u, %c, %s, %f, %% and input %lf");
            if (arg >= args.size()) fail(n, "too few arguments for I/O format");
            Type t = node_type(args[arg++]);
            bool pointer = t.ptr_depth || !t.dims.empty();
            if (input && !pointer) fail(n, "scanf arguments must be addresses");
            if (input) {
                Type element = t;
                if (!element.dims.empty()) element.dims.erase(element.dims.begin());
                else element.ptr_depth--;
                bool valid = s[i] == 'f' ? floating(element) && element.base == (wide ? "DOUBLE" : "FLOAT") :
                    s[i] == 's' || s[i] == 'c' ? element.base == "CHAR" && !element.ptr_depth :
                    !floating(element) && !aggregate(element) && !element.ptr_depth && element.base != "CHAR";
                if (!valid) fail(n, "scanf format and destination type disagree");
            }
            if (output && ((s[i] == 's' && !pointer) || (s[i] == 'f' && !floating(t)) ||
                (s[i] != 's' && s[i] != 'f' && (pointer || floating(t)))))
                fail(n, "printf format and argument type disagree");
        }
    }

    void expression(Node *n) {
        if (!n) { emit("move $v0, $zero"); return; }
        Type t = node_type(n);
        switch (n->kind) {
            case N_INT: case N_CHAR: case N_SIZEOF:
                emit("li $v0, " + number(n->const_val)); return;
            case N_FLOAT: {
                std::string literal = fresh("real");
                std::string value = n->value;
                if (value.back() == 'f' || value.back() == 'F' || value.back() == 'l' || value.back() == 'L') value.pop_back();
                data << "    .align " << (t.base == "DOUBLE" ? 3 : 2) << '\n' << literal
                     << ": " << (t.base == "DOUBLE" ? ".double " : ".float ") << value << '\n';
                emit("la $t0, " + literal); emit(std::string(t.base == "DOUBLE" ? "l.d" : "l.s") + " $f0, 0($t0)"); return;
            }
            case N_STR: emit("la $v0, " + string_label(decode_string_literal(n->value))); return;
            case N_ID:
                if (!n->decl) fail(n, "function values are not supported");
                address(n);
                if (storage[n->decl].array_parameter) emit("lw $v0, 0($v0)");
                else load(t);
                return;
            case N_INDEX: case N_MEMBER: address(n); load(t); return;
            case N_BIN: binary(n); return;
            case N_UN: unary(n); return;
            case N_ASSIGN: assignment(n); return;
            case N_CAST: expression(n->kids[0]); convert(node_type(n->kids[0]), t); return;
            case N_CALL: invocation(n); return;
            case N_TERNARY: {
                std::string otherwise = fresh(), end = fresh();
                expression(n->kids[0]); truth(node_type(n->kids[0])); branch("beq $v0, $zero, " + otherwise);
                expression(n->kids[1]); convert(node_type(n->kids[1]), t); jump(end); label(otherwise);
                expression(n->kids[2]); convert(node_type(n->kids[2]), t); label(end); return;
            }
            case N_LIST: case N_EXPRSTMT:
                for (Node *k : n->kids) expression(k);
                return;
            case N_BLOCK: return;
            default: fail(n, "unsupported target expression");
        }
    }

    void write_initializer(DeclInfo *d, const Type &t, int offset, Node *value) {
        object_address(d); address_offset("$v0", "$v0", offset); push(integer_type());
        expression(value); convert(node_type(value), t); pop(integer_type(), "$t0"); store(t);
    }

    void initialize(DeclInfo *d, Type t, int offset, Node *init) {
        if (!init) return;
        if (!t.dims.empty() && t.base == "CHAR" && !t.ptr_depth && t.dims.size() == 1 && init->kind == N_STR) {
            std::string bytes = decode_string_literal(init->value);
            if ((int)bytes.size() > t.dims[0]) fail(init, "string initializer exceeds array bound");
            for (size_t i = 0; i < bytes.size(); i++) {
                object_address(d); address_offset("$t0", "$v0", offset + (int)i);
                emit("li $v0, " + number((unsigned char)bytes[i])); store(integer_char());
            }
            return;
        }
        if (!aggregate(t)) {
            if (init->kind == N_INITLIST) {
                if (init->kids.size() != 1) fail(init, "scalar initializer needs one value");
                init = init->kids[0];
            }
            write_initializer(d, t, offset, init); return;
        }
        if (t.dims.empty()) {
            if (init->kind == N_INITLIST) {
                auto members = semantic_struct_members(t);
                if (init->kids.size() > members.size()) fail(init, "too many structure initializer members");
                for (size_t i = 0; i < init->kids.size(); i++)
                    initialize(d, members[i].second, offset + semantic_member_offset(t, members[i].first), init->kids[i]);
                return;
            }
            if (node_type(init).base != t.base || node_type(init).ptr_depth || !node_type(init).dims.empty())
                fail(init, "incompatible structure initializer");
            object_address(d); address_offset("$v0", "$v0", offset); push(integer_type()); expression(init);
            emit("move $a1, $v0"); pop(integer_type(), "$a0"); emit("li $a2, " + number(size(t))); call("__rt_copy"); return;
        }
        if (init->kind != N_INITLIST) fail(init, "array initializer requires braces or a string");
        Type element = t; element.dims.erase(element.dims.begin());
        if (!init->kids.empty() && (init->kids[0]->kind == N_INITLIST || (!element.dims.empty() && init->kids[0]->kind == N_STR))) {
            if ((int)init->kids.size() > t.dims[0]) fail(init, "too many array initializer rows");
            for (size_t i = 0; i < init->kids.size(); i++) initialize(d, element, offset + (int)i * size(element), init->kids[i]);
        } else {
            Type scalar = t; scalar.dims.clear();
            int count = size(t) / size(scalar);
            if ((int)init->kids.size() > count) fail(init, "too many array initializer elements");
            for (size_t i = 0; i < init->kids.size(); i++) initialize(d, scalar, offset + (int)i * size(scalar), init->kids[i]);
        }
    }

    Type integer_char() { Type t; t.base = "CHAR"; return t; }

    void declaration(DeclInfo *d) {
        if (d->is_typedef || d->is_function || !d->init) return;
        if (aggregate(*d->type)) {
            object_address(d); emit("move $a0, $v0"); emit("li $a1, " + number(size(*d->type))); call("__rt_zero");
        }
        initialize(d, *d->type, 0, d->init);
    }

    Node *condition(Node *n) {
        if (!n || (n->kind == N_BLOCK && n->kids.empty())) return nullptr;
        if (n->kind == N_EXPRSTMT) return n->kids.empty() ? nullptr : n->kids[0];
        return n;
    }

    void condition_branch(Node *n, const std::string &dest, bool when_true = false) {
        n = condition(n); if (!n) return;
        expression(n); truth(node_type(n));
        branch(std::string(when_true ? "bne" : "beq") + " $v0, $zero, " + dest);
    }

    std::string user_label(const std::string &name) { return "__user_" + current->decl->name + "_" + name; }

    void statement(Node *n) {
        if (!n) return;
        switch (n->kind) {
            case N_BLOCK: for (Node *k : n->kids) statement(k); return;
            case N_DECL: for (DeclInfo *d : n->decls) declaration(d); return;
            case N_EXPRSTMT: expression(n); return;
            case N_RETURN:
                if (!n->kids.empty()) {
                    expression(n->kids[0]);
                    if (aggregate(*current->type)) {
                        if (node_type(n->kids[0]).base != current->type->base) fail(n, "incompatible structure return");
                        emit("move $a1, $v0"); address_offset("$t0", "$fp", current_frame);
                        emit("lw $a0, 0($t0)"); emit("li $a2, " + number(size(*current->type))); call("__rt_copy");
                    } else convert(node_type(n->kids[0]), *current->type);
                }
                else emit("move $v0, $zero");
                jump(epilogue); return;
            case N_IF: {
                std::string otherwise = fresh(), end = fresh();
                condition_branch(n->kids[0], otherwise); statement(n->kids[1]); jump(end); label(otherwise);
                if (n->kids.size() > 2) statement(n->kids[2]);
                label(end); return;
            }
            case N_WHILE: case N_UNTIL: case N_DO: case N_FOR: {
                std::string start = fresh(), next = fresh(), end = fresh();
                bool counted = n->kind == N_FOR, posttest = n->kind == N_DO;
                if (counted) statement(n->kids[0]);
                label(start);
                if (!posttest) condition_branch(n->kids[counted ? 1 : 0], end, n->kind == N_UNTIL);
                loops.push_back({end, posttest || counted ? next : start});
                statement(n->kids[counted ? 3 : posttest ? 0 : 1]); loops.pop_back(); label(next);
                if (counted) statement(n->kids[2]);
                if (posttest) condition_branch(n->kids[1], start, true);
                else jump(start);
                label(end); return;
            }
            case N_BREAK: jump(loops.back().first); return;
            case N_CONTINUE: jump(loops.back().second); return;
            case N_GOTO: jump(user_label(n->name)); return;
            case N_LABEL: label(user_label(n->name)); if (!n->kids.empty()) statement(n->kids[0]); return;
            default: expression(n); return;
        }
    }

    void prologue(int frame) {
        address_offset("$sp", "$sp", -frame);
        address_offset("$t0", "$sp", frame - 8); emit("sw $fp, 0($t0)"); emit("sw $ra, 4($t0)");
        emit("move $fp, $sp");
    }

    void finish(int frame) {
        label(epilogue); emit("move $sp, $fp");
        address_offset("$t0", "$sp", frame - 8); emit("lw $fp, 0($t0)"); emit("lw $ra, 4($t0)");
        address_offset("$sp", "$sp", frame); emit("jr $ra"); emit("nop");
    }

    void function(Node *n) {
        current = n; frame_used = 0; user_labels.clear(); loops.clear();
        for (DeclInfo *p : n->decl->params) reserve(p, true);
        for (Node *k : n->kids) collect_locals(k);
        int frame = (frame_used + 8 + 7) / 8 * 8;
        current_frame = frame;
        epilogue = fresh("return"); label(function_label(n->decl->name)); prologue(frame);
        for (size_t i = 0; i < n->decl->params.size(); i++) {
            DeclInfo *p = n->decl->params[i]; if (p->name.empty()) continue;
            Type t = *p->type;
            if (!t.dims.empty()) { t.dims.clear(); t.ptr_depth++; }
            address_offset("$v0", "$fp", frame + (aggregate(*n->type) ? 8 : 0) + (int)i * 8);
            if (aggregate(t)) {
                emit("lw $a1, 0($v0)"); address_offset("$a0", "$fp", storage[p].offset);
                emit("li $a2, " + number(size(t))); call("__rt_copy");
            } else {
                load(t); address_offset("$t0", "$fp", storage[p].offset); store(t);
            }
        }
        for (Node *k : n->kids) statement(k);
        emit("move $v0, $zero"); emit("mtc1 $zero, $f0"); emit("mtc1 $zero, $f1");
        finish(frame); text << '\n';
    }

    bool constant_initializer(Node *n) {
        if (!n) return true;
        if (n->kind == N_INT || n->kind == N_CHAR || n->kind == N_FLOAT || n->kind == N_STR || n->kind == N_SIZEOF) return true;
        if (n->kind == N_UN && n->op == "&") return n->kids[0]->kind == N_ID && !storage[n->kids[0]->decl].label.empty();
        if (n->kind != N_BIN && n->kind != N_UN && n->kind != N_CAST && n->kind != N_TERNARY && n->kind != N_INITLIST) return false;
        if (n->kind == N_UN && (n->op == "++" || n->op == "--" || n->op == "*")) return false;
        for (Node *k : n->kids) if (!constant_initializer(k)) return false;
        return true;
    }

public:
    
    std::string run(Node *root) {
        if (!root) throw std::runtime_error("empty program");
        for (Node *n : root->kids) {
            if (n->kind == N_FUNC) {
                if (functions.count(n->decl->name)) fail(n, "function defined twice");
                functions[n->decl->name] = n;
            } else if (n->kind == N_DECL) {
                for (DeclInfo *d : n->decls) {
                    if (d->is_typedef || d->is_function) continue;
                    std::string name = fresh("global"); storage[d] = {name, 0, false};
                    data << "    .align " << (semantic_type_alignment(*d->type) == 8 ? 3 : 2) << '\n'
                         << name << ": .space " << size(*d->type) << '\n';
                }
            }
        }
        if (!functions.count("main")) throw std::runtime_error("target program requires a main function");
        Node *main = functions["main"];
        if (aggregate(*main->type) || floating(*main->type) || main->type->ptr_depth || main->type->base == "VOID")
            fail(main, "main must return an integer");
        if (!main->decl->params.empty() && !(main->decl->params.size() == 1 && main->decl->params[0]->name.empty() && main->decl->params[0]->spec->base == "VOID"))
            fail(main, "main takes no arguments in SPIM");
        label("main"); emit("li $t0, -8"); emit("and $sp, $sp, $t0");
        call("__cg_globals"); call(function_label("main"));
        emit("move $a0, $v0"); emit("li $v0, 17"); emit("syscall");
        label("__cg_globals"); prologue(8); epilogue = fresh("globals_return");
        for (Node *n : root->kids) if (n->kind == N_DECL)
            for (DeclInfo *d : n->decls) if (!d->is_typedef && !d->is_function && d->init) {
                if (!constant_initializer(d->init)) fail(d->init, "global initializer must be constant");
                declaration(d);
            }
        finish(8);
        for (Node *n : root->kids) if (n->kind == N_FUNC) function(n);
        return "# MIPS32 assembly for SPIM 8.0; generated by targetgen\n.data\n" + data.str() +
            target_runtime_data() + "\n.text\n.globl main\n" + text.str() + target_runtime_text();
    }
};
}

std::string generate_target(Node *root) { return Target().run(root); }
