"""Crash repair through the authored, hash-pinned Patch Pipeline only.

No generated source or dependency is patched in place. Native/owned adapters
share guest-resident ownership; all instruction/delay slots are verified.
"""
import copy
import hashlib
import struct
from legacy_character_presentation_policy import ELFS
from compose_legacy_mod_policy import elf_sections, elf_functions


def compose_memory_safety(policy, elf, revision):
    if revision not in ELFS or hashlib.sha256(elf.read_bytes()).hexdigest() != ELFS[revision]:
        raise ValueError('Memory safety requires a reviewed retail ELF')
    symbols = elf_functions(elf, (1, 2))
    words = {base+i: struct.unpack_from('>I', data, i)[0]
             for base, data in elf_sections(elf) for i in range(0, len(data), 4)}
    result = copy.deepcopy(policy)
    rev = 77 if revision == 'us.v77' else 80
    memory_delta = 0 if rev == 77 else 0x240
    transition_delta = 0 if rev == 77 else 0x520
    declaration = 'extern uint32_t dkr_legacy_memory_service(uint8_t*, recomp_context*, unsigned, unsigned, uint32_t, uint32_t); '
    reason = 'Pre-write guest heap/queue bounds, deferred reserve ownership, and allocation-free transition fallback.'

    def symbol(name):
        matches = symbols.get(name, set())
        if len(matches) != 1:
            raise ValueError('Ambiguous memory safety symbol: '+name)
        return next(iter(matches))[0]

    def service(op, a='0U', b='0U'):
        return f'dkr_legacy_memory_service(rdram, ctx, {op}U, {rev}U, (uint32_t)({a}), (uint32_t)({b}))'

    def insert(key, address, entry):
        matches = [x for x in result.setdefault(key, []) if int(x[address], 0) == int(entry[address], 0)]
        if matches:
            if matches != [entry]:
                raise ValueError('Conflicting memory safety patch '+entry[address])
        else:
            result[key].append(entry)

    def hook(name, pc, expected, text, replacement=None):
        start, size = next(iter(symbols[name]))
        if not start <= pc < start+size or words.get(pc) != expected:
            raise ValueError('Memory safety instruction changed '+name+' '+hex(pc))
        existing = [x for x in result.get('instructionPatches', []) if int(x['vram'], 0) == pc]
        paired = dict(function=name,vram=hex(pc),value=hex(replacement),reason=reason) if replacement is not None else None
        if existing and existing != [paired]:
            raise ValueError('Conflicting memory safety instruction '+hex(pc))
        insert('functionHooks', 'beforeVram', dict(function=name, beforeVram=hex(pc),
            text=declaration+text, reason=reason))
        if replacement is not None:
            insert('instructionPatches', 'vram', dict(function=name, vram=hex(pc), value=hex(replacement), reason=reason))

    def memhook(name, pc, expected, text, replacement=None):
        hook(name, pc+memory_delta, expected, text, replacement)

    # Clear the descriptor before the pool is made; stock remains 1600 slots.
    memhook('mempool_init_main', 0x80070b58, 0x24060640,
        'ctx->r6 = '+service(0)+';', 0)
    memhook('mempool_init_main', 0x80070b5c, 0x0c01c42c if rev == 77 else 0x0c01c4bc,
        '{ recomp_context dkr_alloc = *ctx; dkr_alloc.f_odd = dkr_alloc.mips3_float_mode ? &dkr_alloc.f1.u32l : &dkr_alloc.f0.u32h; '
        'dkr_alloc.r4 = '+service(1)+'; dkr_alloc.r5 = (int32_t)0xffff00ffU; '
        'mempool_alloc(rdram, &dkr_alloc); '+service(2, 'dkr_alloc.r2')+'; }')

    # Replace enqueue at its leaf entry, before ANY original store. Draining
    # still uses the retail timers/order and real mempool_free_addr owner.
    memhook('mempool_free_queue', 0x80071440, 0x3c058012,
        service(4, 'ctx->r4')+'; return;')
    for name, pc, word, register in [
        ('mempool_free_queue_clear', 0x800711cc, 0x265235c8 if rev == 77 else 0x26523b48, 18),
        ('mempool_free_queue_clear', 0x800711d0, 0x261035c8 if rev == 77 else 0x26103b48, 16),
        ('mempool_free_queue_clear', 0x8007121c, 0x258c35c8 if rev == 77 else 0x258c3b48, 12),
        ('mempool_free_queue_clear', 0x80071238, 0x25ce35c8 if rev == 77 else 0x25ce3b48, 14),
        ('mempool_free_timer', 0x800710f0, 0x265235c8 if rev == 77 else 0x26523b48, 18),
    ]:
        # Use symbol-relative verification for the differing static queue base.
        # v80's BSS base differs by 0x580, unlike its code offset (0x240).
        expected = (word & 0xffff0000) | (symbol('gFreeQueue') & 0xffff)
        memhook(name, pc, expected, f'ctx->r{register} = (int32_t)'+service(3)+';', 0)
    memhook('mempool_free_queue_clear', 0x80071198, 0x27bdffd0, service(3)+';')
    memhook('mempool_free_timer', 0x800710b0, 0x27bdffd0, service(3)+';')
    memhook('mempool_free_addr', 0x80071278, 0x27bdffe0,
        'if ('+service(6, 'ctx->r4')+') return; unsigned dkr_free_visits = 0; unsigned dkr_free_max = 0;')
    memhook('mempool_free_addr', 0x80071288, 0x00027100,
        'dkr_free_max = '+service(7, 'ctx->r2')+';')
    memhook('mempool_free_addr', 0x800712ac, 0x00a60019,
        'if ((uint32_t)ctx->r5 >= dkr_free_max || ++dkr_free_visits > dkr_free_max) '+service(11, 'ctx->r4', 'ctx->r31')+';')

    memhook('mempool_slot_find', 0x80070d3c, 0x27bdffc0,
        'unsigned dkr_find_visits = 0; unsigned dkr_find_max = '+service(7, 'ctx->r4')+'; '
        'if ((int32_t)ctx->r5 <= 0 || (uint32_t)ctx->r5 > 0x7ffffff0U) '+service(11, 'ctx->r5', 'ctx->r31')+';')
    memhook('mempool_slot_find', 0x80070dc8, 0x00880019,
        'if ((uint32_t)ctx->r4 >= dkr_find_max || ++dkr_find_visits > dkr_find_max) '+service(11, 'ctx->r4', 'ctx->r31')+';')
    for name, pc, word in [('mempool_slot_assign', 0x8007178c, 0x240b0014), ('mempool_slot_clear', 0x8007164c, 0x240a0014)]:
        memhook(name, pc, word, service(8, 'ctx->r4', 'ctx->r5')+';')
    memhook('mempool_alloc_safe', 0x80070cdc, 0x14400008,
        'if (!ctx->r2) '+service(10, 'MEM_W(0x20, ctx->r29)', 'MEM_W(0x14, ctx->r29)')+';')

    # Optional transition allocations get a permanent three-buffer reserve.
    # If reserve/optional allocation fail, retain transition_begin's colour,
    # duration, inversion and end timer, but use the nonallocating full fade.
    for name, alloc_pc, next_pc, next_word, frame, arg, saves in [
        ('transition_init_shape', 0x800c0b64, 0x800c0b6c, 0x8fa30030, 0x50, 0x50, [(16,0x18),(17,0x1c),(18,0x20),(31,0x24)]),
        ('transition_init_circle', 0x800c1608, 0x800c1610, 0x3c04800e, 0x38, 0x38, [(i,0x1c+4*(i-16)) for i in range(16,22)]+[(31,0x34)]),
    ]:
        alloc_pc += transition_delta;next_pc += transition_delta
        expected = 0x0c000000 | ((symbol('mempool_alloc_safe') >> 2) & 0x3ffffff)
        # Leave the original delay instruction intact.
        start, size = next(iter(symbols[name]))
        if words.get(alloc_pc) != expected or not start <= alloc_pc < start+size:
            raise ValueError('Transition allocation changed')
        insert('instructionPatches','vram',dict(function=name,vram=hex(alloc_pc),value='0x0',reason=reason))
        clear = ''.join(f'MEM_W(0, (int32_t)0x{symbol(s)+offset:08x}U) = 0; ' for s,offset in
            [('sTransitionVtx',0),('sTransitionVtx',4),('sTransitionTris',0),('sTransitionTris',4),
             ('gTransitionNextVtx',0),('gTransitionVtxStep',0),('gTransitionVertexTarget',0),('gCurFadeTransition',0)])
        restore = ''.join(f'ctx->r{reg} = MEM_W(0x{off:x}, ctx->r29); ' for reg,off in saves)
        if name == 'transition_init_circle':
            restore += 'ctx->f_odd[(21 - 1) * 2] = MEM_W(0x10, ctx->r29); ctx->f20.u32l = MEM_W(0x14, ctx->r29); '
        text = '{ unsigned dkr_request = (uint32_t)ctx->r4; uint32_t dkr_reserved = '+service(5, 'dkr_request')+'; '
        text += 'recomp_context dkr_alloc = *ctx; dkr_alloc.f_odd = dkr_alloc.mips3_float_mode ? &dkr_alloc.f1.u32l : &dkr_alloc.f0.u32h; '
        text += 'if (!dkr_reserved) { mempool_alloc(rdram, &dkr_alloc); dkr_reserved = (uint32_t)dkr_alloc.r2; } ctx->r2 = (int32_t)dkr_reserved; '
        text += 'if (!dkr_reserved) { '+service(9, 'dkr_request', 'MEM_W(0x'+hex(saves[-1][1])[2:]+', ctx->r29)')+'; '+clear
        text += f'dkr_alloc.r4 = MEM_W(0x{arg:x}, ctx->r29); transition_fullscreen_start(rdram, &dkr_alloc); '
        text += restore+f'ctx->r29 = ADD32(ctx->r29, 0x{frame:x}); return; '+'} }'
        hook(name,next_pc,next_word,text)
    return result
