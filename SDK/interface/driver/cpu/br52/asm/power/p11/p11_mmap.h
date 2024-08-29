#ifndef __P11_MMAP_H__
#define __P11_MMAP_H__

/////////////////////////////////////////////////////////////////////////////////
//#ifdef PMU_SYSTEM
#if 0
#define P11_RAM_BASE 				0
#else
#define P11_RAM_BASE 				0xF20000
#endif
#define P11_NVRAM_BEGIN             (P11_RAM_BASE)
#define P11_NVRAM_SIZE             	0x2000
#define P11_NVRAM_END               (P11_NVRAM_BEGIN + P11_NVRAM_SIZE)

#define P11_RAM0_BEGIN              (P11_NVRAM_END)
#define P11_RAM0_SIZE                0x4000
#define P11_RAM0_END                (P11_RAM0_BEGIN + P11_RAM0_SIZE)

#define P11_RAM_SIZE				 0x4000

/////////////////////////////////////////////////////////////////////////////////
#define P11_RAM0_END_T 				P11_RAM0_END
#define P11_RAM0_BEGIN_T 			P11_RAM0_BEGIN
#define P11_RAM0_SIZE_T 			(P11_RAM0_BEGIN - P11_RAM0_END)

/////////////////////////////////////////////////////////////////////////////////
#define MSYS_POFF_RAM_END   		P11_NVRAM_END
#define MSYS_POFF_RAM_SIZE  	    0x20
#define MSYS_POFF_RAM_BEGIN 		(MSYS_POFF_RAM_END - MSYS_POFF_RAM_SIZE)

#define M2P_MESSAGE_END 			MSYS_POFF_RAM_BEGIN
#define M2P_MESSAGE_SIZE 			0xe0
#define M2P_MESSAGE_RAM_BEGIN 		(M2P_MESSAGE_END - M2P_MESSAGE_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define P2M_MESSAGE_END				M2P_MESSAGE_RAM_BEGIN
#define P2M_MESSAGE_SIZE 			0x40
#define P2M_MESSAGE_RAM_BEGIN 		(P2M_MESSAGE_END - P2M_MESSAGE_SIZE)

///////////////////////////////////////////////////////////////////////////////
#define P11_NVRAM_END_T				(P2M_MESSAGE_RAM_BEGIN)
#define P11_NVRAM_BEGIN_T 			(P11_NVRAM_BEGIN+P11_ISR_SIZE)
#define P11_NVRAM_SIZE_T			(P11_NVRAM_END_T - P11_NVRAM_BEGIN_T)

///////////////////////////////////////////////////////////////////////////////
#define P11_ISR_END						P11_NVRAM_BEGIN_T
#define P11_ISR_BASE 					0x0
#define P11_ISR_SIZE 					0x80


//////////////////////////////////////////////////////////////////////////////
#define P11_RAM_ACCESS(x)   		(*(volatile u8 *)(x))
#define P2M_MESSAGE_ACCESS(x)      	P11_RAM_ACCESS(P2M_MESSAGE_RAM_BEGIN + x)
#define M2P_MESSAGE_ACCESS(x)      	P11_RAM_ACCESS(M2P_MESSAGE_RAM_BEGIN + x)

#endif

