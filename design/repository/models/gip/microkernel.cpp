#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "microkernel.h"
#include "c_execution_model_class.h"

//#include "../../os/linux/include/asm/ukernel.h"
#include "syscalls.h"
#include "ether.h"

#define MAX_IRQS 8

struct microkernel_data
{
	c_execution_model_class * gip;
	int swi_handler;
	int super;
	unsigned long saved_svc_sp;
	int iret_address;
	int last_syscall;
	int irq_pending;
	int irq_enabled;
	int irq_handler [MAX_IRQS];
	int irq_wrapper;
};

microkernel::microkernel (c_execution_model_class * gip)
{
	private_data = (microkernel_data *) memset (malloc (sizeof(microkernel_data)), 0, sizeof(microkernel_data));
	private_data->gip = gip;
	private_data->super = 1;
}

char tty_in (void);
void tty_out (char c);

void microkernel::handle_support (void)
{
	c_execution_model_class * gip = private_data->gip;
	int r0 = gip->get_register (0);
//	printf ("Support call %d\n", r0);
	switch (r0)
	{
/*
	case UKERNEL_SUPPORT_TTY_GETCH:
		printf ("calling tty_in\n");
		gip->set_register (0, tty_in());
		break;

	case UKERNEL_SUPPORT_TTY_PUTCH:
		tty_out ((char)(gip->get_register (1)));
		break;

	case UKERNEL_SUPPORT_CONSOLE_PUTCH:
		printf ("%c", (char)(gip->get_register(1)));
		break;

	case UKERNEL_SUPPORT_SET_IRQ_HANDLER:
		private_data->irq_handler[gip->get_register(1)] = gip->get_register(2);
		break;
		
	case UKERNEL_SUPPORT_SET_SWI_HANDLER:
		private_data->swi_handler = gip->get_register(1);
		break;

	case UKERNEL_SUPPORT_SET_IRQ_WRAPPER:
		private_data->irq_wrapper = gip->get_register (1);
		break;
		
	case UKERNEL_SUPPORT_SET_IRET:
		private_data->iret_address = gip->get_register (1);
		break;
		
	case UKERNEL_SUPPORT_ETH_TX:
		{
		unsigned int pkt_addr = gip->get_register(1);
		unsigned int pkt_len = gip->get_register(2);
		unsigned char pkt [1500];
		unsigned int i;
//		printf ("eth_tx pkt_addr = %x\n", pkt_addr);
		for (i = 0; i < pkt_len; i++)
			pkt[i] = (unsigned char) (gip->read_memory (pkt_addr+i) >> (8*(i&3)));
//		printf ("pkt = %p, pkt_len = %d\n", pkt, pkt_len);
		ether_send_packet (pkt, pkt_len);
		break;
		}
	
	case UKERNEL_SUPPORT_ETH_RX:
		{
		unsigned int pkt_len = ether_size();
		if (pkt_len)
		{
			unsigned long * pkt = (unsigned long *) ether_rx_packet ();
			unsigned int pkt_addr = gip->get_register(1);
			unsigned int i;
			unsigned int words = (pkt_len + 3) / 4;
			if (pkt_addr & 3)
			{
				printf ("Unaligned buffer passed to eth_rx\n");
				exit (1);
			}
			for (i = 0; i < words; i++)
				gip->write_memory (pkt_addr + i*4, pkt[i], 0xf);
//			printf ("eth_rx pkt = %p, pkt_len = %d\n", pkt, pkt_len);
		}
		gip->set_register (0, pkt_len);
		break;
		}
	
	case UKERNEL_SUPPORT_IDLE:
//		printf ("[idle]\n");
		usleep (1000);
		break;
		
*/	default:
		printf ("Unknown uKernel support call\n");
		exit (1);
	}
}

void microkernel::push (int n)
{
	c_execution_model_class * gip = private_data->gip;
	gip->set_register (13, gip->get_register(13)-4);
/*	gip->write_memory (gip->get_register(13), n, 0xf);*/
}

#define SYSCALL_FRAME_R0 0x00
#define SYSCALL_FRAME_R1 0x04
#define SYSCALL_FRAME_R2 0x08
#define SYSCALL_FRAME_R3 0x0c
#define SYSCALL_FRAME_R4 0x10
#define SYSCALL_FRAME_R5 0x14
#define SYSCALL_FRAME_R6 0x18
#define SYSCALL_FRAME_R7 0x1c
#define SYSCALL_FRAME_R8 0x20
#define SYSCALL_FRAME_R9 0x24
#define SYSCALL_FRAME_R10 0x28
#define SYSCALL_FRAME_R11 0x2c
#define SYSCALL_FRAME_R12 0x30
#define SYSCALL_FRAME_R13 0x34
#define SYSCALL_FRAME_R14 0x38
#define SYSCALL_FRAME_R15 0x3c
#define SYSCALL_FRAME_CPSR 0x40
#define SYSCALL_FRAME_XXX 0x44
#define SYSCALL_FRAME_SIZE 0x48

void gdb_trap (int n);
void swi_hack (void);

static int get_pending_irq (int pending_irqs)
{
	int vector;
	for (vector = 0; vector < MAX_IRQS; vector++)
		if (pending_irqs & (1 << vector))
			return vector;
	return -1;
}

int microkernel::do_interrupt (void)
{
/*
	int vector = get_pending_irq (private_data->irq_pending);
	if (vector == -1) return 0;
	
//	printf ("{ISR%d(pc=%x)", vector, gip->get_register(15));
//	fflush (stdout);

	private_data->irq_pending &= ~(1 << vector);
	private_data->irq_enabled = 0;
	
	unsigned int flags = gip->get_flags();
	unsigned int sp = (flags & am_flag_super) ? gip->get_register (13) : private_data->saved_svc_sp;
	int i;
	sp -= SYSCALL_FRAME_SIZE;
	for (i = 0; i <= 15; i++)
		gip->write_memory (sp + SYSCALL_FRAME_R0 + i*4, gip->get_register (i), 0xf);
	gip->write_memory (sp + SYSCALL_FRAME_CPSR, gip->get_flags(), 0xf);
	
	gip->set_register (0, sp);
	gip->set_register (15, private_data->irq_handler[vector]);
	gip->set_register (13, sp);
	gip->set_register (14, private_data->iret_address);
//	gip->set_register (15, private_data->irq_wrapper);
*/
	return 1;
}

void microkernel::handle_interrupt (int vector)
{
	if (private_data->irq_handler[vector])
	{
//		printf ("[IRQ%d]", vector);
//		fflush (stdout);
		private_data->irq_pending |= (1 << vector);
		if (private_data->irq_enabled)
			do_interrupt();
	}
}

void microkernel::return_from_interrupt (void)
{
	c_execution_model_class * gip = private_data->gip;
//	printf ("ISR(pc=%x)}", gip->read_memory (gip->get_register(13)+SYSCALL_FRAME_R15));
	if (private_data->irq_pending)
	{
		// another one waiting...
		int irq = get_pending_irq (private_data->irq_pending);
		private_data->irq_pending &= ~(1 << irq);
		gip->set_register (0, gip->get_register (13));
		gip->set_register (15, private_data->irq_handler[irq]);
		gip->set_register (14, private_data->iret_address);
//		gip->set_register (15, private_data->irq_wrapper);
//		printf ("{..ISR%d", irq);
	}
	else
	{
		unsigned sp = gip->get_register (13);
		int i;
//		for (i = 0; i < 18*4; i+= 4)
//		{
//			printf ("SP+%2.2x (%8.8x): %8.8x\n", i, sp+i, gip->read_memory (sp+i));
//		}
/*		unsigned int saved_cpsr = gip->read_memory (sp+SYSCALL_FRAME_CPSR);
		if (saved_cpsr & am_flag_super)
		{
//			printf ("IRQ Return to SVC mode\n");
			for (i = 0; i <= 15; i++)
			{
				gip->set_register (i, gip->read_memory (sp + SYSCALL_FRAME_R0 + i*4));
			}
			gip->set_flags (saved_cpsr, 0xffffffff);
		}
		else
		{
//			printf ("IRQ Return to user mode\n");
			for (i = 0; i <= 15; i++)
			{
				gip->set_register (i, gip->read_memory (sp + SYSCALL_FRAME_R0 + i*4));
			}
			gip->set_flags (saved_cpsr, 0xffffffff);
			private_data->saved_svc_sp = sp + SYSCALL_FRAME_SIZE;
		}
		private_data->irq_enabled = 1;
*/	//	gdb_trap(5);
	}
//	fflush (stdout);
}


void microkernel::handle_syscall (int n)
{
	c_execution_model_class * gip = private_data->gip;
	unsigned int flags = gip->get_flags();
	int i;
/*	if (n < noof_call_names)
		printf ("*** uKernel %s\n", call_name[n]);
	else
		printf ("*** uKERNEL HANDLE SWI %x\n", n);
	unsigned int sp = (flags & am_flag_super) ? gip->get_register (13) : private_data->saved_svc_sp;
	
	sp -= SYSCALL_FRAME_SIZE;
	for (i = 0; i <= 15; i++)
		gip->write_memory (sp + SYSCALL_FRAME_R0 + i*4, gip->get_register (i), 0xf);
	gip->write_memory (sp + SYSCALL_FRAME_CPSR, gip->get_flags(), 0xf);
	gip->set_flags (am_flag_super, am_flag_super);
	gip->set_register (13, sp);
	gip->set_register (7, n);
	gip->set_register (15, private_data->swi_handler);
	private_data->last_syscall = n;
*/
//	swi_hack ();
}


void microkernel::handle_syscall_ret (void)
{
	c_execution_model_class * gip = private_data->gip;
	unsigned sp = gip->get_register (13);
	int i;
/*
	printf ("result = 0x%x\n", gip->get_register (0));
//	for (i = 0; i < 18*4; i+= 4)
//	{
//		printf ("SP+%2.2x (%8.8x): %8.8x\n", i, sp+i, gip->read_memory (sp+i));
//	}
	unsigned int saved_cpsr = gip->read_memory (sp+SYSCALL_FRAME_CPSR);
	if (saved_cpsr & am_flag_super)
	{
//		printf ("SWI Return to SVC mode\n");
		for (i = 0; i <= 15; i++)
		{
			if (i != 13)
				gip->set_register (i, gip->read_memory (sp + SYSCALL_FRAME_R0 + i*4));
		}
		gip->set_flags (saved_cpsr, 0xffffffff);
		gip->set_register (13, sp + SYSCALL_FRAME_SIZE);
//		gdb_trap(5);
	}
	else
	{
//		printf ("SWI Return to user mode\n");
		for (i = 0; i <= 15; i++)
		{
			gip->set_register (i, gip->read_memory (sp + SYSCALL_FRAME_R0 + i*4));
		}
		gip->set_flags (saved_cpsr, 0xffffffff);
		private_data->saved_svc_sp = sp + SYSCALL_FRAME_SIZE;
//		if (private_data->last_syscall == 45) 
//			gdb_trap (5);
	}
*/
}

void microkernel::handle_swi (int n)
{
	c_execution_model_class * gip = private_data->gip;
//	printf ("%c", (gip->get_flags() & am_flag_super) ? '#' : '$');
	switch (n)
	{
/*
	case UKERNEL_SWI_DO_SWI:
		break;
		
	case UKERNEL_SWI_RETURN_SWI:
		handle_syscall_ret ();
		break;

	case UKERNEL_SWI_RETURN_IRQ:
		return_from_interrupt ();
		break;
		
	case UKERNEL_SWI_SUPPORT:
		handle_support ();
		break;
		
	case UKERNEL_SWI_DISABLE_IRQ:
		gip->set_register (0, private_data->irq_enabled);
		private_data->irq_enabled = 0;
//		printf ("<DI>");
		break;

	case UKERNEL_SWI_JUST_DISABLE_IRQ:
		private_data->irq_enabled = 0;
//		printf ("<JDI>");
	       	break;
		
	case UKERNEL_SWI_RESTORE_IRQ:
//		printf ("<RI%d>", gip->get_register(0));
		if (gip->get_register (0))
		{
			private_data->irq_enabled = 1;
			if (private_data->irq_pending)
				do_interrupt();
		}
		break;

	case UKERNEL_SWI_ENABLE_IRQ:
//		printf ("<EI>");
		private_data->irq_enabled = 1;
		if (private_data->irq_pending)
			do_interrupt();
		break;

	case UKERNEL_SWI_GET_IRQ:
//		printf ("<GI>");
		gip->set_register (0, private_data->irq_enabled);
		break;
	
	case UKERNEL_SWI_FLUSH_ICACHE:
		printf ("Flush ICache\n");
		gip->flush_cache();
		break;
		
*/	default:
		handle_syscall (n);
		break;
	}
}

