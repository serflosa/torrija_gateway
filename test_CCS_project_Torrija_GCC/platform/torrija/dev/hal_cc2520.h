/***********************************************************************************
  Filename:     hal_cc2520.h

  Description:  This file defines the interface to the CC2520 radio.

***********************************************************************************/
#ifndef HALCC2520_H
#define HALCC2520_H

/***********************************************************************************
* INCLUDES
*/
#include <inttypes.h>
#include <msp430f5519.h>

/* Some general purpose definitions */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL (void *)0
#endif
/***********************************************************************************
* MACROS
*/

/*
*  This macro is for use by other macros to form a fully valid C statement.
*  Without this, the if/else conditionals could show unexpected behavior.
*
*  For example, use...
*    #define SET_REGS()  st( ioreg1 = 0; ioreg2 = 0; )
*  instead of ...
*    #define SET_REGS()  { ioreg1 = 0; ioreg2 = 0; }
*  or
*    #define  SET_REGS()    ioreg1 = 0; ioreg2 = 0;
*  The last macro would not behave as expected in the if/else construct.
*  The second to last macro will cause a compiler error in certain uses
*  of if/else construct
*
*  It is not necessary, or recommended, to use this macro where there is
*  already a valid C statement.  For example, the following is redundant...
*    #define CALL_FUNC()   st(  func();  )
*  This should simply be...
*    #define CALL_FUNC()   func()
*
* (The while condition below evaluates false without generating a
*  constant-controlling-loop type of warning on most compilers.)
*/
#define st(x)      do { x } while (__LINE__ == -1)

/* uint16_t processing */
#define HI_UINT16(a) (((uint16_t)(a) >> 8) & 0xFF)
#define LO_UINT16(a) ((uint16_t)(a) & 0xFF)
/* uint32_t processing */
#define HI_UINT32(a) ((uint16_t) (((uint32_t)(a)) >> 16))
#define LO_UINT32(a) ((uint16_t) ((uint32_t)(a)))


// MCU port control for SPI interface
#define CC2520_DISABLE_SPI_FUNC()       st( P3SEL &= ~(BV(0) | BV(1) | BV(2));
#define CC2520_ENABLE_SPI_FUNC()        st( P5SEL |= (BV(0) | BV(1) | BV(2));

// SPI register definitions
#define CC2520_SPI_TX_REG               (UCB0TXBUF)
#define CC2520_SPI_RX_REG               (UCB0RXBUF)
#define CC2520_SPI_RX_IS_READY()        (UCB0IFG & UCRXIFG)
#define CC2520_SPI_RX_NOT_READY()       (UCB0IFG &= ~UCRXIFG)

// SPI access macros
#define CC2520_SPI_BEGIN()              P2OUT &= ~BIT7 		//st( MCU_IO_SET(5,5,0); )
#define CC2520_SPI_TX(x)                st( CC2520_SPI_RX_NOT_READY(); CC2520_SPI_TX_REG = x; )
#define CC2520_SPI_RX()                 (CC2520_SPI_RX_REG)
#define CC2520_SPI_WAIT_RXRDY()         st( while (!CC2520_SPI_RX_IS_READY()); )
#define CC2520_SPI_END()                P2OUT |= BIT7 		//st( MCU_IO_SET(5,5,1); )

// Outputs: SPI interface

#define CC2520_CSN_OPIN(v)              if(v) P2OUT |= BIT7;else P2OUT &= ~BIT7;//MCU_IO_SET(5,5,v)
#define CC2520_SCLK_OPIN(v)             if(v) P3OUT |= BIT2;else P3OUT &= ~BIT2;//MCU_IO_SET(3,6,v)
#define CC2520_MOSI_OPIN(v)             if(v) P3OUT |= BIT0;else P3OUT &= ~BIT0;//MCU_IO_SET(5,6,v)


// Inputs: SPI interface
#define CC2520_MISO_IPIN                (P3IN & BIT1)
	 // For use in LPM - MCU_IO_SET(5,7,v)
#define CC2520_MISO_OPIN(v)             if(v) P3OUT |= BIT1;else P3OUT &= ~BIT1;						
#define CC2520_MISO_DIR_IN()            P3DIR |= BIT1;						// MCU_PIN_DIR_OUT(5,7)
#define CC2520_MISO_DIR_OUT()           P3DIR &= ~BIT1;	


#define RF_WAIT_TRANSCEIVER_READY()  st( WAIT_SFD_LOW();)
#define WAIT_RSSI_VALID()         st( while (!CC2520_RSSI_VALID_PIN);)
#define WAIT_SFD_HIGH()           st( while (!CC2520_SFD_PIN);)
#define WAIT_SFD_LOW()            st( while (CC2520_SFD_PIN);)
#define CLEAR_EXC_RX_FRM_DONE()   st(CC2520_CLEAR_EXC(CC2520_EXC_RX_FRM_DONE);)

#define DPU_ACTIVE_H_BM           0x08
#define WAIT_DPU_DONE_H()         st( while (CC2520_INS_STROBE(CC2520_INS_SNOP) \
    & DPU_ACTIVE_H_BM);)

// GPIO pin direction control
/****************************************************************
 * CC2520 output means MSP430 input function (PxDIR &= ~BITx)
 */
#define CC2520_GPIO_DIR_OUT(pin) \
    st( \
        if (pin == 0) P2DIR &= ~BIT0; \
            if (pin == 1) P2DIR &= ~BIT1; \
                if (pin == 2) P2DIR &= ~BIT2; \
                    if (pin == 3) P2DIR &= ~BIT3; \
                        if (pin == 4) P2DIR &= ~BIT4; \
                            if (pin == 5) P2DIR &= ~BIT5; \
                                )
// Configure a GPIO pin 'n' as output bound to the function 'fn'

 #define CC2520_CFG_GPIO_OUT(n, fn) \
    st( \
        CC2520_REGWR8(CC2520_GPIOCTRL0 + (n), (fn)); \
            CC2520_GPIO_DIR_OUT(n); \
                )
           
// Read exception map
#define CC2520_GET_EXC(dwMap) \
    CC2520_REGRD(CC2520_EXCFLAG0, 4, (uint8  *)&(dwMap))

// Read FSM state value
#define CC2520_GET_FSM_STATE() \
    (CC2520_REGRD8(CC2520_FSMSTAT0) & CC2520_FSMSTAT_FSM_STATE_BV)

// Clear exception 'n'
#define CC2520_CLEAR_EXC(n) \
    CC2520_BCLR(CC2520_MAKE_BIT_ADDR(CC2520_EXCFLAG0 + ((n >> 3)&0x3), (n) & 0x07))

// Set exception mask A or B
#define CC2520_SET_EXCMASK(m, value) (CC2520_REGWR24(CC2520_EXCMASK##m##0, value))

// Make a BIT address from register address and bit number.
#define CC2520_MAKE_BIT_ADDR(regAddr, bitIndex) (((regAddr) << 3) | (bitIndex))

/***********************************************************************************
* CONSTANTS AND DEFINES
*/

// TXPOWER values
#define CC2520_TXPOWER_MIN_4_DBM        0x88  // Gives ~ -3.1 dBm TX output power
#define CC2520_TXPOWER_0_DBM            0x32  // Gives ~ 0.3 dBm TX output power
#define CC2520_TXPOWER_4_DBM            0xF7  // Gives ~ 4.6 dBm TX output power

#define CC2520_RSSI_OFFSET                76  // dBm (from data sheet)



// CC2520 pin assignments
#define CC2520_RX_FRM_DONE_PIN              (P2IN & BIT0)//MCU_IO_GET(2,0)
#define CC2520_SAMPLED_CCA_PIN              (P2IN & BIT1)//MCU_IO_GET(2,1)
// GPIO2 are reused for RSSI Valid or TX_FRM_DONE
#define CC2520_RSSI_VALID_PIN               (P2IN & BIT2)
#define CC2520_TX_FRM_DONE_PIN              (P2IN & BIT2)
#ifdef INCLUDE_PA
	#define CC2520_SFD_PIN                      (P2IN & BIT2)
#else
	#define CC2520_SFD_PIN                      (P2IN & BIT3)
#endif
#define CC2520_GPIO_SNIFFER_DATA_PIN        (P2IN & BIT4)
#define CC2520_GPIO_SNIFFER_CLK_PIN         (P2IN & BIT5)


#define CC2520_MEMORY_SIZE                  0x400

// Startup time values (in microseconds)
#define CC2520_XOSC_MAX_STARTUP_TIME        300
#define CC2520_VREG_MAX_STARTUP_TIME        200
#define CC2520_SRXON_TO_RANDOM_READY_TIME   144

// Address definitions

// FREG definitions (BSET/BCLR supported)
#define CC2520_FRMFILT0                0x000
#define CC2520_FRMFILT1                0x001
#define CC2520_SRCMATCH                0x002
#define CC2520_SRCSHORTEN0             0x004
#define CC2520_SRCSHORTEN1             0x005
#define CC2520_SRCSHORTEN2             0x006
#define CC2520_SRCEXTEN0               0x008
#define CC2520_SRCEXTEN1               0x009
#define CC2520_SRCEXTEN2               0x00A
#define CC2520_FRMCTRL0                0x00C
#define CC2520_FRMCTRL1                0x00D
#define CC2520_RXENABLE0               0x00E
#define CC2520_RXENABLE1               0x00F
#define CC2520_EXCFLAG0                0x010
#define CC2520_EXCFLAG1                0x011
#define CC2520_EXCFLAG2                0x012
#define CC2520_EXCMASKA0               0x014
#define CC2520_EXCMASKA1               0x015
#define CC2520_EXCMASKA2               0x016
#define CC2520_EXCMASKB0               0x018
#define CC2520_EXCMASKB1               0x019
#define CC2520_EXCMASKB2               0x01A
#define CC2520_EXCBINDX0               0x01C
#define CC2520_EXCBINDX1               0x01D
#define CC2520_EXCBINDY0               0x01E
#define CC2520_EXCBINDY1               0x01F
#define CC2520_GPIOCTRL0               0x020
#define CC2520_GPIOCTRL1               0x021
#define CC2520_GPIOCTRL2               0x022
#define CC2520_GPIOCTRL3               0x023
#define CC2520_GPIOCTRL4               0x024
#define CC2520_GPIOCTRL5               0x025
#define CC2520_GPIOPOLARITY            0x026
#define CC2520_GPIOCTRL                0x028
#define CC2520_DPUCON                  0x02A
#define CC2520_DPUSTAT                 0x02C
#define CC2520_FREQCTRL                0x02E
#define CC2520_FREQTUNE                0x02F
#define CC2520_TXPOWER                 0x030
#define CC2520_TXCTRL                  0x031
#define CC2520_FSMSTAT0                0x032
#define CC2520_FSMSTAT1                0x033
#define CC2520_FIFOPCTRL               0x034
#define CC2520_FSMCTRL                 0x035
#define CC2520_CCACTRL0                0x036
#define CC2520_CCACTRL1                0x037
#define CC2520_RSSI                    0x038
#define CC2520_RSSISTAT                0x039
#define CC2520_TXFIFO_BUF              0x03A
#define CC2520_RXFIRST                 0x03C
#define CC2520_RXFIFOCNT               0x03E
#define CC2520_TXFIFOCNT               0x03F

// SREG definitions (BSET/BCLR unsupported)
#define CC2520_CHIPID                  0x040
#define CC2520_VERSION                 0x042
#define CC2520_EXTCLOCK                0x044
#define CC2520_MDMCTRL0                0x046
#define CC2520_MDMCTRL1                0x047
#define CC2520_FREQEST                 0x048
#define CC2520_RXCTRL                  0x04A
#define CC2520_FSCTRL                  0x04C
#define CC2520_FSCAL0                  0x04E
#define CC2520_FSCAL1                  0x04F
#define CC2520_FSCAL2                  0x050
#define CC2520_FSCAL3                  0x051
#define CC2520_AGCCTRL0                0x052
#define CC2520_AGCCTRL1                0x053
#define CC2520_AGCCTRL2                0x054
#define CC2520_AGCCTRL3                0x055
#define CC2520_ADCTEST0                0x056
#define CC2520_ADCTEST1                0x057
#define CC2520_ADCTEST2                0x058
#define CC2520_MDMTEST0                0x05A
#define CC2520_MDMTEST1                0x05B
#define CC2520_DACTEST0                0x05C
#define CC2520_DACTEST1                0x05D
#define CC2520_ATEST                   0x05E
#define CC2520_DACTEST2                0x05F
#define CC2520_PTEST0                  0x060
#define CC2520_PTEST1                  0x061
#define CC2520_RESERVED                0x062
#define CC2520_DPUBIST                 0x07A
#define CC2520_ACTBIST                 0x07C
#define CC2520_RAMBIST                 0x07E

// RAM size definitions
#define CC2520_RAM_START_ADDR          0x100
#define CC2520_RAM_SIZE                640

// RX and TX buffer definitions
#define CC2520_RAM_TXBUF               0x100
#define CC2520_RAM_RXBUF               0x180

// Security work buffer definitions
#define CC2520_RAM_CBCTEMPL            0x360
#define CC2520_RAM_CBCTEMPH            0x370

// Source address matching definitions
#define CC2520_RAM_SRCTABLEBASE        0x380
#define CC2520_RAM_SRCRESMASK0         0x3E0
#define CC2520_RAM_SRCRESMASK1         0x3E1
#define CC2520_RAM_SRCRESMASK2         0x3E2
#define CC2520_RAM_SRCRESINDEX         0x3E3
#define CC2520_RAM_SRCEXTPENDEN0       0x3E4
#define CC2520_RAM_SRCEXTPENDEN1       0x3E5
#define CC2520_RAM_SRCEXTPENDEN2       0x3E6
#define CC2520_RAM_SRCSHORTPENDEN0     0x3E7
#define CC2520_RAM_SRCSHORTPENDEN1     0x3E8
#define CC2520_RAM_SRCSHORTPENDEN2     0x3E9

// Local address definitions
#define CC2520_RAM_EXTADDR             0x3EA
#define CC2520_RAM_PANID               0x3F2
#define CC2520_RAM_SHORTADDR           0x3F4

// Command strobes (identifiers to be used with GPIO and exception binding)
#define CC2520_STR_SIBUFEX             0x00
#define CC2520_STR_SRXMASKBITCLR       0x01
#define CC2520_STR_SRXMASKBITSET       0x02
#define CC2520_STR_SRXON               0x03
#define CC2520_STR_SSAMPLECCA          0x04
#define CC2520_STR_SACK                0x05
#define CC2520_STR_SACKPEND            0x06
#define CC2520_STR_SNACK               0x07
#define CC2520_STR_STXON               0x08
#define CC2520_STR_STXONCCA            0x09
#define CC2520_STR_SFLUSHRX            0x0A
#define CC2520_STR_SFLUSHTX            0x0B
#define CC2520_STR_SRXFIFOPOP          0x0C
#define CC2520_STR_STXCAL              0x0D
#define CC2520_STR_SRFOFF              0x0E
#define CC2520_STR_SXOSCOFF            0x0F

// Exceptions
#define CC2520_EXC_RF_IDLE             0
#define CC2520_EXC_TX_FRM_DONE         1
#define CC2520_EXC_TX_ACK_DONE         2
#define CC2520_EXC_TX_UNDERFLOW        3
#define CC2520_EXC_TX_OVERFLOW         4
#define CC2520_EXC_RX_UNDERFLOW        5
#define CC2520_EXC_RX_OVERFLOW         6
#define CC2520_EXC_RXENABLE_ZERO       7
#define CC2520_EXC_RX_FRM_DONE         8
#define CC2520_EXC_RX_FRM_ACCEPTED     9
#define CC2520_EXC_SRC_MATCH_DONE      10
#define CC2520_EXC_SRC_MATCH_FOUND     11
#define CC2520_EXC_FIFOP               12
#define CC2520_EXC_SFD                 13
#define CC2520_EXC_DPU_DONE_L          14
#define CC2520_EXC_DPU_DONE_H          15
#define CC2520_EXC_MEMADDR_ERROR       16
#define CC2520_EXC_USAGE_ERROR         17
#define CC2520_EXC_OPERAND_ERROR       18
#define CC2520_EXC_SPI_ERROR           19
#define CC2520_EXC_RF_NO_LOCK          20
#define CC2520_EXC_RX_FRM_ABORTED      21
#define CC2520_EXC_RXBUFMOV_TIMEOUT    22

// Predefined exception channels
#define CC2520_EXC_CH_RX_BV           \
    (BV(CC2520_EXC_RX_UNDERFLOW) | BV(CC2520_EXC_RX_OVERFLOW) \
        | BV(CC2520_EXC_RX_FRM_ABORTED) | BV(CC2520_EXC_RXBUFMOV_TIMEOUT))
#define CC2520_EXC_CH_ERR_BV          \
    (BV(CC2520_EXC_MEMADDR_ERROR) | BV(CC2520_EXC_USAGE_ERROR) \
        | BV(CC2520_EXC_OPERAND_ERROR) | BV(CC2520_EXC_SPI_ERROR))

// CC2520 FSM state defintions
#define CC2520_FSM_IDLE                 0
#define CC2520_FSM_FS_RX_CAL            2
#define CC2520_FSM_RX_RESET             3
#define CC2520_FSM_RX_S_CLK             4
#define CC2520_FSM_RX_ON                5
#define CC2520_FSM_RX_W_SFD             6
#define CC2520_FSM_RD_LEN               7
#define CC2520_FSM_RX_RD_FCF0           8
#define CC2520_FSM_RX_RD_FCF1           9
#define CC2520_FSM_RX_RD_SEQ           10
#define CC2520_FSM_RX_PKT              12
#define CC2520_FSM_RX_FINAL            13
#define CC2520_FSM_RXRX_WAIT           14
#define CC2520_FSM_FIFO_RESET          16
#define CC2520_FSM_RX_OVERFLOW         17
#define CC2520_FSM_FS_LB_CAL_WAIT      20
#define CC2520_FSM_LB_RESET            21
#define CC2520_FSM_LB_S_CLK            22
#define CC2520_FSM_LB_PRE              23
#define CC2520_FSM_LB_PKT              24
#define CC2520_FSM_LB_FINAL            25
#define CC2520_FSM_RX_INF              31
#define CC2520_FSM_FS_TX_CAL           32
#define CC2520_FSM_FS_TX_CAL_WAIT      33
#define CC2520_FSM_TX_RESET            34
#define CC2520_FSM_TX_S_CLK            35
#define CC2520_FSM_TX_PRE              36
#define CC2520_FSM_LEN                 37
#define CC2520_FSM_PKT                 38
#define CC2520_FSM_TX_FINAL            39
#define CC2520_FSM_TXRX_TRANSIT        40
#define CC2520_FSM_PA_SETTLE           41
#define CC2520_FSM_TX_MOD_DOWNRAMP     42
#define CC2520_FSM_FS_ACK_CAL          48
#define CC2520_FSM_TX_ACK_RESET        49
#define CC2520_FSM_TX_ACK_S_CLK        50
#define CC2520_FSM_TX_ACK_PRE          51
#define CC2520_FSM_TX_ACK_LEN          52
#define CC2520_FSM_TX_ACK_PKT          53
#define CC2520_FSM_TX_ACK_FIN_PKT      54
#define CC2520_FSM_TX_ACK_DELAY        55
#define CC2520_FSM_TX_UNDERFLOW        56
#define CC2520_FSM_TX_TURNOFF          57
#define CC2520_FSM_TX_INF              63

// CC2520 FSMSTAT0/1 register bits
#define CC2520_FSMSTAT_FSM_STATE_BV    0x003F
#define CC2520_FSMSTAT_CAL_RUNNING_BV  0x0040
#define CC2520_FSMSTAT_CAL_DONE_BV     0x0080
#define CC2520_FSMSTAT_RX_ACTIVE_BV    0x0100
#define CC2520_FSMSTAT_TX_ACTIVE_BV    0x0200
#define CC2520_FSMSTAT_LOCK_STATUS_BV  0x0400
#define CC2520_FSMSTAT_SAMPLED_CCA_BV  0x0800
#define CC2520_FSMSTAT_CCA_BV          0x1000
#define CC2520_FSMSTAT_SFD_BV          0x2000
#define CC2520_FSMSTAT_FIFOP_BV        0x4000
#define CC2520_FSMSTAT_FIFO_BV         0x8000

// CC2520 status byte
#define CC2520_STB_XOSC_STABLE_BV      0x80
#define CC2520_STB_RSSI_VALID_BV       0x40
#define CC2520_STB_EXC_CHB_BV          0x20
#define CC2520_STB_EXC_CHA_BV          0x10
#define CC2520_STB_DPUH_ACTIVE_BV      0x08
#define CC2520_STB_DPUL_ACTIVE_BV      0x04
#define CC2520_STB_TX_ACTIVE_BV        0x02
#define CC2520_STB_RX_ACTIVE_BV        0x01

// CC2520 GPIO output definitions
#define CC2520_GPIO_EXC_CH_A           0x21
#define CC2520_GPIO_EXC_CH_B           0x22
#define CC2520_GPIO_EXC_CH_INVA        0x23
#define CC2520_GPIO_EXC_CH_INVB        0x24
#define CC2520_GPIO_EXC_CH_RX          0x25
#define CC2520_GPIO_EXC_CH_ERR         0x26
#define CC2520_GPIO_FIFO               0x27
#define CC2520_GPIO_FIFOP              0x28
#define CC2520_GPIO_CCA                0x29
#define CC2520_GPIO_SFD                0x2A
#define CC2520_GPIO_RSSI_VALID         0x2C
#define CC2520_GPIO_SAMPLED_CCA        0x2D
#define CC2520_GPIO_SNIFFER_CLK        0x31
#define CC2520_GPIO_SNIFFER_DATA       0x32
#define CC2520_GPIO_RX_ACTIVE          0x43
#define CC2520_GPIO_TX_ACTIVE          0x44
#define CC2520_GPIO_LOW                0x7E
#define CC2520_GPIO_HIGH               0x7F

// Instruction implementation
#define CC2520_INS_SNOP                0x00
#define CC2520_INS_IBUFLD              0x02
#define CC2520_INS_SIBUFEX             0x03
#define CC2520_INS_SSAMPLECCA          0x04
#define CC2520_INS_SRES                0x0F
#define CC2520_INS_MEMRD               0x10
#define CC2520_INS_MEMWR               0x20
#define CC2520_INS_RXBUF               0x30
#define CC2520_INS_RXBUFCP             0x38
#define CC2520_INS_RXBUFMOV            0x32
#define CC2520_INS_TXBUF               0x3A
#define CC2520_INS_TXBUFCP             0x3E
#define CC2520_INS_RANDOM              0x3C
#define CC2520_INS_SXOSCON             0x40
#define CC2520_INS_STXCAL              0x41
#define CC2520_INS_SRXON               0x42
#define CC2520_INS_STXON               0x43
#define CC2520_INS_STXONCCA            0x44
#define CC2520_INS_SRFOFF              0x45
#define CC2520_INS_SXOSCOFF            0x46
#define CC2520_INS_SFLUSHRX            0x47
#define CC2520_INS_SFLUSHTX            0x48
#define CC2520_INS_SACK                0x49
#define CC2520_INS_SACKPEND            0x4A
#define CC2520_INS_SNACK               0x4B
#define CC2520_INS_SRXMASKBITSET       0x4C
#define CC2520_INS_SRXMASKBITCLR       0x4D
#define CC2520_INS_RXMASKAND           0x4E
#define CC2520_INS_RXMASKOR            0x4F
#define CC2520_INS_MEMCP               0x50
#define CC2520_INS_MEMCPR              0x52
#define CC2520_INS_MEMXCP              0x54
#define CC2520_INS_MEMXWR              0x56
#define CC2520_INS_BCLR                0x58
#define CC2520_INS_BSET                0x59
#define CC2520_INS_CTR                 0x60
#define CC2520_INS_CBCMAC              0x64
#define CC2520_INS_UCBCMAC             0x66
#define CC2520_INS_CCM                 0x68
#define CC2520_INS_UCCM                0x6A
#define CC2520_INS_ECB                 0x70
#define CC2520_INS_ECBO                0x72
#define CC2520_INS_ECBX                0x74
#define CC2520_INS_ECBXO               0x76
#define CC2520_INS_INC                 0x78
#define CC2520_INS_ABORT               0x7F
#define CC2520_INS_REGRD               0x80
#define CC2520_INS_REGWR               0xC0

/***********************************************************************************
* GLOBAL FUNCTIONS
*/
// Instruction prototypes
uint8_t  CC2520_SNOP(void);
uint8_t  CC2520_SIBUFEX(void);
uint8_t  CC2520_SSAMPLECCA(void);
uint8_t  CC2520_SXOSCON(void);
uint8_t  CC2520_STXCAL(void);
uint8_t  CC2520_SRXON(void);
uint8_t  CC2520_STXON(void);
uint8_t  CC2520_STXONCCA(void);
uint8_t  CC2520_SRFOFF(void);
uint8_t  CC2520_SXOSCOFF(void);
uint8_t  CC2520_SFLUSHRX(void);
uint8_t  CC2520_SFLUSHTX(void);
uint8_t  CC2520_SACK(void);
uint8_t  CC2520_SACKPEND(void);
uint8_t  CC2520_SNACK(void);
uint8_t  CC2520_SRXMASKBITSET(void);
uint8_t  CC2520_SRXMASKBITCLR(void);
uint8_t  CC2520_IBUFLD(uint8_t i);
uint8_t  CC2520_SRES(void);
uint8_t  CC2520_MEMRD(uint16_t addr, uint16_t count, uint8_t  *pData);
uint8_t  CC2520_MEMRD8(uint16_t addr);
uint16_t CC2520_MEMRD16(uint16_t addr);
uint32_t CC2520_MEMRD24(uint16_t addr);
uint8_t  CC2520_MEMWR(uint16_t addr, uint16_t count, uint8_t  *pData);
uint8_t  CC2520_MEMWR8(uint16_t addr, uint8_t value);
uint8_t  CC2520_MEMWR16(uint16_t addr, uint16_t value);
uint8_t  CC2520_MEMWR24(uint16_t addr, uint32_t value);
uint8_t  CC2520_RXBUFCP_BEGIN(uint16_t addr, uint8_t *pCurrCount);
uint8_t  CC2520_RXBUFCP_END(uint16_t addr, uint8_t count, uint8_t  *pData);
uint8_t  CC2520_RXBUF(uint8_t count, uint8_t  *pData);
uint8_t  CC2520_RXBUF8(void);
uint16_t CC2520_RXBUF16(void);
uint8_t  CC2520_RXBUFMOV(uint8_t pri, uint16_t addr, uint8_t count, uint8_t *pCurrCount);
uint8_t  CC2520_TXBUF(uint8_t count, uint8_t  *pData);
void   CC2520_TXBUF8(uint8_t data);
void   CC2520_TXBUF16(uint16_t data);
uint8_t  CC2520_TXBUFCP(uint8_t pri, uint16_t addr, uint8_t count, uint8_t *pCurrCount);
uint8_t  CC2520_RANDOM(uint8_t count, uint8_t  *pData);
uint8_t  CC2520_RANDOM8(void);
uint16_t CC2520_RANDOM16(void);
uint8_t  CC2520_RXMASKOR(uint16_t orMask);
uint8_t  CC2520_RXMASKAND(uint16_t andMask);
uint8_t  CC2520_MEMCP(uint8_t pri, uint16_t count, uint16_t src, uint16_t dest);
uint8_t  CC2520_MEMCPR(uint8_t pri, uint16_t count, uint16_t src, uint16_t dest);
uint8_t  CC2520_MEMXCP(uint8_t pri, uint16_t count, uint16_t src, uint16_t dest);
uint8_t  CC2520_MEMXWR(uint16_t addr, uint16_t count, uint8_t  *pData);
uint8_t  CC2520_BSET(uint8_t bitAddr);
uint8_t  CC2520_BCLR(uint8_t bitAddr);
uint8_t  CC2520_CTR(uint8_t pri, uint8_t k, uint8_t c, uint8_t n, uint16_t src, uint16_t dest);
uint8_t  CC2520_UCTR(uint8_t pri, uint8_t k, uint8_t c, uint8_t n, uint16_t src, uint16_t dest);
uint8_t  CC2520_CBCMAC(uint8_t pri, uint8_t k, uint8_t c, uint16_t src, uint16_t dest, uint8_t m);
uint8_t  CC2520_UCBCMAC(uint8_t pri, uint8_t k, uint8_t c, uint16_t src, uint8_t m);
uint8_t  CC2520_CCM(uint8_t pri, uint8_t k, uint8_t c, uint8_t n, uint16_t src, \
    uint16_t dest, uint8_t f, uint8_t m);
uint8_t  CC2520_UCCM(uint8_t pri, uint8_t k, uint8_t c, uint8_t n, uint16_t src, \
    uint16_t dest, uint8_t f, uint8_t m);
uint8_t  CC2520_ECB(uint8_t pri, uint8_t k, uint8_t c, uint16_t src, uint16_t dest);
uint8_t  CC2520_ECBO(uint8_t pri, uint8_t k, uint8_t c, uint16_t addr);
uint8_t  CC2520_ECBX(uint8_t pri, uint8_t k, uint8_t c, uint16_t src, uint16_t dest);
uint8_t  CC2520_ECBXO(uint8_t pri, uint8_t k, uint8_t c, uint16_t addr);
uint8_t  CC2520_INC(uint8_t pri, uint8_t c, uint16_t addr);
uint8_t  CC2520_ABORT(uint8_t c);
uint8_t  CC2520_REGRD(uint8_t addr, uint8_t count, uint8_t  *pValues);
uint8_t  CC2520_REGRD8(uint8_t addr);
uint16_t CC2520_REGRD16(uint8_t addr);
uint32_t CC2520_REGRD24(uint8_t addr);
uint8_t  CC2520_REGWR(uint8_t addr, uint8_t count, uint8_t  *pValues);
void   CC2520_REGWR8(uint8_t addr, uint8_t value);
void   CC2520_REGWR16(uint8_t addr, uint16_t value);
void   CC2520_REGWR24(uint8_t addr, uint32_t value);
uint8_t  CC2520_INS_STROBE(uint8_t strobe);

#endif
