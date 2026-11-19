#pragma once

#include "ast.h"

int semantic_check(Node *root, const char *filename);

int semantic_type_size(const Type &t);
int semantic_type_alignment(const Type &t);
int semantic_member_offset(const Type &t, const std::string &name);
std::vector<std::pair<std::string, Type>> semantic_struct_members(const Type &t);
std::string decode_string_literal(const std::string &text);
