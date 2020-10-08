
import itertools
import functools

TEST_MOV = False
TEST_SHIFTS = False
TEST_DSHIFTS = False
TEST_INC = False
TEST_ADD = False
TEST_MUL = False
TEST_IMUL = False
TEST_DIV = False
TEST_BSF = False
TEST_JCC = True

DREGS = [ 'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'esi', 'esp', 'ebp' ]
WREGS = [ 'ax', 'bx', 'cx', 'dx', 'si', 'di', 'sp', 'bp' ]
BREGS = [ 'al', 'ah', 'bl', 'dh', 'cl', 'ch', 'dl', 'dh' ]

REGS = DREGS + WREGS + BREGS

def emit(x):
    print(x)

def mkmovimm(reg, imm):
    emit('mov $%d, %%%s' % (imm, reg))

def regsz(reg):
    if reg in DREGS:
        return 32
    if reg in WREGS:
        return 16
    if reg in BREGS:
        return 8
    raise RuntimeError()

str_id = 0

def emit_print(text):
    global str_id
    emit(".data\ns_%d: .string \"%s\"\n.text" % (str_id, text.replace('\\', r'\\').replace('"', r'\"')))
    emit("mov save_esp, %esp")
    emit("push $s_%d" % str_id)
    emit("call _kprint")
    str_id += 1


bvals_many = list(range(0, 256, 7)) + list(range(-128, 0, 3))
wvals_many = list(range(0, 2 ** 16, 2 ** 11 - 1)) + list(range(-2 ** 15, 0, 2 ** 11 - 3))
dvals_many = list(range(0, 2 ** 32, 2 ** 24 - 1)) + list(range(-2 ** 31, 0, 2 ** 24 - 3))

emit(".data\nsave_esp: .int 0")

emit(".text\n.global _start\n_start:\nmov %esp, save_esp")

emit_print("begin jittest")

if TEST_MOV:
    emit_print("test mov")
    for reg, imm in itertools.product(BREGS + WREGS + DREGS, bvals_many):
        mkmovimm(reg, imm)
    for reg, imm in itertools.product(WREGS + DREGS, wvals_many):
        mkmovimm(reg, imm)
    for reg, imm in itertools.product(DREGS, dvals_many):
        mkmovimm(reg, imm)

if TEST_SHIFTS:
    emit_print("test shifts")
    SHIFTS = [
        'shr', 'sar', 'shl',
        'rol',
        'ror',
        'rcr',
    ]
    shift_amounts = range(0, 34)
    for shift in SHIFTS:
        emit_print("test shifts.%s" % shift)
        for reg, amnt, val in itertools.product(['al', 'ah', 'ax', 'eax'], shift_amounts, bvals_many):
            mkmovimm(reg, val)
            emit('%s $%d, %%%s' % (shift, amnt, reg))

if TEST_DSHIFTS:
    emit_print("test dshifts")
    DSHIFTS = [
        'shrd', 'shld',
    ]
    shift_amounts_32 = range(0, 32)
    shift_amounts_16 = range(0, 16)
    for dshift in DSHIFTS:
        emit_print("test dshifts.%s" % dshift)
        for amnt, val1, val2 in itertools.product(shift_amounts_16, wvals_many, wvals_many):
            mkmovimm('ax', val1)
            mkmovimm('bx', val2)
            emit('%s $%d, %%%s, %%%s' % (dshift, amnt, 'ax', 'bx'))
        for amnt, val1, val2 in itertools.product(shift_amounts_32, wvals_many, wvals_many):
            mkmovimm('eax', val1 << 16)
            mkmovimm('ebx', val2 << 16)
            emit('%s $%d, %%eax, %%ebx' % (dshift, amnt))

incbvals = [ 1, 2, 3, 12, 17, 0x63, 0x7f, 0x80, 0x81, 0xfe, 0xff ]
incwvals = [ x << 8 for x in incbvals ]
incdvals = [ x << 24 for x in incbvals ]

if TEST_INC:
    emit_print("test inc")
    for reg, val in itertools.product(BREGS + WREGS + DREGS, incbvals):
        mkmovimm(reg, val)
        emit("inc %%%s" % reg)

    for reg, val in itertools.product(WREGS + DREGS, incwvals):
        mkmovimm(reg, val)
        emit("inc %%%s" % reg)

    for reg, val in itertools.product(DREGS, incdvals):
        mkmovimm(reg, val)
        emit("inc %%%s" % reg)

if TEST_ADD:
    emit_print("test add")
    for reg, val1, val2 in itertools.product(BREGS + WREGS + DREGS, incbvals, incbvals):
        mkmovimm(reg, val1)
        emit("add $%d, %%%s" % (val2, reg))

    for reg, val1, val2 in itertools.product(WREGS + DREGS, incwvals, incwvals):
        mkmovimm(reg, val1)
        emit("add $%d, %%%s" % (val2, reg))

    for reg, val1, val2 in itertools.product(DREGS, incdvals, incdvals):
        mkmovimm(reg, val1)
        emit("add $%d, %%%s" % (val2, reg))

if TEST_MUL:
    emit_print("test mul")
    for reg, val1, val2 in itertools.product(BREGS + WREGS + DREGS, incbvals, incbvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('mul %%%s' % reg)
    for reg, val1, val2 in itertools.product(WREGS + DREGS, incwvals, incwvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('mul %%%s' % reg)
    for reg, val1, val2 in itertools.product(DREGS, incdvals, incdvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('mul %%%s' % reg)

if TEST_IMUL:
    emit_print("test imul")
    for reg, val1, val2 in itertools.product(BREGS + WREGS + DREGS, incbvals, incbvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('imul %%%s' % reg)
    for reg, val1, val2 in itertools.product(WREGS + DREGS, incwvals, incwvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('imul %%%s' % reg)
    for reg, val1, val2 in itertools.product(DREGS, incdvals, incdvals):
        mkmovimm(reg, val1)
        mkmovimm('eax', val2)
        emit('imul %%%s' % reg)

    for reg1, reg2, val1, val2 in itertools.chain(itertools.product(WREGS, WREGS, incbvals, incbvals), itertools.product(DREGS, DREGS, incbvals, incbvals)):
        mkmovimm(reg1, val1)
        mkmovimm(reg2, val2)
        emit('imul %%%s, %%%s' % (reg1, reg2))
    for reg1, reg2, val1, val2 in itertools.chain(itertools.product(WREGS, WREGS, incwvals, incwvals), itertools.product(DREGS, DREGS, incwvals, incwvals)):
        mkmovimm(reg1, val1)
        mkmovimm(reg2, val2)
        emit('imul %%%s, %%%s' % (reg1, reg2))
    for reg1, reg2, val1, val2 in itertools.product(DREGS, DREGS, incdvals, incdvals):
        mkmovimm(reg1, val1)
        mkmovimm(reg2, val2)
        emit('imul %%%s, %%%s' % (reg1, reg2))

    for reg1, reg2, val1, val2 in itertools.chain(itertools.product(WREGS, WREGS, incbvals, incbvals), itertools.product(DREGS, DREGS, incbvals, incbvals)):
        mkmovimm(reg1, val1)
        emit('imul $%d, %%%s, %%%s' % (val2, reg1, reg2))
    for reg1, reg2, val1, val2 in itertools.chain(itertools.product(WREGS, WREGS, incwvals, incwvals), itertools.product(DREGS, DREGS, incwvals, incwvals)):
        mkmovimm(reg1, val1)
        emit('imul $%d, %%%s, %%%s' % (val2, reg1, reg2))
    for reg1, reg2, val1, val2 in itertools.product(DREGS, DREGS, incdvals, incdvals):
        mkmovimm(reg1, val1)
        emit('imul $%d, %%%s, %%%s' % (val2, reg1, reg2))

if TEST_DIV:
    emit_print("test div")
    for reg, val1, val2 in itertools.product(BREGS, incbvals, incbvals):
        if val2 // val1 < 2 ** regsz(reg) and reg != 'ah':
            mkmovimm(reg, val1)
            mkmovimm('ax', val2)
            emit('div %%%s' % reg)
    for reg, val1, val2 in itertools.product(WREGS, incwvals, incwvals):
        if val2 // val1 < 2 ** regsz(reg) and reg != 'dx':
            mkmovimm(reg, val1)
            mkmovimm('ax', val2)
            mkmovimm('dx', val2 >> 16)
            emit('div %%%s' % reg)
    for reg, val1, val2 in itertools.product(DREGS, incdvals, incdvals):
        if val2 // val1 < 2 ** regsz(reg) and reg != 'edx':
            mkmovimm(reg, val1)
            mkmovimm('eax', val2)
            mkmovimm('edx', val2 >> 32)
            emit('div %%%s' % reg)

if TEST_BSF:
    emit_print("test bsf")
    for val in wvals_many:
        mkmovimm('ax', val)
        emit('bsf %ax, %bx')
    for val in dvals_many:
        mkmovimm('eax', val)
        emit('bsf %eax, %ebx')

if TEST_JCC:
    emit_print("test jcc")
    # TODO: test flags set with arithmetics

    CF = 0x1
    PF = 0x4
    ZF = 0x40
    SF = 0x80
    FLAG_MASKS = [CF, PF, ZF, SF]
    JUMPS = ['o', 'no', 'b', 'nb', 'z', 'nz', 'be', 'nbe', 's', 'ns', 'p', 'np', 'l', 'nl', 'le', 'nle']
    id = 0
    for jump in JUMPS:
        for flags in itertools.chain.from_iterable(itertools.combinations(FLAG_MASKS, x) for x in range(0, len(FLAG_MASKS))):
            eflags = functools.reduce(lambda x, y: x | y, list(flags), 0)
            emit("mov $%d, %%ah" % eflags)
            emit("sahf")

            label_name = "jcc_test_%d" % id
            id += 1

            emit("j%s %s" % (jump, label_name))
            emit("nop")
            emit("%s:" % label_name)



emit("mov save_esp, %esp")
emit("jmp _kexit")
