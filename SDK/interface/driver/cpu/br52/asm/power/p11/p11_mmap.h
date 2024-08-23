#ifndef __P11_MMAP_H__
#define __P11_MMAP_H__

/////////////////////////////////////////////////////////////////////////////////
#define P11_RAM_BASE 				0xF20000
#define P11_NVRAM_BEGIN             0x80
#define P11_NVRAM_END               0x2000
#define P11_NVRAM_SIZE              (P11_NVRAM_END - P11_NVRAM_BEGIN)

#define P11_RAM0_BEGIN              0x2000
#define P11_RAM0_END                0x4000
#define P11_RAM0_SIZE               (P11_RAM0_END - P11_RAM0_BEGIN)

#define P11_RAM_SIZE				0x4000

/////////////////////////////////////////////////////////////////////////////////
#define MSYS_POFF_RAM_END   		P11_NVRAM_END
#define MSYS_POFF_RAM_SIZE  	    0x20
#define MSYS_POFF_RAM_BEGIN 		(MSYS_POFF_RAM_END - MSYS_POFF_RAM_SIZE)

#define P11_MESSAGE_END 			MSYS_POFF_RAM_BEGIN
#define P2M_MESSAGE_SIZE 			0x40
#define M2P_MESSAGE_SIZE 			0xe0
#define P11_MESSAGE_SIZE 			(M2P_MESSAGE_SIZE + P2M_MESSAGE_SIZE)
#define P11_MESSAGE_BEGIN 			(P11_MESSAGE_END - P11_MESSAGE_SIZE)

///////////////////////////////////////////////////////////////////////////////
#define P2M_MESSAGE_RAM_BEGIN 		(P11_RAM_BASE + P11_MESSAGE_BEGIN)
#define M2P_MESSAGE_RAM_BEGIN 		(P2M_MESSAGE_RAM_BEGIN + P2M_MESSAGE_SIZE)

#define P11_RAM_ACCESS(x)   		(*(volatile u8 *)(x))

#define P2M_MESSAGE_ACCESS(x)      	P11_RAM_ACCESS(P2M_MESSAGE_RAM_BEGIN + x)
#define M2P_MESSAGE_ACCESS(x)      	P11_RAM_ACCESS(M2P_MESSAGE_RAM_BEGIN + x)

#define P11_MESSAGE_RAM_BEGIN 		(P2M_MESSAGE_RAM_BEGIN)
#define P11_RAM_PROTECT_END 		(P11_MESSAGE_RAM_BEGIN)

//////////////////////////////////////////////////////////////////////////////
#endif

