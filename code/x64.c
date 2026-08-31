
enum x64_reg64 {
    x64_RAX = 0,
    x64_RCX = 1,
    x64_RDX = 2,
    x64_RBX = 3,
    x64_RSP = 4,
    x64_RBP = 5,
    x64_RSI = 6,
    x64_RDI = 7,
    x64_R8  = 8,
    x64_R9  = 9,
    x64_R10 = 10,
    x64_R11 = 11,
    x64_R12 = 12,
    x64_R13 = 13,
    x64_R14 = 14,
    x64_R15 = 15,
};

// NOTE(vak): Assumes SystemV ABI: rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#define x64_VOLATILE_REGISTER_COUNT (9)

typedef u32 x64_reg_desc_id;
#define x64_INVALID_REG_DESC_ID (U32_MAX)

struct x64_reg_desc {
    ir_inst_id      holding_inst_id;        // NOTE(vak): Value being held in register is the result of this instruction
    b32             released;               // NOTE(vak): Register may hold some value, but is not needed anymore
};

struct x64_context {
    struct arena*       allocator;
    u8*                 base;
    usize               size;
    struct x64_reg_desc reg_descs[x64_VOLATILE_REGISTER_COUNT];
};

static void x64_emit_bytes(struct x64_context* ctx, void* bytes, usize size);
static void x64_emit8(struct x64_context* ctx, u8 value);
static void x64_emit16(struct x64_context* ctx, u16 value);
static void x64_emit24(struct x64_context* ctx, u32 value);
static void x64_emit32(struct x64_context* ctx, u32 value);
static void x64_emit40(struct x64_context* ctx, u64 value);
static void x64_emit48(struct x64_context* ctx, u64 value);
static void x64_emit56(struct x64_context* ctx, u64 value);
static void x64_emit64(struct x64_context* ctx, u64 value);

static enum x64_reg64 x64_get_reg_encoding(x64_reg_desc_id id)
{
    static enum x64_reg64 table[x64_VOLATILE_REGISTER_COUNT] =
    {
        x64_RAX,
        x64_RCX,
        x64_RDX,
        x64_RSI,
        x64_RDI,
        x64_R8,
        x64_R9,
        x64_R10,
        x64_R11,
    };

    enum x64_reg64 result = table[id];
    return (result);
}

static x64_reg_desc_id x64_find_reg_holding_inst(struct x64_context* ctx, struct ir_inst_block* block, ir_inst_id inst_id)
{
    x64_reg_desc_id reg_desc_id = x64_INVALID_REG_DESC_ID;

    struct ir_inst* inst = block->insts + inst_id;
    b32 is_imm = (inst->opcode == IR_IMM64);

    for (u32 index = 0; index < x64_VOLATILE_REGISTER_COUNT; index++)
    {
        struct x64_reg_desc* desc = ctx->reg_descs + index;

        if (desc->holding_inst_id == IR_INVALID_INST_ID)
            continue;

        if (desc->holding_inst_id == inst_id)
        {
            reg_desc_id = index;
            break;
        }

        if (is_imm) // NOTE(vak): Another register may be loaded with the same immediate
        {
            struct ir_inst* other_inst = block->insts + desc->holding_inst_id;
            if (other_inst->opcode != IR_IMM64) continue;

            if (other_inst->imm == inst->imm)
            {
                reg_desc_id = index;
                break;
            }
        }
    }

    if (reg_desc_id != x64_INVALID_REG_DESC_ID)
    {
        struct x64_reg_desc* desc = ctx->reg_descs + reg_desc_id;
        desc->released = false; // NOTE(vak): User is looking for this register, so it is now in use.
    }

    return (reg_desc_id);
}

static x64_reg_desc_id x64_acquire_free_reg_for_inst(struct x64_context* ctx, ir_inst_id inst_id)
{
    x64_reg_desc_id reg_desc_id = x64_INVALID_REG_DESC_ID;

    for (u32 index = 0; index < x64_VOLATILE_REGISTER_COUNT; index++)
    {
        struct x64_reg_desc* desc = ctx->reg_descs + index;
        if ((desc->holding_inst_id == x64_INVALID_REG_DESC_ID) || (desc->released))
        {
            reg_desc_id = index;
            desc->holding_inst_id = inst_id;
            desc->released = false;
            break;
        }
    }

    ensure(reg_desc_id != x64_INVALID_REG_DESC_ID, str("ran out of registers. stack clobbering is not implemented yet."));
    return (reg_desc_id);
}

static void x64_change_reg_owner(struct x64_context* ctx, x64_reg_desc_id reg_desc_id, ir_inst_id inst_id)
{
    struct x64_reg_desc* desc = ctx->reg_descs + reg_desc_id;
    desc->holding_inst_id = inst_id;
}

static void x64_release_reg(struct x64_context* ctx, x64_reg_desc_id reg_desc_id)
{
    struct x64_reg_desc* desc = ctx->reg_descs + reg_desc_id;

    // NOTE(vak): We maintain desc->holding_inst_id because it can hold some old
    // value that might be needed in the future. This can prevent redundant loads
    // in certain situations (like "10 + 10 + 10 + 10 + 10").

    desc->released = true;
}

static void x64_load_r64_imm64(struct x64_context* ctx, x64_reg_desc_id reg_desc_id, u64 imm64)
{
    enum x64_reg64 reg_encoding = x64_get_reg_encoding(reg_desc_id);

    // [rex (b8 + reg) imm64]: mov reg, imm64

    u16 instruction = 0xb848;
    instruction += (reg_encoding >> 3);         // REX.B for extended regs (r8 and above)
    instruction += (reg_encoding & 0x7) << 8;   // 0xb8 + reg

    x64_emit16(ctx, instruction);
    x64_emit64(ctx, imm64);
}

static void x64_move_r64_r64(struct x64_context* ctx, x64_reg_desc_id dst, x64_reg_desc_id src)
{
    if (dst == src)
        return;

    enum x64_reg64 dst_encoding = x64_get_reg_encoding(dst);
    enum x64_reg64 src_encoding = x64_get_reg_encoding(src);

    // [rex 8b modrm] mov dst, src

    u32 instruction = 0xc08b48;

    u32 rex_r       = (dst_encoding & 0x8) >> 1;    // REX.R for extended regs on dest operand (r8 and above)
    u32 rex_b       = (src_encoding & 0x8) >> 3;    // REX.B for extended regs on source operand (r8 and above)
    u32 modrm_reg   = (dst_encoding & 0x7) << 19;   // ModRM.reg (dest operand)
    u32 modrm_rm    = (src_encoding & 0x7) << 16;   // ModRM.rm  (source operand)

    instruction += (rex_r + rex_b) + (modrm_reg + modrm_rm);   

    x64_emit24(ctx, instruction);
}

static void x64_load_inst_into_r64(struct x64_context* ctx, x64_reg_desc_id dst, struct ir_inst* src_inst)
{
    switch (src_inst->opcode)
    {
        default: panic(str("unknown opcode not handled in x64_load_inst_to_r64()")); break;

        case IR_IMM64: x64_load_r64_imm64(ctx, dst, src_inst->imm); break;
    }
}

static void x64_generate(struct arena* allocator, struct ir_inst_block* block, struct x64_context* result)
{
    zero_type(result);
    result->allocator = allocator;
    result->base = arena_alloc_at(allocator);

    for (u32 index = 0; index < x64_VOLATILE_REGISTER_COUNT; index++)
    {
        struct x64_reg_desc* desc = result->reg_descs + index;
        desc->holding_inst_id = IR_INVALID_INST_ID;
    }

    for (ir_inst_id id = 0; id < block->inst_count; id++)
    {
        struct ir_inst* inst = block->insts + id;

        switch (inst->opcode)
        {
            default: break;

            case IR_ADD:
            case IR_SUB:
            {
                ir_inst_id left_id = inst->left;
                ir_inst_id right_id = inst->right;
                
                x64_reg_desc_id left_reg = x64_find_reg_holding_inst(result, block, left_id);

                if (left_reg == x64_INVALID_REG_DESC_ID)
                {
                    left_reg = x64_acquire_free_reg_for_inst(result, left_id);
                    x64_load_inst_into_r64(result, left_reg, block->insts + left_id);
                }

                x64_reg_desc_id right_reg = x64_find_reg_holding_inst(result, block, right_id);

                if (right_reg == x64_INVALID_REG_DESC_ID)
                {
                    right_reg = x64_acquire_free_reg_for_inst(result, right_id);
                    x64_load_inst_into_r64(result, right_reg, block->insts + right_id);
                }

                // IR_ADD   -> [rex 03 modrm]: add left_reg, right_reg
                // IR_SUB   -> [rex 2b modrm]: sub left_reg, right_reg
                {
                    enum x64_reg64 dst_encoding = x64_get_reg_encoding(left_reg);
                    enum x64_reg64 src_encoding = x64_get_reg_encoding(right_reg);

                    u32 instruction = (inst->opcode == IR_ADD) ? 0xc00348 : 0xc02b48;

                    u32 rex_r       = (dst_encoding & 0x8) >> 1;    // REX.R for extended regs on dest operand (r8 and above)
                    u32 rex_b       = (src_encoding & 0x8) >> 3;    // REX.B for extended regs on source operand (r8 and above)
                    u32 modrm_reg   = (dst_encoding & 0x7) << 19;   // ModRM.reg (dest operand)
                    u32 modrm_rm    = (src_encoding & 0x7) << 16;   // ModRM.rm  (source operand)

                    instruction += (rex_r + rex_b) + (modrm_reg + modrm_rm);   

                    x64_emit24(result, instruction);
                }

                x64_change_reg_owner(result, left_reg, id);

                if (right_reg != left_reg)
                    x64_release_reg(result, right_reg);
            } break;

            case IR_RET:
            {
                ir_inst_id retval_id = inst->operand;
                struct ir_inst* retval_inst = block->insts + retval_id;

                x64_reg_desc_id reg_desc_id = x64_find_reg_holding_inst(result, block, retval_id);
                if (reg_desc_id == x64_INVALID_REG_DESC_ID)
                {
                    x64_load_inst_into_r64(result, 0, retval_inst);
                    reg_desc_id = 0;
                }

                x64_move_r64_r64(result, 0, reg_desc_id);

                // c3 ret
                x64_emit8(result, 0xc3);
            } break;
        }
    }
}

static void x64_emit_bytes(struct x64_context* ctx, void* bytes, usize size)
{
    u8* write_at = arena_push_size(ctx->allocator, size);
    copy_mem(write_at, bytes, size);
    ctx->size += size;
}

static void x64_emit8(struct x64_context* ctx, u8 value)       { x64_emit_bytes(ctx, &value, 1); }
static void x64_emit16(struct x64_context* ctx, u16 value)     { x64_emit_bytes(ctx, &value, 2); }
static void x64_emit24(struct x64_context* ctx, u32 value)     { x64_emit_bytes(ctx, &value, 3); }
static void x64_emit32(struct x64_context* ctx, u32 value)     { x64_emit_bytes(ctx, &value, 4); }
static void x64_emit40(struct x64_context* ctx, u64 value)     { x64_emit_bytes(ctx, &value, 5); }
static void x64_emit48(struct x64_context* ctx, u64 value)     { x64_emit_bytes(ctx, &value, 6); }
static void x64_emit56(struct x64_context* ctx, u64 value)     { x64_emit_bytes(ctx, &value, 7); }
static void x64_emit64(struct x64_context* ctx, u64 value)     { x64_emit_bytes(ctx, &value, 8); }

