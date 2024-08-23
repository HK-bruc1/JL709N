
//===============================================================================//
//
//      input IO define
//
//===============================================================================//
#define PA0_IN  1
#define PA1_IN  2
#define PA2_IN  3
#define PA3_IN  4
#define PA4_IN  5
#define PA5_IN  6
#define PA6_IN  7
#define PA7_IN  8
#define PB0_IN  9
#define PB1_IN  10
#define PB2_IN  11
#define PB3_IN  12
#define PB4_IN  13
#define PB5_IN  14
#define PB6_IN  15
#define PB7_IN  16
#define PB8_IN  17
#define PC0_IN  18
#define PC1_IN  19
#define PC2_IN  20
#define PC3_IN  21
#define PC4_IN  22
#define PC5_IN  23
#define PC6_IN  24
#define PC7_IN  25
#define USBDP_IN  26
#define USBDM_IN  27
#define PP0_IN  28

//===============================================================================//
//
//      function input select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 FI_GP_ICH0;
    __RW __u8 FI_GP_ICH1;
    __RW __u8 FI_GP_ICH2;
    __RW __u8 FI_GP_ICH3;
    __RW __u8 FI_GP_ICH4;
    __RW __u8 FI_GP_ICH5;
    __RW __u8 FI_SD0_CMD;
    __RW __u8 FI_SD0_DA0;
    __RW __u8 FI_SPI1_CLK;
    __RW __u8 FI_SPI1_DA0;
    __RW __u8 FI_SPI1_DA1;
    __RW __u8 FI_SPI1_DA2;
    __RW __u8 FI_SPI1_DA3;
    __RW __u8 FI_SPI2_CLK;
    __RW __u8 FI_SPI2_DA0;
    __RW __u8 FI_SPI2_DA1;
    __RW __u8 FI_SPI2_DA2;
    __RW __u8 FI_SPI2_DA3;
    __RW __u8 FI_IIC0_SCL;
    __RW __u8 FI_IIC0_SDA;
    __RW __u8 FI_UART0_RX;
    __RW __u8 FI_UART1_RX;
    __RW __u8 FI_UART2_RX;
    __RW __u8 FI_ALNK0_MCLK;
    __RW __u8 FI_ALNK0_LRCK;
    __RW __u8 FI_ALNK0_SCLK;
    __RW __u8 FI_ALNK0_DAT0;
    __RW __u8 FI_ALNK0_DAT1;
    __RW __u8 FI_ALNK0_DAT2;
    __RW __u8 FI_ALNK0_DAT3;
    __RW __u8 FI_SPDIF_DIA;
    __RW __u8 FI_SPDIF_DIB;
    __RW __u8 FI_SPDIF_DIC;
    __RW __u8 FI_SPDIF_DID;
    __RW __u8 FI_CAN_RX;
    __RW __u8 FI_QDEC0_A;
    __RW __u8 FI_QDEC0_B;
    __RW __u8 FI_CHAIN_IN0;
    __RW __u8 FI_CHAIN_IN1;
    __RW __u8 FI_CHAIN_IN2;
    __RW __u8 FI_CHAIN_IN3;
    __RW __u8 FI_CHAIN_RST;
    __RW __u8 FI_TOTAL;
} JL_IMAP_TypeDef;

#define JL_IMAP_BASE      (ls_base + map_adr(0x3a, 0x00))
#define JL_IMAP           ((JL_IMAP_TypeDef   *)JL_IMAP_BASE)


