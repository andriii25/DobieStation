#include <cstdio>
#include "intc.hpp"
#include "timers.hpp"

EmotionTiming::EmotionTiming(INTC* intc) : intc(intc)
{

}

void EmotionTiming::reset()
{
    for (int i = 0; i < 4; i++)
    {
        timers[i].counter = 0;
        timers[i].clocks = 0;
        timers[i].control.enabled = false;
    }
}

void EmotionTiming::run()
{
    for (int i = 0; i < 4; i++)
    {
        if (timers[i].control.enabled)
        {
            timers[i].clocks++;
            switch (timers[i].control.mode)
            {
                case 0:
                    if (timers[i].clocks >= 2)
                        count_up(i, 2);
                    break;
                case 3:
                    //TODO: actual value for HSYNC
                    if (timers[i].clocks >= 15000)
                    {
                        count_up(i, 15000);
                    }
                    break;
            }
        }
    }
}

uint32_t EmotionTiming::read32(uint32_t addr)
{
    switch (addr)
    {
        case 0x10000000:
            return timers[0].counter;
        default:
            printf("[EE Timing] Unrecognized read32 from $%08X\n", addr);
            return 0;
    }
}

void EmotionTiming::write32(uint32_t addr, uint32_t value)
{
    switch (addr)
    {
        case 0x10000010:
            write_control(0, value);
            break;
        case 0x10000810:
            write_control(1, value);
            break;
        case 0x10001010:
            write_control(2, value);
            break;
        case 0x10001810:
            write_control(3, value);
            break;
        case 0x10001820:
            printf("[EE Timing] Timer 3 compare: $%08X\n", value);
            timers[3].compare = value & 0xFFFF;
            break;
        default:
            printf("[EE Timing] Unrecognized write32 to $%08X of $%08X\n", addr, value);
            break;
    }
}

void EmotionTiming::count_up(int index, int cycles_per_count)
{
    timers[index].clocks -= cycles_per_count;
    timers[index].counter++;

    //Overflow check
    if (timers[index].counter > 0xFFFF)
    {
        if (index == 3)
            printf("[EE Timing] Timer 3 overflow!\n");
        timers[index].counter = 0;
        if (timers[index].control.overflow_int_enable)
        {
            timers[index].control.overflow_int = true;
            intc->assert_IRQ((int)Interrupt::TIMER0 + index);
        }
    }
}

void EmotionTiming::write_control(int index, uint32_t value)
{
    printf("[EE Timing] Write32 timer %d control: $%08X\n", index, value);
    timers[index].control.mode = value & 0x3;
    timers[index].control.gate_enable = value & (1 << 2);
    timers[index].control.gate_VBLANK = value & (1 << 3);
    timers[index].control.gate_mode = (value >> 4) & 0x3;
    timers[index].control.clear_on_reference = value & (1 << 6);
    timers[index].control.enabled = value & (1 << 7);
    timers[index].control.compare_int_enable = value & (1 << 8);
    timers[index].control.overflow_int_enable = value & (1 << 9);
    timers[index].control.compare_int &= ~(value & (1 << 10));
    timers[index].control.overflow_int &= ~(value & (1 << 11));
}
