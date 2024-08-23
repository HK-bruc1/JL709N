
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define P11_FO_GP_OCH0        ((0 << 2)|BIT(1))
#define P11_FO_GP_OCH1        ((1 << 2)|BIT(1))
#define P11_FO_GP_OCH2        ((2 << 2)|BIT(1))
#define P11_FO_UART0_TX        ((3 << 2)|BIT(1)|BIT(0))
#define P11_FO_IIC_SCL        ((4 << 2)|BIT(1)|BIT(0))
#define P11_FO_IIC_SDA        ((5 << 2)|BIT(1)|BIT(0))

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 P11_PB0_OUT;
    __RW __u8 P11_PB1_OUT;
} P11_OMAP_TypeDef;

#define P11_OMAP_BASE      (p11_sfr_base + map_adr(0x16, 0x00))
#define P11_OMAP           ((P11_OMAP_TypeDef   *)P11_OMAP_BASE)


