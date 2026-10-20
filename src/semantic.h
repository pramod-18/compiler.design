#pragma once

#include "ast.h"

int semantic_check(Node *root, const char *filename);

int semantic_type_size(const Type &t);
