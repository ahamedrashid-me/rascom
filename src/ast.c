#include "../include/ast.h"

ASTNode *ast_node_new(ASTNodeType type) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    return node;
}

ASTList *ast_list_new(void) {
    ASTList *list = xmalloc(sizeof(ASTList));
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

void ast_list_add(ASTList *list, ASTNode *node) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->nodes = xrealloc(list->nodes, sizeof(ASTNode*) * list->capacity);
    }
    list->nodes[list->count++] = node;
}

void ast_node_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
    case AST_PROGRAM:
        ast_list_free(node->program.imports);
        ast_list_free(node->program.constants);
        ast_list_free(node->program.groups);
        ast_list_free(node->program.functions);
        break;

    case AST_CONST_DECL:
        free(node->const_decl.name);
        ast_node_free(node->const_decl.value);
        break;

    case AST_FUNCTION:
        free(node->function.name);
        free(node->function.return_type);
        ast_list_free(node->function.params);
        ast_node_free(node->function.body);
        break;

    case AST_BLOCK:
        ast_list_free(node->block.statements);
        break;

    case AST_RETURN:
        ast_node_free(node->ret.value);
        break;

    case AST_IF:
        ast_node_free(node->if_stmt.condition);
        ast_node_free(node->if_stmt.then_block);
        ast_node_free(node->if_stmt.else_block);
        break;

    case AST_WHILE:
        ast_node_free(node->while_stmt.condition);
        ast_node_free(node->while_stmt.body);
        break;

    case AST_LOOP:
        ast_node_free(node->loop_stmt.init);
        ast_node_free(node->loop_stmt.condition);
        ast_node_free(node->loop_stmt.increment);
        ast_node_free(node->loop_stmt.body);
        break;

    case AST_SHOW:
        ast_node_free(node->show.value);
        break;

    case AST_READ:
        ast_node_free(node->read.target);
        ast_node_free(node->read.prompt);
        break;

    case AST_CYCLE:
        ast_node_free(node->cycle.value);
        ast_list_free(node->cycle.cases);
        ast_node_free(node->cycle.default_case);
        break;

    case AST_WHEN:
        ast_node_free(node->when.value);
        ast_node_free(node->when.body);
        break;

    case AST_CHECK:
        ast_node_free(node->check.try_block);
        ast_list_free(node->check.handlers);
        break;

    case AST_VAR_DECL:
        free(node->var_decl.type);
        free(node->var_decl.name);
        free(node->var_decl.array_element_type);
        ast_node_free(node->var_decl.value);
        ast_node_free(node->var_decl.array_size);
        break;

    case AST_ASSIGN:
        free(node->assign.name);
        ast_node_free(node->assign.value);
        break;

    case AST_ARRAY_ASSIGN:
        free(node->array_assign.name);
        ast_node_free(node->array_assign.index);
        ast_node_free(node->array_assign.value);
        break;

    case AST_BINARY_OP:
        free(node->binary.op);
        ast_node_free(node->binary.left);
        ast_node_free(node->binary.right);
        break;

    case AST_UNARY_OP:
        free(node->unary.op);
        ast_node_free(node->unary.operand);
        break;

    case AST_TERNARY_OP:
        ast_node_free(node->ternary.condition);
        ast_node_free(node->ternary.true_expr);
        ast_node_free(node->ternary.false_expr);
        break;

    case AST_CALL:
        free(node->call.name);
        ast_list_free(node->call.args);
        break;

    case AST_LITERAL:
        free(node->literal.value);
        free(node->literal.type);
        break;

    case AST_IDENT:
        free(node->ident.name);
        break;

    case AST_ARRAY_DECL:
        free(node->array_decl.element_type);
        free(node->array_decl.name);
        ast_node_free(node->array_decl.size);
        ast_list_free(node->array_decl.initializer);
        break;

    case AST_ARRAY_ACCESS:
        free(node->array_access.name);
        ast_node_free(node->array_access.index);
        break;

    case AST_GROUP_DEF:
        free(node->group_def.name);
        ast_list_free(node->group_def.fields);
        break;

    case AST_GROUP_DECL:
        free(node->group_decl.type_name);
        free(node->group_decl.var_name);
        break;

    case AST_MEMBER_ACCESS:
        ast_node_free(node->member_access.object);
        free(node->member_access.member_name);
        break;

    case AST_MEMBER_ASSIGN:
        ast_node_free(node->member_assign.object);
        free(node->member_assign.member_name);
        ast_node_free(node->member_assign.value);
        break;

    case AST_MAP_DECL:
        free(node->map_decl.key_type);
        free(node->map_decl.value_type);
        free(node->map_decl.name);
        break;

    case AST_MAP_SET:
        free(node->map_set.map_name);
        ast_node_free(node->map_set.key);
        ast_node_free(node->map_set.value);
        break;

    case AST_MAP_GET:
        free(node->map_get.map_name);
        ast_node_free(node->map_get.key);
        break;

    case AST_MAP_HAS:
        free(node->map_has.map_name);
        ast_node_free(node->map_has.key);
        break;

    case AST_MAP_REMOVE:
        free(node->map_remove.map_name);
        ast_node_free(node->map_remove.key);
        break;

    case AST_BUILTIN_CALL:
        free(node->builtin_call.name);
        ast_list_free(node->builtin_call.args);
        break;

    case AST_PACKAGE_IMPORT:
        free(node->package_import.package_name);
        free(node->package_import.alias);
        break;

    case AST_BREAK:
    case AST_CONTINUE:
        // No additional cleanup needed
        break;

    case AST_DEFER:
        ast_node_free(node->defer.stmt);
        break;

    case AST_EMIT:
        ast_node_free(node->emit.value);
        break;

    case AST_MANME:
        ast_node_free(node->manme.body);
        break;

    case AST_SHOWF:
        ast_node_free(node->showf.interpolated);
        break;

    case AST_INTERPOLATED_STRING:
        ast_list_free(node->interpolated_string.chunks);
        ast_list_free(node->interpolated_string.expressions);
        break;
    }

    free(node);
}

void ast_list_free(ASTList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        ast_node_free(list->nodes[i]);
    }
    free(list->nodes);
    free(list);
}