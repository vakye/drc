
struct x64_context {
    struct arena*   allocator;
    u8*             base;
    usize           size;
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

static void x64_generate(struct arena* allocator, struct ir_inst_block* block, struct x64_context* result)
{
    zero_type(result);
    result->allocator = allocator;
    result->base = arena_alloc_at(allocator);

    for (ir_inst_id id = 0; id < block->inst_count; id++)
    {
        struct ir_inst* inst = block->insts + id;

        switch (inst->opcode)
        {
            default: break;

            case IR_RET:
            {
                if (id > 0)
                {
                    ir_inst_id retval_id = id - 1;
                    struct ir_inst* ret_inst = block->insts + retval_id;

                    ensure(ret_inst->opcode == IR_IMM64, str("returning anything other than an immediate is not supported at the moment"));

                    // 48 b8 imm64  mov rax, imm64
                    x64_emit16(result, 0xb848);
                    x64_emit64(result, ret_inst->imm);
                }

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

