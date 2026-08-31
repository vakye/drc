
enum ir_opcode {
    IR_NOP = 0,
    IR_IMM64,
    IR_ADD,
    IR_SUB,
    IR_IMUL,
    IR_IDIV,
    IR_IMOD,
    IR_RET,

    IR_OPCODE_COUNT,
};

typedef u32 ir_inst_id;

struct ir_inst {
    enum ir_opcode opcode;
    union
    {
        struct { usize imm; };
        struct { ir_inst_id left, right; };
    };
};

struct ir_inst_block {
    struct ir_inst* insts;
    u32             inst_count;
};

static b32              ir_generate_node    (struct arena* allocator, struct node* node, struct ir_inst_block* result);
static struct ir_inst*  ir_push_inst        (struct arena* allocator, struct ir_inst_block* result);

static void     print_ir_inst_id    (ir_inst_id id, usize padding);
static void     print_ir_inst       (struct ir_inst* inst);
static void     print_ir_inst_block (struct ir_inst_block* block);

static b32 ir_generate(
    struct arena*           allocator,
    struct parsed_context*  parsed,
    struct ir_inst_block*   result)
{
    zero_type(result);
    result->insts = arena_alloc_at(allocator);

    ir_generate_node(allocator, parsed->root_node, result);

    struct ir_inst* inst = ir_push_inst(allocator, result);
    inst->opcode = IR_RET;

    return (true);
}

static ir_inst_id ir_generate_node(struct arena* allocator, struct node* node, struct ir_inst_block* result)
{
    ensure(node != 0, str("passing null node to ir_generate_node()"));

    ir_inst_id inst_id = 0;

    switch (node->kind)
    {
        default: {} break;

        case NODE_KIND_INTEGER:
        {
            inst_id = result->inst_count;
            struct ir_inst* inst = ir_push_inst(allocator, result);
            inst->opcode = IR_IMM64;
            inst->imm = node->integer;
        } break;

        case NODE_KIND_ADD:
        case NODE_KIND_SUB:
        case NODE_KIND_MUL:
        case NODE_KIND_DIV:
        case NODE_KIND_MOD:
        {
            ir_inst_id left = ir_generate_node(allocator, node->left, result);
            ir_inst_id right = ir_generate_node(allocator, node->right, result);

            inst_id = result->inst_count;
            struct ir_inst* inst = ir_push_inst(allocator, result);
            inst->left = left;
            inst->right = right;

            switch (node->kind)
            {
                case NODE_KIND_ADD: inst->opcode = IR_ADD; break;
                case NODE_KIND_SUB: inst->opcode = IR_SUB; break;
                case NODE_KIND_MUL: inst->opcode = IR_IMUL; break;
                case NODE_KIND_DIV: inst->opcode = IR_IDIV; break;
                case NODE_KIND_MOD: inst->opcode = IR_IMOD; break;
            }
        } break;
    }

    return (inst_id);
}

static struct ir_inst* ir_push_inst(struct arena* allocator, struct ir_inst_block* result)
{
    struct ir_inst* inst = arena_push_type(allocator, struct ir_inst);
    zero_type(inst);
    result->inst_count++;

    return (inst);
}

static void print_ir_inst_id(ir_inst_id id, usize padding)
{
    usize inst_id_written = 0;
    inst_id_written += print_char('%');
    inst_id_written += print_usize(id);
    right_pad(inst_id_written, padding);
}

static void print_ir_inst(struct ir_inst* inst)
{
    static struct string ir_opcode_names[IR_OPCODE_COUNT] =
    {
        #define ir_do_opcode_name(opcode) \
            [IR_##opcode] = {#opcode, sizeof(#opcode) - 1}

        ir_do_opcode_name(NOP),
        ir_do_opcode_name(IMM64),
        ir_do_opcode_name(ADD),
        ir_do_opcode_name(SUB),
        ir_do_opcode_name(IMUL),
        ir_do_opcode_name(IDIV),
        ir_do_opcode_name(IMOD),
        ir_do_opcode_name(RET),

        #undef ir_do_opcode_name
    };

    right_pad(print(ir_opcode_names[inst->opcode]), 16);

    switch (inst->opcode)
    {
        default: break;

        case IR_IMM64: print_usize(inst->imm); break;

        case IR_ADD:
        case IR_SUB:
        case IR_IMUL:
        case IR_IDIV:
        case IR_IMOD:
        {
            print_ir_inst_id(inst->left, 12);
            print_ir_inst_id(inst->right, 12);
        } break;
    }

    print_newline();
}

static void print_ir_inst_block(struct ir_inst_block* block)
{
    for (ir_inst_id index = 0; index < block->inst_count; index++)
    {
        print(str("    "));
        print_ir_inst_id(index, 12);
        print_ir_inst(block->insts + index);
    }
}

