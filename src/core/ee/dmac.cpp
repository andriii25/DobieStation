#include <cstdio>
#include <cstdlib>
#include "dmac.hpp"

#include "../emulator.hpp"

enum CHANNELS
{
    VIF0,
    VIF1,
    GIF,
    IPU_FROM,
    IPU_TO,
    SIF0,
    SIF1,
    SIF2,
    SPR_FROM,
    SPR_TO
};

DMAC::DMAC(EmotionEngine* cpu, Emulator* e, GraphicsInterface* gif, SubsystemInterface* sif) :
    cpu(cpu), e(e), gif(gif), sif(sif)
{

}

void DMAC::reset()
{
    master_disable = 0x1201; //hax
    control.master_enable = false;
    for (int i = 0; i < 10; i++)
    {
        channels[i].control = 0;
        interrupt_stat.channel_mask[i] = false;
        interrupt_stat.channel_stat[i] = false;
    }
    interrupt_stat.mfifo_mask = false;
    interrupt_stat.stall_mask = false;
    interrupt_stat.bus_stat = false;
    interrupt_stat.mfifo_stat = false;
    interrupt_stat.stall_stat = false;
}

void DMAC::run()
{
    if (!control.master_enable || (master_disable & (1 << 16)))
        return;
    for (int i = 0; i < 10; i++)
    {
        if (channels[i].control & 0x100)
        {
            switch (i)
            {
                case GIF:
                    process_GIF();
                    break;
                case SIF0:
                    process_SIF0();
                    break;
                case SIF1:
                    process_SIF1();
                    break;
            }
        }
    }
}

void DMAC::transfer_end(int index)
{
    printf("[DMAC] Transfer end: %d\n", index);
    channels[index].control &= ~0x100;
    interrupt_stat.channel_stat[index] = true;
    int1_check();
}

void DMAC::int1_check()
{
    bool int1_signal = false;
    for (int i = 0; i < 10; i++)
    {
        if (interrupt_stat.channel_stat[i] & interrupt_stat.channel_mask[i])
        {
            int1_signal = true;
            break;
        }
    }
    cpu->set_int1_signal(int1_signal);
}

void DMAC::process_GIF()
{
    if (channels[GIF].quadword_count)
    {
        uint64_t quad[2];
        quad[0] = e->read64(channels[GIF].address);
        quad[1] = e->read64(channels[GIF].address + 8);
        gif->send_PATH3(quad);

        channels[GIF].address += 16;
        channels[GIF].quadword_count--;
    }
    else
    {
        if (channels[GIF].tag_end)
        {
            transfer_end(GIF);
        }
        else
            handle_source_chain(GIF);
    }
}

void DMAC::process_SIF0()
{
    if (channels[SIF0].quadword_count)
    {
        if (sif->get_SIF0_size() >= 4)
        {
            for (int i = 0; i < 4; i++)
            {
                uint32_t word = sif->read_SIF0();
                printf("[DMAC] Read from SIF0: $%08X\n", word);
                e->write32(channels[SIF0].address, word);
                channels[SIF0].address += 4;
            }
            channels[SIF0].quadword_count--;
        }
    }
    else
    {
        if (channels[SIF0].tag_end)
        {
            transfer_end(SIF0);
        }
        else if (sif->get_SIF0_size() >= 2)
        {
            uint64_t DMAtag = sif->read_SIF0();
            DMAtag |= (uint64_t)sif->read_SIF0() << 32;
            printf("[DMAC] SIF0 tag: $%08X_%08X\n", DMAtag >> 32, DMAtag);

            channels[SIF0].quadword_count = DMAtag & 0xFFFF;
            channels[SIF0].address = DMAtag >> 32;
            channels[SIF0].tag_address += 16;

            int mode = (DMAtag >> 28) & 0x7;

            bool IRQ = (DMAtag & (1UL << 31));
            bool TIE = channels[SIF0].control & (1 << 7);
            if (mode == 7 || (IRQ && TIE))
                channels[SIF0].tag_end = true;

            channels[SIF0].control &= 0xFFFF;
            channels[SIF0].control |= DMAtag & 0xFFFF0000;
        }
    }
}

void DMAC::process_SIF1()
{
    if (channels[SIF1].quadword_count)
    {
        if (sif->get_SIF1_size() <= SubsystemInterface::MAX_FIFO_SIZE - 4)
        {
            uint64_t quad[2];
            quad[0] = e->read64(channels[SIF1].address);
            quad[1] = e->read64(channels[SIF1].address + 8);
            sif->write_SIF1(quad);

            channels[SIF1].address += 16;
            channels[SIF1].quadword_count--;
        }
    }
    else
    {
        if (channels[SIF1].tag_end)
        {
            transfer_end(SIF1);
        }
        else
            handle_source_chain(SIF1);
    }
}

void DMAC::handle_source_chain(int index)
{
    uint64_t DMAtag = e->read64(channels[index].tag_address);
    printf("[DMAC] Source DMAtag read $%08X: $%08X_%08X\n", channels[index].tag_address, DMAtag >> 32, DMAtag & 0xFFFFFFFF);

    //Change CTRL to have the upper 16 bits equal to bits 16-31 of the most recently read DMAtag
    channels[index].control &= 0xFFFF;
    channels[index].control |= DMAtag & 0xFFFF0000;

    uint16_t quadword_count = DMAtag & 0xFFFF;
    uint8_t id = (DMAtag >> 28) & 0x7;
    uint32_t addr = (DMAtag >> 32) & 0x7FFFFFF0;
    bool IRQ_after_transfer = DMAtag & (1UL << 31);
    bool TIE = channels[index].control & (1 << 7);
    channels[index].quadword_count = quadword_count;
    switch (id)
    {
        case 0:
            //refe
            channels[index].address = addr;
            channels[index].tag_address += 16;
            channels[index].tag_end = true;
            break;
        case 1:
            //cnt
            channels[index].address = channels[index].tag_address + 16;
            channels[index].tag_address = channels[index].address + (quadword_count * 16);
            break;
        case 2:
        {
            uint32_t temp = channels[index].tag_address;
            channels[index].tag_address = addr;
            channels[index].address = temp + 16;
        }
            break;
        case 3:
            //ref
            channels[index].address = addr;
            channels[index].tag_address += 16;
            break;
        case 7:
            //end
            channels[index].address = channels[index].tag_address + 16;
            channels[index].tag_end = true;
            break;
        default:
            printf("\n[DMAC] Unrecognized source chain DMAtag id %d\n", id);
            exit(1);
    }
    if (IRQ_after_transfer && TIE)
        channels[index].tag_end = true;
    printf("New address: $%08X\n", channels[index].address);
    printf("New tag addr: $%08X\n", channels[index].tag_address);
}

void DMAC::start_DMA(int index)
{
    printf("[DMAC] D%d started: $%08X\n", index, channels[index].control);
    printf("Addr: $%08X\n", channels[index].address);
    printf("Mode: %d\n", (channels[index].control >> 2) & 0x3);
    printf("ASP: %d\n", (channels[index].control >> 4) & 0x3);
    printf("TTE: %d\n", channels[index].control & (1 << 6));
    int mode = (channels[index].control >> 2) & 0x3;
    channels[index].tag_end = (mode == 0); //always end transfers in normal mode
}

uint32_t DMAC::read_master_disable()
{
    return master_disable;
}

void DMAC::write_master_disable(uint32_t value)
{
    master_disable = value;
}

uint32_t DMAC::read32(uint32_t address)
{
    uint32_t reg = 0;
    switch (address)
    {
        case 0x1000A000:
            reg = channels[GIF].control;
            break;
        case 0x1000C000:
            reg = channels[SIF0].control;
            break;
        case 0x1000C400:
            reg = channels[SIF1].control;
            break;
        case 0x1000C420:
            reg = channels[SIF1].quadword_count;
            break;
        case 0x1000C430:
            reg = channels[SIF1].tag_address;
            break;
        case 0x1000E000:
            reg |= control.master_enable;
            reg |= control.cycle_stealing << 1;
            reg |= control.mem_drain_channel << 2;
            reg |= control.stall_source_channel << 4;
            reg |= control.stall_dest_channel << 6;
            reg |= control.release_cycle << 8;
            break;
        case 0x1000E010:
            for (int i = 0; i < 10; i++)
            {
                reg |= interrupt_stat.channel_stat[i] << i;
                reg |= interrupt_stat.channel_mask[i] << (i + 16);
            }
            reg |= interrupt_stat.stall_stat << 13;
            reg |= interrupt_stat.mfifo_stat << 14;
            reg |= interrupt_stat.bus_stat << 15;

            reg |= interrupt_stat.stall_mask << 29;
            reg |= interrupt_stat.mfifo_mask << 30;
            break;
        default:
            printf("[DMAC] Unrecognized read32 from $%08X\n", address);
            break;
    }
    return reg;
}

void DMAC::write32(uint32_t address, uint32_t value)
{
    switch (address)
    {
        case 0x1000A000:
            channels[GIF].control = value;
            if (value & 0x100)
                start_DMA(GIF);
            break;
        case 0x1000A010:
            printf("[DMAC] GIF M_ADR: $%08X\n", value);
            channels[GIF].address = value & ~0xF;
            break;
        case 0x1000A020:
            printf("[DMAC] GIF QWC: $%08X\n", value & 0xFFFF);
            channels[GIF].quadword_count = value & 0xFFFF;
            break;
        case 0x1000A030:
            printf("[DMAC] GIF T_ADR: $%08X\n", value);
            channels[GIF].tag_address = value & ~0xF;
            break;
        case 0x1000C000:
            printf("[DMAC] SIF0 CTRL: $%08X\n", value);
            channels[SIF0].control = value;
            if (value & 0x100)
                start_DMA(SIF0);
            break;
        case 0x1000C020:
            printf("[DMAC] SIF0 QWC: $%08X\n", value);
            channels[SIF0].quadword_count = value & 0xFFFF;
            break;
        case 0x1000C030:
            printf("[DMAC] SIF0 T_ADR: $%08X\n", value);
            channels[SIF0].tag_address = value & ~0xF;
            break;
        case 0x1000C400:
            printf("[DMAC] SIF1 CTRL: $%08X\n", value);
            channels[SIF1].control = value;
            if (value & 0x100)
                start_DMA(SIF1);
            break;
        case 0x1000C420:
            printf("[DMAC] SIF1 QWC: $%08X\n", value);
            channels[SIF1].quadword_count = value & 0xFFFF;
            break;
        case 0x1000C430:
            printf("[DMAC] SIF1 T_ADR: $%08X\n", value);
            channels[SIF1].tag_address = value & ~0xF;
            break;
        case 0x1000E000:
            printf("[DMAC] Write32 D_CTRL: $%08X\n", value);
            control.master_enable = value & 0x1;
            control.cycle_stealing = value & 0x2;
            control.mem_drain_channel = (value >> 2) & 0x3;
            control.stall_source_channel = (value >> 4) & 0x3;
            control.stall_dest_channel = (value >> 6) & 0x3;
            control.release_cycle = (value >> 8) & 0x7;
            break;
        case 0x1000E010:
            printf("[DMAC] Write32 D_STAT: $%08X\n", value);
            for (int i = 0; i < 10; i++)
            {
                if (value & (1 << i))
                    interrupt_stat.channel_stat[i] = false;

                //Reverse mask
                if (value & (1 << (i + 16)))
                    interrupt_stat.channel_mask[i] ^= 1;
            }

            interrupt_stat.stall_stat &= ~(value & (1 << 13));
            interrupt_stat.mfifo_stat &= ~(value & (1 << 14));
            interrupt_stat.bus_stat &= ~(value & (1 << 15));

            if (value & (1 << 29))
                interrupt_stat.stall_mask ^= 1;
            if (value & (1 << 30))
                interrupt_stat.mfifo_mask ^= 1;

            int1_check();
            break;
        default:
            printf("[DMAC] Unrecognized write32 of $%08X to $%08X\n", value, address);
            break;
    }
}
