/*
 * Task 4: Assembly Cycle-Counted Phase Delay
 * System Clock: 8 MHz HSI Oscillator (1 cycle = 125 ns)
 * Target Delay: 125 us (45 degrees phase shift for a 1 kHz sine wave)
 */

    .syntax unified
    .cpu cortex-m0plus
    .thumb

    .global DSP_Loop
    .type DSP_Loop, %function   @ Informs the linker that DSP_Loop is a function

// Peripheral base register address definitions (STM32F0 series)
.equ ADC_DR_ADDR,   0x40012440    // ADC Data Register (ADC1->DR)
.equ DAC_DHR_ADDR,  0x40007408    // DAC Channel 1 12-bit Right-Aligned Data Holding Register (DAC->DHR12R1)

    .section .text
    .align 2
    .thumb_func                   @ Required for Cortex-M: Marks target symbol as Thumb

DSP_Loop:
    // Load peripheral register addresses into registers once
    LDR R0, =ADC_DR_ADDR
    LDR R1, =DAC_DHR_ADDR

dsp_polling_loop:
    // -------------------------------------------------------------------------
    // 1. Read sampled value from ADC
    // -------------------------------------------------------------------------
    LDR R3, [R0]             // Load sample from ADC_DR into R3         [2 cycles]

    // -------------------------------------------------------------------------
    // 2. Cycle-counted software delay loop
    //    Target delay loop duration = 989 cycles
    //    Loop Body: SUBS (1 cycle) + BNE (2 cycles taken, 1 cycle untaken)
    //    Total Loop Time = (330 * 3) - 1 = 989 cycles
    // -------------------------------------------------------------------------
    LDR R4, =330             // Initialize counter N = 330              [2 cycles]

delay_loop:
    SUBS R4, R4, #1          // Decrement R4                            [1 cycle]
    BNE  delay_loop          // Branch if R4 != 0                       [2 cycles taken, 1 untaken]

    NOP                      // Pad 1 cycle                             [1 cycle]
    NOP                      // Pad 1 cycle                             [1 cycle]

    // -------------------------------------------------------------------------
    // 3. Write sample directly to DAC
    // -------------------------------------------------------------------------
    STR R3, [R1]             // Output R3 sample value to DAC_DHR12R1   [2 cycles]

    // -------------------------------------------------------------------------
    // 4. Repeat polling loop indefinitely
    // -------------------------------------------------------------------------
    B   dsp_polling_loop     // Branch back to start of loop           [3 cycles]

    // -------------------------------------------------------------------------
    // CYCLE BUDGET SUMMARY:
    //  ADC Read (LDR):              2 cycles
    //  Delay Load (LDR):            2 cycles
    //  Delay Loop:                989 cycles  ((330 * 3) - 1)
    //  NOP Padding:                 2 cycles  (1 + 1)
    //  DAC Write (STR):             2 cycles
    //  Loop Branch (B):             3 cycles
    // -------------------------------------------------------------------------
    //  TOTAL ITERATION TIME:     1000 cycles
    //  1000 cycles * 125 ns = 125.00 us (Exact 45-degree phase delay)
