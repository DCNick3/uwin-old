// Arithmetic operations, plus shifts and rotates
#include "halfix/cpu/cpu.h"
#include "halfix/cpu/opcodes.h"

#define EXCEPTION_HANDLER return 1

void cpu_arith8(int op, uint8_t* dest, uint8_t src)
{
    int old_cf;
    switch (op & 7) {
    case 0: // ADD
        thread_cpu.lop2 = src;
            thread_cpu.lr = (int8_t)(*dest += src);
            thread_cpu.laux = ADD8;
        break;
    case 1: // OR
        thread_cpu.lr = (int8_t)(*dest |= src);
            thread_cpu.laux = BIT;
        break;
    case 2: // ADC
        old_cf = cpu_get_cf(); // Get CF before any flags state gets modified
        thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = (int8_t)(*dest += src + old_cf);
            thread_cpu.laux = ADC8;
        break;
    case 3: // SBB
        old_cf = cpu_get_cf();
            thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = (int8_t)(*dest -= src + old_cf);
            thread_cpu.laux = SBB8;
        break;
    case 4: // AND
        thread_cpu.lr = (int8_t)(*dest &= src);
            thread_cpu.laux = BIT;
        break;
    case 5: // SUB
        thread_cpu.lop2 = src;
            thread_cpu.lr = (int8_t)(*dest -= src);
            thread_cpu.laux = SUB8;
        break;
    case 6: // XOR
        thread_cpu.lr = (int8_t)(*dest ^= src);
            thread_cpu.laux = BIT;
        break;
    }
}
void cpu_arith16(int op, uint16_t* dest, uint16_t src)
{
    int old_cf;
    switch (op & 7) {
    case 0: // ADD
        thread_cpu.lop2 = src;
            thread_cpu.lr = (int16_t)(*dest += src);
            thread_cpu.laux = ADD16;
        break;
    case 1: // OR
        thread_cpu.lr = (int16_t)(*dest |= src);
            thread_cpu.laux = BIT;
        break;
    case 2: // ADC
        old_cf = cpu_get_cf();
            thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = (int16_t)(*dest += src + old_cf);
            thread_cpu.laux = ADC16;
        break;
    case 3: // SBB
        old_cf = cpu_get_cf();
            thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = (int16_t)(*dest -= src + old_cf);
            thread_cpu.laux = SBB16;
        break;
    case 4: // AND
        thread_cpu.lr = (int16_t)(*dest &= src);
            thread_cpu.laux = BIT;
        break;
    case 5: // SUB
        thread_cpu.lop2 = src;
            thread_cpu.lr = (int16_t)(*dest -= src);
            thread_cpu.laux = SUB16;
        break;
    case 6: // XOR
        thread_cpu.lr = (int16_t)(*dest ^= src);
            thread_cpu.laux = BIT;
        break;
    }
}
void cpu_arith32(int op, uint32_t* dest, uint32_t src)
{
    int old_cf;
    switch (op & 7) {
    case 0: // ADD
        thread_cpu.lop2 = src;
            thread_cpu.lr = *dest += src;
            thread_cpu.laux = ADD32;
        break;
    case 1: // OR
        thread_cpu.lr = *dest |= src;
            thread_cpu.laux = BIT;
        break;
    case 2: // ADC
        old_cf = cpu_get_cf();
            thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = *dest += src + old_cf;
            thread_cpu.laux = ADC32;
        break;
    case 3: // SBB
        old_cf = cpu_get_cf();
            thread_cpu.lop1 = *dest;
            thread_cpu.lop2 = src;
            thread_cpu.lr = *dest -= src + old_cf;
            thread_cpu.laux = SBB32;
        break;
    case 4: // AND
        thread_cpu.lr = *dest &= src;
            thread_cpu.laux = BIT;
        break;
    case 5: // SUB
        thread_cpu.lop2 = src;
            thread_cpu.lr = *dest -= src;
            thread_cpu.laux = SUB32;
        break;
    case 6: // XOR
        thread_cpu.lr = *dest ^= src;
            thread_cpu.laux = BIT;
        break;
        // Case 7 should never be generated
    }
}
void cpu_shift8(int op, uint8_t* dest, uint8_t src)
{
    uint8_t op1, op2, res;
    int cf;
    if (src != 0) {
        switch (op & 7) {
        case 0: // ROL
            op1 = *dest;
            op2 = src & 7;
            if (op2) {
                res = (op1 << op2) | (op1 >> (8 - op2));
            } else
                res = *dest;
            if (src & 31) {
                cpu_set_cf(res & 1);
                cpu_set_of((res ^ (res >> 7)) & 1);
            }
            break;
        case 1: // ROR
            op1 = *dest;
            op2 = src & 7;
            if (op2) {
                res = (op1 >> op2) | (op1 << (8 - op2));
            } else
                res = *dest;
            if(src & 31) {
                cpu_set_cf(res >> 7 & 1);
                cpu_set_of((res ^ (res << 1)) >> 7 & 1);
            }
            break;
        case 2: // RCL
            op1 = *dest;
            op2 = src % 9;
            if (op2) {
                cf = cpu_get_cf();
                res = (op1 << op2) | (cf << (op2 - 1)) | (op1 >> (9 - op2));
                cf = (op1 >> (8 - op2)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((cf ^ (res >> 7)) & 1);
            } else
                res = *dest;
            break;
        case 3: // RCR
            op1 = *dest;
            op2 = src % 9;
            if (op2) {
                cf = cpu_get_cf();
                res = (op1 >> op2) | (cf << (8 - op2)) | (op1 << (9 - op2));
                cf = (op1 >> (op2 - 1)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((res ^ (res << 1)) >> 7 & 1);
            } else
                res = *dest;
            break;
        case 4: // SHL
        case 6: // SAL (same thing as SHL)
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src & 31;
                res = thread_cpu.lr = (int8_t)(*dest << thread_cpu.lop2);
                thread_cpu.laux = SHL8;
            } else res = *dest;
            break;
        case 5: // SHR
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = (int8_t)(*dest >> thread_cpu.lop2);
                thread_cpu.laux = SHR8;
            } else res = *dest;
            break;
        case 7: // SAR
            src &= 31;
            if(src) {
                thread_cpu.lop1 = (int8_t)*dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = (int8_t)((((int8_t)(*dest)) >> thread_cpu.lop2));
                thread_cpu.laux = SAR8;
            } else res = *dest;
            break;
        }
        *dest = res;
    }
}

//static int count = 0;
void cpu_shift16(int op, uint16_t* dest, uint16_t src)
{
    uint16_t op1, op2, res;
    int cf;
    if (src != 0) {
        switch (op & 7) {
        case 0:
            op1 = *dest;
            op2 = src & 15;
            if (op2) {
                res = (op1 << op2) | (op1 >> (16 - op2));
            } else
                res = *dest;
            if(src & 31) {
                cpu_set_cf(res & 1);
                cpu_set_of((res ^ (res >> 15)) & 1);
            }
            break;
        case 1:
            op1 = *dest;
            op2 = src & 15;
            if (op2) {
                res = (op1 >> op2) | (op1 << (16 - op2));
            } else
                res = *dest;
            if(src & 31) {
                cpu_set_cf(res >> 15 & 1);
                cpu_set_of((res ^ (res << 1)) >> 15 & 1);
            }
            break;
        case 2:
            op1 = *dest;
            op2 = src % 17;
            if (op2) {
                cf = cpu_get_cf();
                res = (op1 << op2) | (cf << (op2 - 1)) | (op1 >> (17 - op2));
                cf = (op1 >> (16 - op2)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((cf ^ (res >> 15)) & 1);
            } else
                res = *dest;
            break;
        case 3:
            op1 = *dest;
            op2 = src % 17;
            if (op2) {
                int cf = cpu_get_cf();
                res = (op1 >> op2) | (cf << (16 - op2)) | (op1 << (17 - op2));
                cf = (op1 >> (op2 - 1)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((res ^ (res << 1)) >> 15 & 1);
            } else
                res = *dest;
            break;
        case 4:
        case 6:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = (int16_t)(*dest << thread_cpu.lop2);
                thread_cpu.laux = SHL16;
                break;
            } else res = *dest;
        case 5:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = (int16_t)(*dest >> thread_cpu.lop2);
                thread_cpu.laux = SHR16;
            } else res = *dest;
            break;
        case 7:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = (int16_t)*dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = (int16_t)((((int16_t)(*dest)) >> thread_cpu.lop2));
                thread_cpu.laux = SAR16;
            } else res = *dest;
            break;
        }
        *dest = res;
    }
}
void cpu_shift32(int op, uint32_t* dest, uint32_t src)
{
    uint32_t op1, op2, res;
    int cf;
    if (src != 0) {
        switch (op & 7) {
        case 0:
            op1 = *dest;
            op2 = src & 31;
            if (op2) {
                res = (op1 << op2) | (op1 >> (32 - op2));
            } else
                res = *dest;
            if(src & 31) {
                cpu_set_cf(res & 1);
                cpu_set_of((res ^ (res >> 31)) & 1);
            }
            break;
        case 1:
            op1 = *dest;
            op2 = src & 31;
            if (op2) {
                res = (op1 >> op2) | (op1 << (32 - op2));
            } else
                res = *dest;
            if(src & 31) {
                cpu_set_cf(res >> 31 & 1);
                cpu_set_of((res ^ (res << 1)) >> 31 & 1);
            }
            break;
        case 2:
            op1 = *dest;
            op2 = src % 33;
            if (op2) {
                cf = cpu_get_cf();
                if (op2 == 1)
                    res = (op1 << 1) | cf;
                else
                    res = (op1 << op2) | (cf << (op2 - 1)) | (op1 >> (33 - op2));
                cf = (op1 >> (32 - op2)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((cf ^ (res >> 31)) & 1);
            } else
                res = *dest;
            break;
        case 3:
            op1 = *dest;
            op2 = src % 33;
            if (op2) {
                cf = cpu_get_cf();
                if (op2 == 1)
                    res = (op1 >> 1) | (cf << 31);
                else
                    res = (op1 >> op2) | (cf << (32 - op2)) | (op1 << (33 - op2));
                cf = (op1 >> (op2 - 1)) & 1;
                cpu_set_cf(cf);
                cpu_set_of((res ^ (res << 1)) >> 31 & 1);
            } else
                res = *dest;
            break;
        case 4:
        case 6:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = *dest << thread_cpu.lop2;
                thread_cpu.laux = SHL32;
            } else res = *dest;
            break;
        case 5:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = *dest >> thread_cpu.lop2;
                thread_cpu.laux = SHR32;
            } else res = *dest;
            break;
        case 7:
            src &= 31;
            if(src) {
                thread_cpu.lop1 = *dest;
                thread_cpu.lop2 = src;
                res = thread_cpu.lr = ((int32_t)(*dest)) >> thread_cpu.lop2;
                thread_cpu.laux = SAR32;
            } else res = *dest;
            break;
        }
        *dest = res;
    }
}

int cpu_muldiv8(int op, uint32_t src)
{
    uint32_t result, result_mod;
    int8_t low, high;
    switch (op & 7) {
    case 0 ... 3:
        ABORT();
    case 4: // mul
        result = src * thread_cpu.reg8[AL];
            thread_cpu.lop1 = 0;
            thread_cpu.lop2 = result >> 8;
        break;
    case 5: // imul
        result = ((uint32_t)(int8_t)src * (uint32_t)(int8_t)thread_cpu.reg8[AL]);
        low = result;
        high = result >> 8;
            thread_cpu.lop1 = low >> 7;
            thread_cpu.lop2 = high;
        break;
    case 6: // div
        if (src == 0)
            EXCEPTION_DE();
        result = thread_cpu.reg16[AX] / src;
        if (result > 0xFF)
            EXCEPTION_DE();
        result_mod = thread_cpu.reg16[AX] % src;
            thread_cpu.reg8[AL] = result;
            thread_cpu.reg8[AH] = result_mod;
        return 0;
    case 7: { // idiv
        int16_t temp;
        if (src == 0)
            EXCEPTION_DE();
        temp = result = (int16_t)thread_cpu.reg16[AX] / (int8_t)src;
        if (temp > 0x7F || temp < -0x80)
            EXCEPTION_DE();
        result_mod = (int16_t)thread_cpu.reg16[AX] % (int8_t)src;
        thread_cpu.reg8[AL] = result;
        thread_cpu.reg8[AH] = result_mod;
        return 0;
    }
    }
    thread_cpu.lr = (int8_t)result;
    thread_cpu.laux = MUL;
    thread_cpu.reg16[AX] = result;
    return 0;
}
int cpu_muldiv16(int op, uint32_t src)
{
    uint32_t result, result_mod, original;
    int16_t low, high;
    switch (op & 7) {
    case 0 ... 3:
        ABORT();
    case 4: // mul
        result = src * thread_cpu.reg16[AX];
            thread_cpu.lop1 = 0;
            thread_cpu.lop2 = result >> 16;
        break;
    case 5: // imul
        result = (uint32_t)(int16_t)src * (uint32_t)(int16_t)thread_cpu.reg16[AX];
        low = result;
        high = result >> 16;
            thread_cpu.lop1 = low >> 15;
            thread_cpu.lop2 = high;
        break;
    case 6: // div
        if (src == 0)
            EXCEPTION_DE();
        original = thread_cpu.reg16[DX] << 16 | thread_cpu.reg16[AX];
        result = original / src;
        if (result > 0xFFFF)
            EXCEPTION_DE();
        result_mod = original % src;
            thread_cpu.reg16[AX] = result;
            thread_cpu.reg16[DX] = result_mod;
        return 0;
    case 7: { // idiv
        int32_t temp;
        if (src == 0)
            EXCEPTION_DE();
        original = thread_cpu.reg16[DX] << 16 | thread_cpu.reg16[AX];
        temp = result = (int32_t)original / (int16_t)src;
        if (temp > 0x7FFF || temp < -0x8000)
            EXCEPTION_DE();
        result_mod = (int32_t)original % (int16_t)src;
        thread_cpu.reg16[AX] = result;
        thread_cpu.reg16[DX] = result_mod;
        return 0;
    }
    }
    thread_cpu.lr = (int16_t)result;
    thread_cpu.laux = MUL;
    thread_cpu.reg16[AX] = result;
    thread_cpu.reg16[DX] = result >> 16;
    return 0;
}
int cpu_muldiv32(int op, uint32_t src)
{
    uint64_t result, result_mod, original;
    int32_t low, high;
    switch (op & 7) {
    case 0 ... 3:
        ABORT();
    case 4: // mul
        result = (uint64_t)src * thread_cpu.reg32[EAX];
            thread_cpu.lop1 = 0;
            thread_cpu.lop2 = result >> 32;
        break;
    case 5: // imul
        result = (uint64_t)(int32_t)src * (uint64_t)(int32_t)thread_cpu.reg32[EAX];
        low = result;
        high = result >> 32;
            thread_cpu.lop1 = low >> 31;
            thread_cpu.lop2 = high;
        break;
    case 6: // div
        if (src == 0)
            EXCEPTION_DE();
        original = (uint64_t)thread_cpu.reg32[EDX] << 32 | thread_cpu.reg32[EAX];
        result = original / src;
        if (result > 0xFFFFFFFF)
            EXCEPTION_DE();
        result_mod = original % src;
            thread_cpu.reg32[EAX] = result;
            thread_cpu.reg32[EDX] = result_mod;
        return 0;
    case 7: { // idiv
        int64_t temp;
        if (src == 0)
            EXCEPTION_DE();
        original = (uint64_t)thread_cpu.reg32[EDX] << 32 | thread_cpu.reg32[EAX];
        temp = result = (int64_t)original / (int32_t)src;

        if (temp > 0x7FFFFFFF || temp < -(int64_t)0x80000000)
            EXCEPTION_DE();
        result_mod = (int64_t)original % (int32_t)src;
        thread_cpu.reg32[EAX] = result;
        thread_cpu.reg32[EDX] = result_mod;
        return 0;
    }
    }
    thread_cpu.lr = result;
    thread_cpu.laux = MUL;
    thread_cpu.reg32[EAX] = result;
    thread_cpu.reg32[EDX] = result >> 32;
    return 0;
}

// Negation operations
// Note:
//  neg eax
// is equal to
//  mov ecx, eax
//  xor eax, eax
//  sub eax, ecx ; 0 - eax
// in terms of flags, results, etc.
void cpu_neg8(uint8_t* dest)
{
    thread_cpu.lr = (int8_t)(*dest = -(thread_cpu.lop2 = *dest));
    thread_cpu.laux = SUB8;
}
void cpu_neg16(uint16_t* dest)
{
    thread_cpu.lr = (int16_t)(*dest = -(thread_cpu.lop2 = *dest));
    thread_cpu.laux = SUB16;
}
void cpu_neg32(uint32_t* dest)
{
    thread_cpu.lr = *dest = -(thread_cpu.lop2 = *dest);
    thread_cpu.laux = SUB32;
}

// Double shifts
void cpu_shrd16(uint16_t* dest_ptr, uint16_t src, int count)
{
    count &= 0x1F;
    if (count) {
        uint16_t dest = *dest_ptr, result;
        if (count < 16)
            result = (dest >> count) | (src << (16 - count));
        else {
            result = (src >> (count - 16)) | (dest << (32 - count));
            dest = src;
            count -= 16;
        }
        src = count;

        thread_cpu.lr = (int16_t)result;
        thread_cpu.lop1 = dest;
        thread_cpu.lop2 = src;
        thread_cpu.laux = SHRD16;
        *dest_ptr = result;
    }
}

void cpu_shrd32(uint32_t* dest_ptr, uint32_t src, int count)
{
    count &= 0x1F;
    if (count) {
        uint32_t dest = *dest_ptr, result;
        result = (dest >> count) | (src << (32 - count));
        //dest = src;
        //src = count;

        thread_cpu.lr = result;
        thread_cpu.lop1 = dest;
        thread_cpu.lop2 = count;
        thread_cpu.laux = SHRD32; // SHRD is handled the same way as SHR
        *dest_ptr = result;
    }
}

void cpu_shld16(uint16_t* dest_ptr, uint16_t src, int count)
{
    count &= 0x1F;
    if (count) {
        uint16_t dest = *dest_ptr, result;
        if (count < 16)
            result = (dest << count) | (src >> (16 - count));
        else
            result = (src << (count - 16)) | (dest >> (32 - count));

        if (count > 16)
            dest = src;
        src = count;

        thread_cpu.lr = (int16_t)result;
        thread_cpu.lop1 = dest;
        thread_cpu.lop2 = src;
        thread_cpu.laux = SHLD16;
        *dest_ptr = result;
    }
}
void cpu_shld32(uint32_t* dest_ptr, uint32_t src, int count)
{
    count &= 0x1F;
    if (count) {
        uint32_t dest = *dest_ptr, result;
        result = (dest << count) | (src >> (32 - count));

        src = count;

        thread_cpu.lr = result;
        thread_cpu.lop1 = dest;
        thread_cpu.lop2 = src;
        thread_cpu.laux = SHLD32;
        *dest_ptr = result;
    }
}

void cpu_inc8(uint8_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;

    thread_cpu.lr = (int8_t)(++*dest_ptr);
    thread_cpu.laux = INC8;
}
void cpu_inc16(uint16_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;
    thread_cpu.lr = (int16_t)(++*dest_ptr);
    thread_cpu.laux = INC16;
}
void cpu_inc32(uint32_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;

    thread_cpu.lr = ++*dest_ptr;
    thread_cpu.laux = INC32;
}
void cpu_dec8(uint8_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;

    thread_cpu.lr = (int8_t)(--*dest_ptr);
    thread_cpu.laux = DEC8;
}
void cpu_dec16(uint16_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;

    thread_cpu.lr = (int16_t)(--*dest_ptr);
    thread_cpu.laux = DEC16;
}
void cpu_dec32(uint32_t* dest_ptr)
{
    int cf = cpu_get_cf();
    thread_cpu.eflags &= ~EFLAGS_CF;
    thread_cpu.eflags |= cf;

    thread_cpu.lr = --*dest_ptr;
    thread_cpu.laux = DEC32;
}

void cpu_not8(uint8_t* dest_ptr)
{
    *dest_ptr = ~*dest_ptr;
}
void cpu_not16(uint16_t* dest_ptr)
{
    *dest_ptr = ~*dest_ptr;
}
void cpu_not32(uint32_t* dest_ptr)
{
    *dest_ptr = ~*dest_ptr;
}

uint8_t cpu_imul8(uint8_t op1, uint8_t op2)
{
    uint16_t result = (uint16_t)(int8_t)op1 * (uint16_t)(int8_t)op2;
    thread_cpu.laux = MUL;
    int8_t high = result >> 8, low = result;
    thread_cpu.lop1 = low >> 7;
    thread_cpu.lop2 = high;
    thread_cpu.lr = low;
    return (uint8_t)result;
}
uint16_t cpu_imul16(uint16_t op1, uint16_t op2)
{
    uint32_t result = (uint32_t)(int16_t)op1 * (uint32_t)(int16_t)op2;
    thread_cpu.laux = MUL;
    int16_t high = result >> 16, low = result;
    thread_cpu.lop1 = low >> 15;
    thread_cpu.lop2 = high;
    thread_cpu.lr = low;
    return (uint16_t)result;
}
uint32_t cpu_imul32(uint32_t op1, uint32_t op2)
{
    uint64_t result = (uint64_t)(int32_t)op1 * (uint64_t)(int32_t)op2;
    thread_cpu.laux = MUL;
    int32_t high = result >> 32, low = result;
    thread_cpu.lop1 = low >> 31;
    thread_cpu.lop2 = high;
    thread_cpu.lr = low;
    return (uint32_t)result;
}

void cpu_cmpxchg8(uint8_t* op1, uint8_t op2)
{
    thread_cpu.lop2 = *op1;
    thread_cpu.lr = (int8_t)(thread_cpu.reg8[AL] - thread_cpu.lop2);
    thread_cpu.laux = SUB8;
    if (!thread_cpu.lr)
        *op1 = op2;
    else
        thread_cpu.reg8[AL] = thread_cpu.lop2;
}
void cpu_cmpxchg16(uint16_t* op1, uint16_t op2)
{
    thread_cpu.lop2 = *op1;
    thread_cpu.lr = (int16_t)(thread_cpu.reg16[AX] - thread_cpu.lop2);
    thread_cpu.laux = SUB16;
    if (!thread_cpu.lr)
        *op1 = op2;
    else
        thread_cpu.reg16[AX] = thread_cpu.lop2;
}
void cpu_cmpxchg32(uint32_t* op1, uint32_t op2)
{
    thread_cpu.lop2 = *op1;
    thread_cpu.lr = thread_cpu.reg32[EAX] - thread_cpu.lop2;
    thread_cpu.laux = SUB32;
    if (!thread_cpu.lr)
        *op1 = op2;
    else
        thread_cpu.reg32[EAX] = thread_cpu.lop2;
}

void xadd8(uint8_t* op1, uint8_t* op2)
{
    thread_cpu.lop2 = *op2;
    thread_cpu.lr = (int8_t)(*op1 + thread_cpu.lop2);
    thread_cpu.laux = ADD8;
    *op2 = *op1;
    *op1 = thread_cpu.lr;
}
void xadd16(uint16_t* op1, uint16_t* op2)
{
    thread_cpu.lop2 = *op2;
    thread_cpu.lr = (int16_t)(*op1 + thread_cpu.lop2);
    thread_cpu.laux = ADD16;
    *op2 = *op1;
    *op1 = thread_cpu.lr;
}
void xadd32(uint32_t* op1, uint32_t* op2)
{
    thread_cpu.lop2 = *op2;
    thread_cpu.lr = *op1 + thread_cpu.lop2;
    thread_cpu.laux = ADD32;
    *op2 = *op1;
    *op1 = thread_cpu.lr;
}