#include <cmath>
#include <cstdio>
#include "cop1.hpp"

Cop1::Cop1()
{

}

//Returns a floating point value that may be changed to accomodate the FPU's non-compliance to the IEEE 754 standard.
float Cop1::convert(uint32_t value)
{
    //Detect infinities/NaN
    if ((value & 0x7F800000) == 0x7F800000)
        value = (value & 0x80000000) | 0x7F7FFFFF;
    return *(float*)&value;
}

void Cop1::reset()
{
    control.condition = false;
}

bool Cop1::get_condition()
{
    return control.condition;
}

uint32_t Cop1::get_gpr(int index)
{
    return gpr[index].u;
}

void Cop1::mtc(int index, uint32_t value)
{
    printf("\n[FPU] MTC1: %d, $%08X", index, value);
    gpr[index].u = value;
}

uint32_t Cop1::cfc(int index)
{
    switch (index)
    {
        case 0:
            return 0x2E00;
        case 31:
            return control.condition << 23;
        default:
            return 0;
    }
}

void Cop1::ctc(int index, uint32_t value)
{
    printf("\n[FPU] CTC1: $%08X (%d)", value, index);
}

void Cop1::cvt_s_w(int dest, int source)
{
    float bark = (float)gpr[source].u;
    gpr[dest].f = bark;
    printf("\n[FPU] CVT_S_W: %f", bark);
}

void Cop1::cvt_w_s(int dest, int source)
{
    //Default rounding mode in the FPU is truncate
    //TODO: is it possible to change that?
    gpr[dest].u = (uint32_t)trunc(gpr[source].f);
    printf("\n[FPU]CVT_W_S: $%08X", gpr[dest].u);
}

void Cop1::add_s(int dest, int reg1, int reg2)
{
    float op1 = convert(gpr[reg1].u);
    float op2 = convert(gpr[reg2].u);
    gpr[dest].f = op1 + op2;
    printf("\n[FPU] add.s: %f + %f = %f", op1, op2, gpr[dest].f);
}

void Cop1::sub_s(int dest, int reg1, int reg2)
{
    float op1 = convert(gpr[reg1].u);
    float op2 = convert(gpr[reg2].u);
    gpr[dest].f = op1 - op2;
    printf("\n[FPU] sub.s: %f - %f = %f", op1, op2, gpr[dest].f);
}

void Cop1::mul_s(int dest, int reg1, int reg2)
{
    float op1 = convert(gpr[reg1].u);
    float op2 = convert(gpr[reg2].u);
    gpr[dest].f = op1 * op2;
    printf("\n[FPU] mul.s: %f * %f = %f", op1, op2, gpr[dest].f);
}

void Cop1::div_s(int dest, int reg1, int reg2)
{
    float numerator = convert(gpr[reg1].u);
    float denominator = convert(gpr[reg2].u);
    gpr[dest].f = numerator / denominator;
    printf("\n[FPU] div.s: %f / %f = %f", numerator, denominator, gpr[dest].f);
}

void Cop1::mov_s(int dest, int source)
{
    gpr[dest].u = gpr[source].u;
    printf("\n[FPU] mov.s: (%d, %d)", dest, source);
}

void Cop1::neg_s(int dest, int source)
{
    gpr[dest].f = -gpr[source].f;
    printf("\n[FPU] neg.s: %f = -%f", gpr[source].f, gpr[dest].f);
}

void Cop1::adda_s(int reg1, int reg2)
{
    float op1 = convert(gpr[reg1].u);
    float op2 = convert(gpr[reg2].u);
    accumulator.f = op1 + op2;
    printf("\n[FPU] adda.s: %f + %f = %f", op1, op2, accumulator.f);
}

void Cop1::madd_s(int dest, int reg1, int reg2)
{
    float op1 = convert(gpr[reg1].u);
    float op2 = convert(gpr[reg2].u);
    float acc = convert(accumulator.u);
    gpr[dest].f = acc + (op1 * op2);
    printf("\n[FPU] madd.s: %f + %f * %f = %f", acc, op1, op2, gpr[dest].f);
}

void Cop1::c_lt_s(int reg1, int reg2)
{
    control.condition = gpr[reg1].f < gpr[reg2].f;
    printf("\n[FPU] c.lt.s: %f, %f", gpr[reg1].f, gpr[reg2].f);
}

void Cop1::c_eq_s(int reg1, int reg2)
{
    control.condition = gpr[reg1].f == gpr[reg2].f;
    printf("\n[FPU] c.eq.s: %f, %f", gpr[reg1].f, gpr[reg2].f);
}
