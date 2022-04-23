#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "bloop.h"

struct node {
    int ID;
    char name[32];
    struct nk_rect bounds;
    float value;
    struct nk_color color;
    int input_count;
    int output_count;
    struct node *next;
    struct node *prev;
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    struct nk_vec2 in;
    struct nk_vec2 out;
};

struct node_linking {
    int active;
    struct node *node;
    int input_id;
    int input_slot;
};

struct node_editor {
    int initialized;
    struct node node_buf[64];
    struct node_link links[128];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    struct nk_rect bounds;
    struct node *selected;
    int show_grid;
    struct nk_vec2 scrolling;
    struct node_linking linking;
};
static struct node_editor nodeEditor;

typedef struct add_node_result {
    int id;
    int children_left;
    int children_right;
} add_node_result;

add_node_result *bloop_generator_to_nodes_and_link(struct node_editor *editor, bloop_generator *g, int output_id, int output_slot, int x);
add_node_result *bloop_generator_to_nodes(struct node_editor *editor, bloop_generator *g, int x);
int node_editor(struct nk_context *ctx);

extern bloop_generator *generator;
