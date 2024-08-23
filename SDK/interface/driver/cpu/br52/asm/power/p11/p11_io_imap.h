
//===============================================================================//
//
//      input IO define
//
//===============================================================================//
#define P11_PB0_IN  1
#define P11_PB1_IN  2

//===============================================================================//
//
//      function input select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 P11_FI_GP_ICH0;
    __RW __u8 P11_FI_GP_ICH1;
    __RW __u8 P11_FI_GP_ICH2;
    __RW __u8 P11_FI_UART0_RX;
    __RW __u8 P11_FI_IIC_SCL;
    __RW __u8 P11_FI_IIC_SDA;
} P11_IMAP_TypeDef;

#define P11_IMAP_BASE      (p11_sfr_base + map_adr(0x17, 0x00))
#define P11_IMAP           ((P11_IMAP_TypeDef   *)P11_IMAP_BASE)


