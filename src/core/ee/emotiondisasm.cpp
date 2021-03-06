#include <iomanip>
#include <sstream>
#include "emotion.hpp"
#include "emotiondisasm.hpp"

using namespace std;

#define RS ((instruction >> 21) & 0x1F)
#define RT ((instruction >> 16) & 0x1F)
#define RD ((instruction >> 11) & 0x1F)
#define SA ((instruction >> 6 ) & 0x1F)
#define IMM ((int16_t)(instruction & 0xFFFF))

string EmotionDisasm::disasm_instr(uint32_t instruction, uint32_t instr_addr)
{
    if (!instruction)
        return "nop";
    switch (instruction >> 26)
    {
        case 0x00:
            return disasm_special(instruction);
        case 0x01:
            return disasm_regimm(instruction, instr_addr);
        case 0x02:
            return disasm_j(instruction, instr_addr);
        case 0x03:
            return disasm_jal(instruction, instr_addr);
        case 0x04:
            return disasm_beq(instruction, instr_addr);
        case 0x05:
            return disasm_bne(instruction, instr_addr);
        case 0x06:
            return disasm_blez(instruction, instr_addr);
        case 0x07:
            return disasm_bgtz(instruction, instr_addr);
        case 0x08:
            return disasm_addi(instruction);
        case 0x09:
            return disasm_addiu(instruction);
        case 0x0A:
            return disasm_slti(instruction);
        case 0x0B:
            return disasm_sltiu(instruction);
        case 0x0C:
            return disasm_andi(instruction);
        case 0x0D:
            return disasm_ori(instruction);
        case 0x0E:
            return disasm_xori(instruction);
        case 0x0F:
            return disasm_lui(instruction);
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            return disasm_cop(instruction, instr_addr);
        case 0x14:
            return disasm_beql(instruction, instr_addr);
        case 0x15:
            return disasm_bnel(instruction, instr_addr);
        case 0x16:
            return disasm_branch_inequality("blezl", instruction, instr_addr);
        case 0x19:
            return disasm_daddiu(instruction);
        case 0x1A:
            return disasm_ldl(instruction);
        case 0x1B:
            return disasm_ldr(instruction);
        case 0x1C:
            return disasm_mmi(instruction, instr_addr);
        case 0x1E:
            return disasm_lq(instruction);
        case 0x1F:
            return disasm_sq(instruction);
        case 0x20:
            return disasm_lb(instruction);
        case 0x21:
            return disasm_lh(instruction);
        case 0x22:
            return disasm_lwl(instruction);
        case 0x23:
            return disasm_lw(instruction);
        case 0x24:
            return disasm_lbu(instruction);
        case 0x25:
            return disasm_lhu(instruction);
        case 0x26:
            return disasm_lwr(instruction);
        case 0x27:
            return disasm_lwu(instruction);
        case 0x28:
            return disasm_sb(instruction);
        case 0x29:
            return disasm_sh(instruction);
        case 0x2A:
            return disasm_swl(instruction);
        case 0x2B:
            return disasm_sw(instruction);
        case 0x2C:
            return disasm_sdl(instruction);
        case 0x2D:
            return disasm_sdr(instruction);
        case 0x2E:
            return disasm_swr(instruction);
        case 0x2F:
            return "cache";
        case 0x30:
            return disasm_loadstore("lwc0", instruction);
        case 0x31:
            return disasm_lwc1(instruction);
        case 0x36:
            return "TODO: lqc2";
        case 0x37:
            return disasm_ld(instruction);
        case 0x39:
            return disasm_swc1(instruction);
        case 0x3E:
            return "TODO: sqc2";
        case 0x3F:
            return disasm_sd(instruction);
        default:
            return unknown_op("normal", instruction >> 26, 2);
    }
}


string EmotionDisasm::disasm_j(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_jump("j", instruction, instr_addr);
}

string EmotionDisasm::disasm_jal(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_jump("jal", instruction, instr_addr);
}


string EmotionDisasm::disasm_jump(const string opcode, uint32_t instruction, uint32_t instr_addr)
{
    stringstream output;
    uint32_t addr = (instruction & 0x3FFFFFF) << 2;
    addr += (instr_addr + 4) & 0xF0000000;
    output << "$" << setfill('0') << setw(8) << hex << addr;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_bne(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_equality("bne", instruction, instr_addr);
}


string EmotionDisasm::disasm_branch_equality(string opcode, uint32_t instruction, uint32_t instr_addr)
{
    stringstream output;
    int offset = IMM;
    offset <<=2;
    uint64_t rs = RS;
    uint64_t rt = RT;

    output << EmotionEngine::REG(rs) << ", ";
    if (!rt)
        opcode += "z";
    else
        output << EmotionEngine::REG(rt) << ", ";
    output << "$" << setfill('0') << setw(8) << hex << (instr_addr + offset + 4);

    return opcode + " " + output.str();
}



string EmotionDisasm::disasm_addiu(uint32_t instruction)
{
    return disasm_math("addiu", instruction);
}

string EmotionDisasm::disasm_regimm(uint32_t instruction, uint32_t instr_addr)
{
    stringstream output;
    string opcode = "";
    switch (RT)
    {
        case 0x00:
            opcode += "bltz";
            break;
        case 0x01:
            opcode += "bgez";
            break;
        case 0x02:
            opcode += "bltzl";
            break;
        case 0x03:
            opcode += "bgezl";
            break;
        case 0x19:
            return disasm_mtsah(instruction);
        default:
            return unknown_op("regimm", RT, 2);
    }
    int32_t offset = IMM;
    offset <<= 2;
    output << EmotionEngine::REG(RS) << ", "
           << "$" << setfill('0') << setw(8) << hex << (instr_addr + offset + 4);

    return opcode + " " + output.str();

}

string EmotionDisasm::disasm_mtsah(uint32_t instruction)
{
    stringstream output;
    output << "mtsah " << EmotionEngine::REG(RS) << ", " << (uint16_t)IMM;
    return output.str();
}

string EmotionDisasm::disasm_special(uint32_t instruction)
{
    switch (instruction & 0x3F)
    {
        case 0x00:
            return disasm_sll(instruction);
        case 0x02:
            return disasm_srl(instruction);
        case 0x03:
            return disasm_sra(instruction);
        case 0x04:
            return disasm_sllv(instruction);
        case 0x06:
            return disasm_srlv(instruction);
        case 0x07:
            return disasm_srav(instruction);
        case 0x08:
            return disasm_jr(instruction);
        case 0x09:
            return disasm_jalr(instruction);
        case 0x0A:
            return disasm_movz(instruction);
        case 0x0B:
            return disasm_movn(instruction);
        case 0x0C:
            return disasm_syscall_ee(instruction);
        case 0x0F:
            return "sync";
        case 0x10:
            return disasm_mfhi(instruction);
        case 0x11:
            return disasm_mthi(instruction);
        case 0x12:
            return disasm_mflo(instruction);
        case 0x13:
            return disasm_mtlo(instruction);
        case 0x14:
            return disasm_dsllv(instruction);
        case 0x16:
            return disasm_dsrlv(instruction);
        case 0x17:
            return disasm_dsrav(instruction);
        case 0x18:
            return disasm_mult(instruction);
        case 0x19:
            return disasm_multu(instruction);
        case 0x1A:
            return disasm_div(instruction);
        case 0x1B:
            return disasm_divu(instruction);
        case 0x20:
            return disasm_add(instruction);
        case 0x21:
            return disasm_addu(instruction);
        case 0x22:
            return disasm_sub(instruction);
        case 0x23:
            return disasm_subu(instruction);
        case 0x24:
            return disasm_and_ee(instruction);
        case 0x25:
            return disasm_or_ee(instruction);
        case 0x26:
            return disasm_xor_ee(instruction);
        case 0x27:
            return disasm_nor(instruction);
        case 0x28:
            return disasm_mfsa(instruction);
        case 0x29:
            return disasm_mtsa(instruction);
        case 0x2A:
            return disasm_slt(instruction);
        case 0x2B:
            return disasm_sltu(instruction);
        case 0x2C:
            return disasm_dadd(instruction);
        case 0x2D:
            return disasm_daddu(instruction);
        case 0x2F:
            return disasm_dsubu(instruction);
        case 0x38:
            return disasm_dsll(instruction);
        case 0x3A:
            return disasm_dsrl(instruction);
        case 0x3C:
            return disasm_dsll32(instruction);
        case 0x3E:
            return disasm_dsrl32(instruction);
        case 0x3F:
            return disasm_dsra32(instruction);
        default:
            return unknown_op("special", instruction & 0x3F, 2);
    }
}

string EmotionDisasm::disasm_sll(uint32_t instruction)
{
    return disasm_special_shift("sll", instruction);
}

string EmotionDisasm::disasm_srl(uint32_t instruction)
{
    return disasm_special_shift("srl", instruction);
}

string EmotionDisasm::disasm_sra(uint32_t instruction)
{
    return disasm_special_shift("sra", instruction);
}

string EmotionDisasm::disasm_variableshift(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RT) << ", "
           << EmotionEngine::REG(RS);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_sllv(uint32_t instruction)
{
    return disasm_variableshift("sllv", instruction);
}

string EmotionDisasm::disasm_srlv(uint32_t instruction)
{
    return disasm_variableshift("srlv", instruction);
}

string EmotionDisasm::disasm_srav(uint32_t instruction)
{
    return disasm_variableshift("srav", instruction);
}

string EmotionDisasm::disasm_jr(uint32_t instruction)
{
    stringstream output;
    string opcode = "jr";
    output << EmotionEngine::REG(RS);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_jalr(uint32_t instruction)
{
    stringstream output;
    string opcode = "jalr";
    if (RD != 31)
        output << EmotionEngine::REG(RD) << ", ";
    output << EmotionEngine::REG(RS);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_conditional_move(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RS) << ", "
           << EmotionEngine::REG(RT);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_movz(uint32_t instruction)
{
    return disasm_conditional_move("movz", instruction);
}

string EmotionDisasm::disasm_movn(uint32_t instruction)
{
    return disasm_conditional_move("movn", instruction);
}

string EmotionDisasm::disasm_syscall_ee(uint32_t instruction)
{
    stringstream output;
    string opcode = "syscall";
    uint32_t code = (instruction >> 6) & 0xFFFFF;
    output << "$" << setfill('0') << setw(8) << hex << code;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_movereg(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD);
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_mfhi(uint32_t instruction)
{
    return disasm_movereg("mfhi", instruction);
}

string EmotionDisasm::disasm_moveto(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RS);
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_mthi(uint32_t instruction)
{
    return disasm_moveto("mthi", instruction);
}

string EmotionDisasm::disasm_mflo(uint32_t instruction)
{
    return disasm_movereg("mflo", instruction);
}

string EmotionDisasm::disasm_mtlo(uint32_t instruction)
{
    return disasm_moveto("mtlo", instruction);
}

string EmotionDisasm::disasm_dsllv(uint32_t instruction)
{
    return disasm_variableshift("dsllv", instruction);
}

string EmotionDisasm::disasm_dsrlv(uint32_t instruction)
{
    return disasm_variableshift("dsrlv", instruction);
}

string EmotionDisasm::disasm_dsrav(uint32_t instruction)
{
    return disasm_variableshift("dsrav", instruction);
}

string EmotionDisasm::disasm_mult(uint32_t instruction)
{
    return disasm_special_simplemath("mult", instruction);
}

string EmotionDisasm::disasm_multu(uint32_t instruction)
{
    return disasm_special_simplemath("multu", instruction);
}

string EmotionDisasm::disasm_division(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RS) << ", "
           << EmotionEngine::REG(RT);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_div(uint32_t instruction)
{
    return disasm_division("div", instruction);
}

string EmotionDisasm::disasm_divu(uint32_t instruction)
{
    return disasm_division("divu", instruction);
}

string EmotionDisasm::disasm_special_simplemath(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RS) << ", "
           << EmotionEngine::REG(RT);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_add(uint32_t instruction)
{
    return disasm_special_simplemath("add", instruction);
}

string EmotionDisasm::disasm_addu(uint32_t instruction)
{
    return disasm_special_simplemath("addu", instruction);
}

string EmotionDisasm::disasm_sub(uint32_t instruction)
{
    return disasm_special_simplemath("add", instruction);
}

string EmotionDisasm::disasm_subu(uint32_t instruction)
{
    return disasm_special_simplemath("subu", instruction);
}

string EmotionDisasm::disasm_and_ee(uint32_t instruction)
{
    return disasm_special_simplemath("and", instruction);
}

string EmotionDisasm::disasm_or_ee(uint32_t instruction)
{
    return disasm_special_simplemath("or", instruction);
}

string EmotionDisasm::disasm_xor_ee(uint32_t instruction)
{
    return disasm_special_simplemath("xor", instruction);
}

string EmotionDisasm::disasm_nor(uint32_t instruction)
{
    return disasm_special_simplemath("nor", instruction);
}

string EmotionDisasm::disasm_mfsa(uint32_t instruction)
{
    return disasm_movereg("mfsa", instruction);
}

string EmotionDisasm::disasm_mtsa(uint32_t instruction)
{
    return disasm_moveto("mtsa", instruction);
}

string EmotionDisasm::disasm_slt(uint32_t instruction)
{
    return disasm_special_simplemath("slt", instruction);
}

string EmotionDisasm::disasm_sltu(uint32_t instruction)
{
    return disasm_special_simplemath("sltu", instruction);
}

string EmotionDisasm::disasm_dadd(uint32_t instruction)
{
    return disasm_special_simplemath("dadd", instruction);
}

string EmotionDisasm::disasm_daddu(uint32_t instruction)
{
    if (RT == 0)
        return disasm_move(instruction);
    return disasm_special_simplemath("daddu", instruction);
}

string EmotionDisasm::disasm_dsubu(uint32_t instruction)
{
    return disasm_special_simplemath("dsubu", instruction);
}

string EmotionDisasm::disasm_special_shift(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RT) << ", "
           << SA;
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_dsll(uint32_t instruction)
{
    return disasm_special_shift("dsll", instruction);
}

string EmotionDisasm::disasm_dsrl(uint32_t instruction)
{
    return disasm_special_shift("dsrl", instruction);
}

string EmotionDisasm::disasm_dsll32(uint32_t instruction)
{
    return disasm_special_shift("dsll32", instruction);
}

string EmotionDisasm::disasm_dsrl32(uint32_t instruction)
{
    return disasm_special_shift("dsrl32", instruction);
}

string EmotionDisasm::disasm_dsra32(uint32_t instruction)
{
    return disasm_special_shift("dsra32", instruction);
}


string EmotionDisasm::disasm_beq(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_equality("beq", instruction, instr_addr);
}


string EmotionDisasm::disasm_branch_inequality(const std::string opcode, uint32_t instruction, uint32_t instr_addr)
{
    stringstream output;
    int32_t offset = IMM;
    offset <<= 2;

    output << EmotionEngine::REG(RS) << ", "
           << "$" << setfill('0') << setw(8) << hex << (instr_addr + offset + 4);

    return opcode + " " + output.str();

}

string EmotionDisasm::disasm_blez(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_inequality("blez", instruction, instr_addr);
}

string EmotionDisasm::disasm_bgtz(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_inequality("bgtz", instruction, instr_addr);
}

string EmotionDisasm::disasm_math(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RT) << ", " << EmotionEngine::REG(RS) << ", "
           << "$" << setfill('0') << setw(4) << hex << IMM;
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_addi(uint32_t instruction)
{
    return disasm_math("addi", instruction);
}

string EmotionDisasm::disasm_slti(uint32_t instruction)
{
    return disasm_math("slti", instruction);
}

string EmotionDisasm::disasm_sltiu(uint32_t instruction)
{
    return disasm_math("sltiu", instruction);
}

string EmotionDisasm::disasm_andi(uint32_t instruction)
{
    return disasm_math("andi", instruction);
}

string EmotionDisasm::disasm_ori(uint32_t instruction)
{
    if (IMM == 0)
        return disasm_move(instruction);
    return disasm_math("ori", instruction);
}

string EmotionDisasm::disasm_move(uint32_t instruction)
{
    stringstream output;
    string opcode = "move";
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RS);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_xori(uint32_t instruction)
{
    return disasm_math("xori", instruction);
}

string EmotionDisasm::disasm_lui(uint32_t instruction)
{
    stringstream output;
    string opcode = "lui";
    output << EmotionEngine::REG(RT) << ", "
           << "$" << setfill('0') << setw(4) << hex << IMM;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_beql(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_equality("beql", instruction, instr_addr);
}

string EmotionDisasm::disasm_bnel(uint32_t instruction, uint32_t instr_addr)
{
    return disasm_branch_equality("bnel", instruction, instr_addr);
}

string EmotionDisasm::disasm_daddiu(uint32_t instruction)
{
    return disasm_math("daddiu", instruction);
}

string EmotionDisasm::disasm_loadstore(const std::string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RT) << ", "
           << IMM
           << "{" << EmotionEngine::REG(RS) << "}";
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_ldl(uint32_t instruction)
{
    return disasm_loadstore("ldl", instruction);
}

string EmotionDisasm::disasm_ldr(uint32_t instruction)
{
    return disasm_loadstore("ldr", instruction);
}

string EmotionDisasm::disasm_lq(uint32_t instruction)
{
    return disasm_loadstore("lq", instruction);
}

string EmotionDisasm::disasm_sq(uint32_t instruction)
{
    return disasm_loadstore("sq", instruction);
}

string EmotionDisasm::disasm_lb(uint32_t instruction)
{
    return disasm_loadstore("lb", instruction);
}

string EmotionDisasm::disasm_lh(uint32_t instruction)
{
    return disasm_loadstore("lh", instruction);
}

string EmotionDisasm::disasm_lwl(uint32_t instruction)
{
    return disasm_loadstore("lwl", instruction);
}

string EmotionDisasm::disasm_lw(uint32_t instruction)
{
    return disasm_loadstore("lw", instruction);
}

string EmotionDisasm::disasm_lbu(uint32_t instruction)
{
    return disasm_loadstore("lbu", instruction);
}

string EmotionDisasm::disasm_lhu(uint32_t instruction)
{
    return disasm_loadstore("lhu", instruction);
}

string EmotionDisasm::disasm_lwr(uint32_t instruction)
{
    return disasm_loadstore("lwr", instruction);
}

string EmotionDisasm::disasm_lwu(uint32_t instruction)
{
    return disasm_loadstore("lwu", instruction);
}

string EmotionDisasm::disasm_sb(uint32_t instruction)
{
    return disasm_loadstore("sb", instruction);
}

string EmotionDisasm::disasm_sh(uint32_t instruction)
{
    return disasm_loadstore("sh", instruction);
}

string EmotionDisasm::disasm_swl(uint32_t instruction)
{
    return disasm_loadstore("swl", instruction);
}

string EmotionDisasm::disasm_sw(uint32_t instruction)
{
    return disasm_loadstore("sw", instruction);
}

string EmotionDisasm::disasm_sdl(uint32_t instruction)
{
    return disasm_loadstore("sdl", instruction);
}

string EmotionDisasm::disasm_sdr(uint32_t instruction)
{
    return disasm_loadstore("sdr", instruction);
}

string EmotionDisasm::disasm_swr(uint32_t instruction)
{
    return disasm_loadstore("swr", instruction);
}

string EmotionDisasm::disasm_lwc1(uint32_t instruction)
{
    return disasm_loadstore("lwc1", instruction);
}

string EmotionDisasm::disasm_ld(uint32_t instruction)
{
    return disasm_loadstore("ld", instruction);
}

string EmotionDisasm::disasm_swc1(uint32_t instruction)
{
    return disasm_loadstore("swc1", instruction);
}

string EmotionDisasm::disasm_sd(uint32_t instruction)
{
    return disasm_loadstore("sd", instruction);
}

string EmotionDisasm::disasm_cop(uint32_t instruction, uint32_t instr_addr)
{
    uint16_t op = RS;
    uint8_t cop_id = ((instruction >> 26) & 0x3);
    switch (op | (cop_id * 0x100))
    {
        case 0x000:
        case 0x100:
            return disasm_cop_mfc(instruction);
        case 0x004:
        case 0x104:
            return disasm_cop_mtc(instruction);
        case 0x010:
        {
            uint8_t op2 = instruction & 0x3F;
            switch (op2)
            {
                case 0x2:
                    return "tlbwi";
                case 0x18:
                    return "eret";
                case 0x38:
                    return "ei";
                case 0x39:
                    return "di";
                default:
                    return unknown_op("cop0x010", op2, 2);

            }
        }
        case 0x106:
        case 0x206:
            return disasm_cop_ctc(instruction);
        case 0x108:
            return disasm_cop_bc1(instruction, instr_addr);
        case 0x110:
            return disasm_cop_s(instruction);
        case 0x114:
            return disasm_cop_cvt_s_w(instruction);
        case 0x102:
        case 0x202:
            return disasm_cop_cfc(instruction);
        default:
            if (cop_id == 2)
                return disasm_cop2(instruction);
            return unknown_op("cop", op, 2);
    }
}

string EmotionDisasm::disasm_cop_move(string opcode, uint32_t instruction)
{
    stringstream output;

    int cop_id = (instruction >> 26) & 0x3;

    output << opcode << cop_id << " " << EmotionEngine::REG(RT) << ", "
           << RD;

    return output.str();
}

string EmotionDisasm::disasm_cop_mfc(uint32_t instruction)
{
    return disasm_cop_move("mfc", instruction);
}

string EmotionDisasm::disasm_cop_mtc(uint32_t instruction)
{
    return disasm_cop_move("mtc", instruction);
}

string EmotionDisasm::disasm_cop_cfc(uint32_t instruction)
{
    return disasm_cop_move("cfc", instruction);
}

string EmotionDisasm::disasm_cop_ctc(uint32_t instruction)
{
    return disasm_cop_move("ctc", instruction);
}

string EmotionDisasm::disasm_cop_s(uint32_t instruction)
{
    uint8_t op = instruction & 0x3F;
    switch (op)
    {
        case 0x0:
            return disasm_fpu_add(instruction);
        case 0x1:
            return disasm_fpu_sub(instruction);
        case 0x2:
            return disasm_fpu_mul(instruction);
        case 0x3:
            return disasm_fpu_div(instruction);
        case 0x6:
            return disasm_fpu_mov(instruction);
        case 0x7:
            return disasm_fpu_neg(instruction);
        case 0x18:
            return disasm_fpu_adda(instruction);
        case 0x1C:
            return disasm_fpu_madd(instruction);
        case 0x24:
            return disasm_fpu_cvt_w_s(instruction);
        case 0x32:
            return disasm_fpu_c_eq_s(instruction);
        case 0x34:
            return disasm_fpu_c_lt_s(instruction);
        default:
            return unknown_op("FPU-S", op, 2);
    }
}

string EmotionDisasm::disasm_fpu_math(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << "f" << SA << ", "
           << "f" << RD << ", "
           << "f" << RT;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_fpu_add(uint32_t instruction)
{
    return disasm_fpu_math("add.s", instruction);
}

string EmotionDisasm::disasm_fpu_sub(uint32_t instruction)
{
    return disasm_fpu_math("sub.s", instruction);
}

string EmotionDisasm::disasm_fpu_mul(uint32_t instruction)
{
    return disasm_fpu_math("mul.s", instruction);
}

string EmotionDisasm::disasm_fpu_div(uint32_t instruction)
{
    return disasm_fpu_math("div.s", instruction);
}

string EmotionDisasm::disasm_fpu_singleop_math(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << "f" << SA << ", "
           << "f" << RD;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_fpu_mov(uint32_t instruction)
{
    return disasm_fpu_singleop_math("mov.s", instruction);
}

string EmotionDisasm::disasm_fpu_neg(uint32_t instruction)
{
    return disasm_fpu_singleop_math("neg.s", instruction);
}

string EmotionDisasm::disasm_fpu_acc(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << "f" << RD << ", "
           << "f" << RT;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_fpu_adda(uint32_t instruction)
{
    return disasm_fpu_acc("adda.s", instruction);
}

string EmotionDisasm::disasm_fpu_madd(uint32_t instruction)
{
    return disasm_fpu_math("madd.s", instruction);
}

string EmotionDisasm::disasm_fpu_convert(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << "f" << SA << ", "
           << "f" << RD;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_fpu_cvt_w_s(uint32_t instruction)
{
    return disasm_fpu_convert("cvt.w.s", instruction);
}

string EmotionDisasm::disasm_fpu_compare(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << "f" << RD << ", "
           << "f" << RT;

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_fpu_c_lt_s(uint32_t instruction)
{
    return disasm_fpu_compare("c.lt.s", instruction);
}

string EmotionDisasm::disasm_fpu_c_eq_s(uint32_t instruction)
{
    return disasm_fpu_compare("c.eq.s", instruction);
}

string EmotionDisasm::disasm_cop_bc1(uint32_t instruction, uint32_t instr_addr)
{

    stringstream output;
    string opcode = "";
    const static char* ops[] = {"bc1f", "bc1fl", "bc1t", "bc1tl"};
    int32_t offset = IMM << 2;
    uint8_t op = RT;
    if (op > 3)
        return unknown_op("BC1", op, 2);
    opcode = ops[op];
    output << "$" << setfill('0') << setw(8) << hex << (instr_addr + 4 + offset);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_cop_cvt_s_w(uint32_t instruction)
{
    return disasm_fpu_convert("cvt.s.w", instruction);
}

string EmotionDisasm::get_dest_field(uint8_t field)
{
    const static char vectors[] = {'x', 'y', 'z', 'w'};
    string out;
    for (int i = 0; i < 4; i++)
    {
        if (field & (1 << i))
            out += vectors[i];
    }
    return out;
}

string EmotionDisasm::disasm_cop2(uint32_t instruction)
{
    uint8_t op = RS;
    if (op >= 0x10)
        return disasm_cop2_special(instruction);
    switch (op)
    {
        case 0x1:
            return disasm_qmfc2(instruction);
        default:
            return unknown_op("cop2", op, 2);
    }
}

string EmotionDisasm::disasm_qmfc2(uint32_t instruction)
{
    stringstream output;
    output << "qmfc2";
    if (instruction & 1)
        output << ".i";
    output << " " << EmotionEngine::REG(RT) << ", vf" << ((instruction >> 11) & 0x1F);
    return output.str();
}

string EmotionDisasm::disasm_cop2_special(uint32_t instruction)
{
    uint8_t op = instruction & 0x3F;
    if (op >= 0x3C)
        return disasm_cop2_special2(instruction);
    switch (op)
    {
        case 0x2C:
            return disasm_vsub(instruction);
        default:
            return unknown_op("cop2 special", op, 2);
    }
}

string EmotionDisasm::disasm_vsub(uint32_t instruction)
{
    stringstream output;
    uint32_t fd = (instruction >> 6) & 0x1F;
    uint32_t fs = (instruction >> 11) & 0x1F;
    uint32_t ft = (instruction >> 16) & 0x1F;
    uint8_t dest_field = (instruction >> 21) & 0xF;
    string field = "." + get_dest_field(dest_field);
    output << "vsub" << field;
    output << " vf" << fd;
    output << ", vf" << fs;
    output << ", vf" << ft;
    return output.str();
}

string EmotionDisasm::disasm_cop2_special2(uint32_t instruction)
{
    uint16_t op = (instruction & 0x3) | ((instruction >> 4) & 0x7C);
    switch (op)
    {
        case 0x3F:
            return disasm_viswr(instruction);
        default:
            return unknown_op("cop2 special2", op, 2);
    }
}

string EmotionDisasm::disasm_viswr(uint32_t instruction)
{
    stringstream output;
    uint32_t is = (instruction >> 11) & 0x1F;
    uint32_t it = (instruction >> 16) & 0x1F;
    uint8_t dest_field = (instruction >> 21) & 0xF;
    output << "viswr." << get_dest_field(dest_field);
    output << " " << "vi" << it << ", (vi" << is << ")";
    return output.str();
}

string EmotionDisasm::disasm_mmi_copy(const string opcode, uint32_t instruction)
{
    stringstream output;
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RT);

    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_mmi(uint32_t instruction, uint32_t instr_addr)
{
    int op = instruction & 0x3F;
    switch (op)
    {
        case 0x04:
            return disasm_plzcw(instruction);
        case 0x08:
            return disasm_mmi0(instruction);
        case 0x09:
            return disasm_mmi2(instruction);
        case 0x10:
            return disasm_mfhi1(instruction);
        case 0x11:
            return disasm_mthi1(instruction);
        case 0x12:
            return disasm_mflo1(instruction);
        case 0x13:
            return disasm_mtlo1(instruction);
        case 0x18:
            return disasm_mult1(instruction);
        case 0x1A:
            return disasm_div1(instruction);
        case 0x1B:
            return disasm_divu1(instruction);
        case 0x28:
            return disasm_mmi1(instruction);
        case 0x29:
            return disasm_mmi3(instruction);
        default:
            return unknown_op("mmi", op, 2);
    }
}

string EmotionDisasm::disasm_plzcw(uint32_t instruction)
{
    stringstream output;
    string opcode = "plzcw";
    output << EmotionEngine::REG(RD) << ", "
           << EmotionEngine::REG(RS);
    return opcode + " " + output.str();
}

string EmotionDisasm::disasm_mmi0(uint32_t instruction)
{
    int op = (instruction >> 6) & 0x1F;
    switch (op)
    {
        case 0x09:
            return disasm_psubb(instruction);
        case 0x12:
            return disasm_special_simplemath("pcgtb", instruction);
        default:
            return unknown_op("mmi0", op, 2);
    }
}

string EmotionDisasm::disasm_psubb(uint32_t instruction)
{
    return disasm_special_simplemath("psubb", instruction);
}

string EmotionDisasm::disasm_mmi1(uint32_t instruction)
{
    uint8_t op = (instruction >> 6) & 0x1F;
    switch (op)
    {
        case 0x10:
            return disasm_special_simplemath("padduw", instruction);
        default:
            return unknown_op("mmi1", op, 2);
    }
}

string EmotionDisasm::disasm_mmi2(uint32_t instruction)
{
    int op = (instruction >> 6) & 0x1F;
    switch (op)
    {
        case 0x0E:
            return disasm_pcpyld(instruction);
        case 0x12:
            return disasm_pand(instruction);
        default:
            return unknown_op("mmi2", op, 2);
    }
}

string EmotionDisasm::disasm_pcpyld(uint32_t instruction)
{
    return disasm_special_simplemath("pcpyld", instruction);
}

string EmotionDisasm::disasm_pand(uint32_t instruction)
{
    return disasm_special_simplemath("pand", instruction);
}

string EmotionDisasm::disasm_mfhi1(uint32_t instruction)
{
    return disasm_movereg("mfhi1", instruction);
}

string EmotionDisasm::disasm_mthi1(uint32_t instruction)
{
    return disasm_moveto("mthi1", instruction);
}

string EmotionDisasm::disasm_mflo1(uint32_t instruction)
{
    return disasm_movereg("mflo1", instruction);
}

string EmotionDisasm::disasm_mtlo1(uint32_t instruction)
{
    return disasm_moveto("mtlo1", instruction);
}

string EmotionDisasm::disasm_mult1(uint32_t instruction)
{
    return disasm_special_simplemath("mult1", instruction);
}

string EmotionDisasm::disasm_div1(uint32_t instruction)
{
    return disasm_division("div1", instruction);
}

string EmotionDisasm::disasm_divu1(uint32_t instruction)
{
    return disasm_division("divu1", instruction);
}

string EmotionDisasm::disasm_mmi3(uint32_t instruction)
{
    switch (SA)
    {
        case 0x0E:
            return disasm_pcpyud(instruction);
        case 0x12:
            return disasm_por(instruction);
        case 0x13:
            return disasm_pnor(instruction);
        case 0x1B:
            return disasm_mmi_copy("pcpyh", instruction);
        default:
            return unknown_op("mmi3", SA, 2);
    }
}

string EmotionDisasm::disasm_pcpyud(uint32_t instruction)
{
    return disasm_special_simplemath("pcpyud", instruction);
}

string EmotionDisasm::disasm_por(uint32_t instruction)
{
    return disasm_special_simplemath("por", instruction);
}

string EmotionDisasm::disasm_pnor(uint32_t instruction)
{
    return disasm_special_simplemath("pnor", instruction);
}

string EmotionDisasm::unknown_op(const string optype, uint32_t op, int width)
{
    stringstream output;
    output << "Unrecognized " << optype << " op "
           << "$" << setfill('0') << setw(width) << hex << op;
    return output.str();
}

