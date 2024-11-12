#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#define WIDTH 800
#define HEIGHT WIDTH

static Arena node_arena = {0};

typedef enum {
    NK_X,
    NK_Y,
    NK_RANDOM,

    NK_RULE,
    NK_NUMBER,
    NK_BOOLEAN,

    NK_ADD,
    NK_MULT,
    NK_MOD,
    NK_GT,
    NK_LT,
    NK_GTEQ,
    NK_LTEQ,

    NK_TRIPLE,
    NK_IF,

    COUNT_NK,
} Node_Kind;

static_assert(COUNT_NK == 15, "number of nodes have changed.");
const char *nk_names[COUNT_NK] = {
    [NK_X] = "x",
    [NK_Y] = "y",
    [NK_NUMBER] = "number",
    [NK_ADD] = "add",
    [NK_MULT] = "mult",
    [NK_MOD] = "mod",
    [NK_BOOLEAN] = "boolean",
    [NK_GT] = "gt",
    [NK_LT] = "lt",
    [NK_GTEQ] = "gteq",
    [NK_LTEQ] = "lteq",
    [NK_TRIPLE] = "triple",
    [NK_IF] = "if",
    [NK_RULE] = "rule",
    [NK_RANDOM] = "random",
};

typedef struct Node Node;

typedef struct {
    Node *lhs;
    Node *rhs;
} Node_Binop;

typedef struct {
    Node *first;
    Node *second;
    Node *third;
} Node_Triple;

typedef struct {
    Node *cond;
    Node *then;
    Node *elze;
} Node_If;

typedef union {
    float number;
    bool boolean;
    Node_Binop binop;
    Node_Triple triple;
    Node_If iff;
    int rule;
} Node_As;

typedef struct {
    Node *node;
    float probability;
} Grammar_Branch;

typedef struct {
    Grammar_Branch *items;
    size_t capacity;
    size_t count;
} Grammar_Branches;

typedef struct {
    Grammar_Branches *items;
    size_t capacity;
    size_t count;
} Grammar;

struct Node {
    Node_Kind kind;
    const char *file_path;
    int line;
    Node_As as;
};

Node *node_loc(const char *file_path, int line, Node_Kind kind) {
    Node *node = arena_alloc(&node_arena, sizeof(Node));
    node->kind = kind;
    node->file_path = file_path;
    node->line = line;
    return node;
}

Node *node_binop_loc(const char *file_path, int line, Node_Kind kind, Node *lhs, Node *rhs) {
    Node *node = node_loc(file_path, line, kind);
    node->as.binop.lhs = lhs;
    node->as.binop.rhs = rhs;
    return node;
}

Node *node_number_loc(const char *file_path, int line, float number) {
    Node *node = node_loc(file_path, line, NK_NUMBER);
    node->as.number = number;
    return node;
}

#define node_number(number) node_number_loc(__FILE__, __LINE__, number)

#define node_x() node_loc(__FILE__, __LINE__, NK_X)
#define node_y() node_loc(__FILE__, __LINE__, NK_Y)
#define node_random() node_loc(__FILE__, __LINE__, NK_RANDOM)

#define node_add(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_ADD, lhs, rhs)
#define node_mult(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_MULT, lhs, rhs)
#define node_mod(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_MOD, lhs, rhs)
#define node_gt(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_GT, lhs, rhs)
#define node_lt(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_LT, lhs, rhs)
#define node_gteq(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_GTEQ, lhs, rhs)
#define node_lteq(lhs, rhs) node_binop_loc(__FILE__, __LINE__, NK_LTEQ, lhs, rhs)

Node *node_boolean_loc(const char *file_path, int line, bool boolean) {
    Node *node = node_loc(file_path, line, NK_BOOLEAN);
    node->as.boolean = boolean;
    return node;
}

#define node_boolean(boolean) node_boolean_loc(__FILE__, __LINE__, boolean)

Node *node_triple_loc(const char *file_path, int line, Node *first, Node *second, Node *third) {
    Node *node = node_loc(file_path, line, NK_TRIPLE);
    node->as.triple.first = first;
    node->as.triple.second = second;
    node->as.triple.third = third;
    return node;
}

Node *node_rule_loc(const char *file_path, int line, int rule) {
    Node *node = node_loc(file_path, line, NK_RULE);
    node->as.rule = rule;
    return node;
}

#define node_rule(rule) node_rule_loc(__FILE__, __LINE__, rule)

#define node_triple(first, second, third) node_triple_loc(__FILE__, __LINE__, first, second, third)

Node *node_if_loc(const char *file_path, int line, Node *cond, Node *then, Node *elze) {
    Node *node = node_loc(file_path, line, NK_IF);
    node->as.iff.cond = cond;
    node->as.iff.then = then;
    node->as.iff.elze = elze;
    return node;
}

#define node_if(cond, then, elze) node_if_loc(__FILE__, __LINE__, cond, then, elze)

void node_print(Node *node) {
    switch (node->kind) {
        case NK_X:
            printf("x");
            break;
        case NK_Y:
            printf("y");
            break;
        case NK_NUMBER:
            printf("%f", node->as.number);
            break;
        case NK_ADD:
            printf("add(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_MULT:
            printf("mult(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_MOD:
            printf("mod(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_BOOLEAN:
            printf("%s", node->as.boolean ? "true" : "false");
            break;
        case NK_GT:
            printf("gt(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_LT:
            printf("lt(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_GTEQ:
            printf("gteq(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_LTEQ:
            printf("lteq(");
            node_print(node->as.binop.lhs);
            printf(",");
            node_print(node->as.binop.rhs);
            printf(")");
            break;
        case NK_TRIPLE:
            printf("(");
            node_print(node->as.triple.first);
            printf(",");
            node_print(node->as.triple.second);
            printf(",");
            node_print(node->as.triple.third);
            printf(")");
            break;
        case NK_IF:
            printf("if ");
            node_print(node->as.iff.cond);
            printf(" then ");
            node_print(node->as.iff.then);
            printf(" else ");
            node_print(node->as.iff.elze);
            break;
        case NK_RULE:
            printf("rule(%d)", node->as.rule);
            break;
        case NK_RANDOM:
            printf("random");
            break;
        case COUNT_NK:
        default: UNREACHABLE("node_print");
    }
}

#define node_print_ln(node) (node_print(node), printf("\n"))

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA32;

static RGBA32 pixels[WIDTH*HEIGHT];

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float r, g, b;
} Color;

bool expect_number(Node *expr) {
    if (expr->kind != NK_NUMBER) {
        nob_log(ERROR, "%s:%d: expected a number.", expr->file_path, expr->line);
        return false;
    }
    return true;
}

bool expect_triple(Node *expr) {
    if (expr->kind != NK_TRIPLE) {
        nob_log(ERROR, "%s:%d: expected a triple.", expr->file_path, expr->line);
        return false;
    }
    return true;
}

bool expect_boolean(Node *expr) {
    if (expr->kind != NK_BOOLEAN) {
        nob_log(ERROR, "%s:%d: expected a boolean.", expr->file_path, expr->line);
        return false;
    }
    return true;
}

Node *eval(Node *expr, float x, float y) {
    switch (expr->kind) {
        case NK_X:      return node_number_loc(expr->file_path, expr->line, x);
        case NK_Y:      return node_number_loc(expr->file_path, expr->line, y);
        case NK_BOOLEAN:
        case NK_NUMBER: return expr;
        case NK_RANDOM:
        case NK_RULE: {
            nob_log(ERROR, "%s:%d: cannot evaluate a grammar-only node.", expr->file_path, expr->line);
            return NULL;
        }
        case NK_ADD: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_number_loc(expr->file_path, expr->line, lhs->as.number + rhs->as.number);
        }
        case NK_MULT: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_number_loc(expr->file_path, expr->line, lhs->as.number * rhs->as.number);
        }
        case NK_MOD: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_number_loc(expr->file_path, expr->line, fmodf(lhs->as.number, rhs->as.number));
        }
        case NK_GT: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_boolean_loc(expr->file_path, expr->line, lhs->as.number > rhs->as.number);
        }
        case NK_LT: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_boolean_loc(expr->file_path, expr->line, lhs->as.number < rhs->as.number);
        }
        case NK_GTEQ: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_boolean_loc(expr->file_path, expr->line, lhs->as.number >= rhs->as.number);
        }
        case NK_LTEQ: {
            Node *lhs = eval(expr->as.binop.lhs, x, y);
            if (!lhs) return NULL;
            if (!expect_number(lhs)) return NULL;
            Node *rhs = eval(expr->as.binop.rhs, x, y);
            if (!rhs) return NULL;
            if (!expect_number(rhs)) return NULL;
            return node_boolean_loc(expr->file_path, expr->line, lhs->as.number <= rhs->as.number);
        }
        case NK_TRIPLE: {
            Node *first = eval(expr->as.triple.first, x, y);
            if (!first) return NULL;
            Node *second = eval(expr->as.triple.second, x, y);
            if (!second) return NULL;
            Node *third = eval(expr->as.triple.third, x, y);
            if (!third) return NULL;
            return node_triple_loc(expr->file_path, expr->line, first, second, third);
        }
        case NK_IF: {
            Node *cond = eval(expr->as.iff.cond, x, y);
            if (!cond) return NULL;
            if (!expect_boolean(cond)) return NULL;
            Node *then = eval(expr->as.iff.then, x, y);
            if (!then) return NULL;
            // if (!expect_boolean(then)) return NULL;
            Node *elze = eval(expr->as.iff.elze, x, y);
            if (!elze) return NULL;
            // if (!expect_boolean(elze)) return NULL;
            return cond->as.boolean ? then : elze;
        }
        case COUNT_NK:
        default:
            UNREACHABLE("eval()");
    }
}

bool eval_func(Node *f, float x, float y, Color *c) {
    Node *res = eval(f, x, y);
    if (res == NULL) return false;
    if (!expect_triple(res)) return false;
    if (!expect_number(res->as.triple.first)) return false;
    if (!expect_number(res->as.triple.second)) return false;
    if (!expect_number(res->as.triple.third)) return false;

    c->r = res->as.triple.first->as.number;
    c->g = res->as.triple.second->as.number;
    c->b = res->as.triple.third->as.number;
    return true;
}

bool render_pixels(Node *f) {
    for (size_t y = 0; y < HEIGHT; ++y) {
        float ny = (float)y / HEIGHT * 2.0f - 1;
        for (size_t x = 0; x < WIDTH; ++x) {
            float nx = (float)x / WIDTH * 2.0f - 1;
            Color c;
            if (!eval_func(f, nx, ny, &c)) return false;
            size_t index = y * WIDTH + x;
            pixels[index].r = (c.r + 1) / 2 * 255;
            pixels[index].g = (c.g + 1) / 2 * 255;
            pixels[index].b = (c.b + 1) / 2 * 255;
            pixels[index].a = 255;

        }
    }
    return true;
}

void grammar_print(Grammar grammar) {
    for (size_t i = 0; i < grammar.count; ++i) {
        printf("%zu ::= ", i);
        Grammar_Branches *branches = &grammar.items[i];
        for (size_t j = 0; j < branches->count; ++j) {
            if (j > 0) printf(" | ");
            node_print(branches->items[j].node);
            printf(" [%.02f]", branches->items[j].probability);
        }
        printf("\n");
    }
}

float rand_float(void) {
    return (float)rand()/RAND_MAX;
}

Node *gen_rule(Grammar grammar, size_t rule, int depth);

Node *gen_node(Grammar grammar, Node *node, int depth) {
    switch (node->kind) {
        case NK_X:
        case NK_Y:
        case NK_NUMBER:
        case NK_BOOLEAN:
            return node;

        case NK_ADD:
        case NK_MULT:
        case NK_MOD:
        case NK_GT:
        case NK_LT:
        case NK_GTEQ:
        case NK_LTEQ: {
            Node *lhs = gen_node(grammar, node->as.binop.lhs, depth);
            if (!lhs) return NULL;
            Node *rhs = gen_node(grammar, node->as.binop.rhs, depth);
            if (!rhs) return NULL;
            return node_binop_loc(node->file_path, node->line, node->kind, lhs, rhs);
        }

        case NK_TRIPLE: {
            Node *first = gen_node(grammar, node->as.triple.first, depth);
            if (!first) return NULL;
            Node *second = gen_node(grammar, node->as.triple.second, depth);
            if (!second) return NULL;
            Node *third = gen_node(grammar, node->as.triple.third, depth);
            if (!third) return NULL;
            return node_triple_loc(node->file_path, node->line, first, second, third);
        }
        case NK_IF: {
            Node *cond = gen_node(grammar, node->as.iff.cond, depth);
            if (!cond) return NULL;
            Node *then = gen_node(grammar, node->as.iff.then, depth);
            if (!then) return NULL;
            Node *elze = gen_node(grammar, node->as.iff.elze, depth);
            if (!elze) return NULL;
            return node_if_loc(node->file_path, node->line, cond, then, elze);
            break;
        }

        case NK_RULE: {
            return gen_rule(grammar, node->as.rule, depth - 1);
        }
        case NK_RANDOM: {
            return node_number_loc(node->file_path, node->line, rand_float() * 2.0f - 1.0f);
        }

        case COUNT_NK:
        default:
            UNREACHABLE("gen_node()");
    }
}

#define GEN_RULE_MAX_ATTEMPTS 10

Node *gen_rule(Grammar grammar, size_t rule, int depth) {
    if (depth <= 0) return NULL;

    assert(rule < grammar.count);

    Grammar_Branches *branches = &grammar.items[rule];
    assert(branches->count > 0);

    Node *node = NULL;
    for (size_t attempts = 0; node == NULL && attempts < GEN_RULE_MAX_ATTEMPTS; ++attempts) {
        float p = rand_float();
        float t = 0.0f;
        for (size_t i = 0; i < branches->count; ++i) {
            t += branches->items[i].probability;
            if (t >= p) {
                node = gen_node(grammar, branches->items[i].node, depth - 1);
                break;
            }
        }
    };
    return node;
}


int main(void) {
    srand(time(0));

    Grammar grammar = {0};
    Grammar_Branches branches = {0};
    int e = 0;
    int a = 1;
    int c = 2;

    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_triple(node_rule(c), node_rule(c), node_rule(c)),
        .probability = 1.0f,
    }));
    arena_da_append(&node_arena, &grammar, branches);
    memset(&branches, 0, sizeof(branches));

    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_random(),
        .probability = 1.0f/3.0f,
    }));
    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_x(),
        .probability = 1.0f/3.0f,
    }));
    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_y(),
        .probability = 1.0f/3.0f,
    }));
    arena_da_append(&node_arena, &grammar, branches);
    memset(&branches, 0, sizeof(branches));

    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_rule(a),
        .probability = 1.0f/4.0f,
        // .probability = 1.0f/2.0f,
    }));
    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_add(node_rule(c), node_rule(c)),
        .probability = 3.0f/8.0f,
        // .probability = 1.0f/4.0f,
    }));
    arena_da_append(&node_arena, &branches, ((Grammar_Branch) {
        .node = node_mult(node_rule(c), node_rule(c)),
        .probability = 3.0f/8.0f,
        // .probability = 1.0f/4.0f,
    }));
    arena_da_append(&node_arena, &grammar, branches);
    memset(&branches, 0, sizeof(branches));

    Node *f = gen_rule(grammar, e, 20);
    if (!f) {
        nob_log(ERROR, "the generation process could not terminate.");
        return 1;
    }
    node_print_ln(f);

    // bool ok = render_pixels(node_triple( node_x(), node_y(), node_y()));
    // bool ok = render_pixels(
    //     node_if(
    //         node_gteq(node_mult(node_x(), node_y()), node_number(0)),
    //         node_triple(
    //             node_x(),
    //             node_y(),
    //             node_number(1)),
    //         node_triple(
    //             node_mod(node_x(), node_y()),
    //             node_mod(node_x(), node_y()),
    //             node_mod(node_x(), node_y()))));

    bool ok = render_pixels(f);
    if (!ok) return 1;

    const char *output_path = "output.png";
    if (!stbi_write_png(output_path, WIDTH, HEIGHT, 4, pixels, WIDTH * sizeof(RGBA32))) {
        nob_log(ERROR, "could not save image: %s", output_path);
        return 1;
    };
    nob_log(INFO, "generated: %s", output_path);
    return 0;
}
