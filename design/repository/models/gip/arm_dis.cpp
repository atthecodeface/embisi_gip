/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arm_dis.h"
#include "symbols.h"

/*a Statics
 */
static char *str_alu_opcode[16] =
{
	"and", "eor", "sub", "rsb",
	"add", "adc", "sbc", "rsc",
	"tst", "teq", "cmp", "cmn",
	"orr", "mov", "bic", "mvn"
};

static char *str_cc[16] =
{
	"eq", "ne", "cs", "cc",
	"mi", "pl", "vs", "vc",
	"hi", "ls", "ge", "lt",
	"gt", "le", "", "nv"
};

static char *str_r[16] =
{
	"r0", "r1", "r2", "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "sp", "lr", "pc" 
};

static char *str_shf[4] =
{
	"lsl", "lsr", "asr", "ror"
};

static char text_result[256];

/*a Support functions
 */
/*f get_string
 */
static char *get_string( int value )
{
	if ((value>-256) && (value<256))
	{
		sprintf( text_result, "%d", value );
		return text_result;
	}
	sprintf( text_result, "0x%8x", value );
	return text_result;
}

/*f rotate_right
 */
static int rotate_right( int value, int by )
{
	unsigned int result;
	result = ((unsigned int) value)>>by;
	result |= ((unsigned int) value)<<(32-by);
	return (int) result;
}

/*f append_string
 */
static void append_string( char **buffer, char *string )
{
	strcpy( *buffer, string );
	*buffer = *buffer+strlen(*buffer);
}

/*a Disassembly functions
 */
/*f arm_dis_coproc
 */
static int arm_dis_coproc( int address, int opcode, char *buffer )
{
    int cc, cpr, cpopc, crn, crd, cpn, cp, crm;

    cc      = (opcode>>28) & 0x0f;
    cpr     = (opcode>>24) & 0x0f;
	cpopc   = (opcode>>20) & 0x0f;
	crn     = (opcode>>16) & 0x0f;
	crd     = (opcode>>12) & 0x0f;
	cpn     = (opcode>>8) & 0x0f;
	cp      = (opcode>>5) & 0x07;
	crm     = (opcode>>0) & 0x0f;

    if (cpr!=0xe)
        return 0;

    if (opcode&0x10)
    {
        sprintf( buffer,
                 "m%s%s p%d, %d, c%d, c%d, c%d, %d",
                 (cpopc&1)?"rc":"cr", str_cc[ cc ],
                 cpn, cpopc>>1, crd, crn, crm, cp );
    }
    else
    {
        sprintf( buffer,
                 "cdp%s p%d, %d, c%d, c%d, c%d, %d",
                 str_cc[ cc ],
                 cpn, cpopc, crd, crn, crm, cp );
    }
	return 1;
}

/*f arm_dis_branch
 */
static int arm_dis_branch( int address, int opcode, char *buffer )
{
	int cc, five, link, offset;
	int target;
	const char * target_label;

	cc      = (opcode>>28) & 0x0f;
	five    = (opcode>>25) & 0x07;
	link    = (opcode>>24) & 0x01;
	offset  = (opcode>> 0) & 0x00ffffff;

	if (five != 5)
		return 0;

	target = address+((offset<<8)>>6)+8;
	target_label = symbol_lookup (target);
	
	if (!target_label)
	{
		sprintf( buffer,
			 "b%s%s %08x",
			 link ? "l" : "",
			 str_cc[ cc ],
			 target);
	}
	else
	{
		sprintf( buffer,
			 "b%s%s %s (%08x)",
			 link ? "l" : "",
			 str_cc[ cc ],
			 target_label,
			 target);
	}
	return 1;
}

/*f arm_dis_alu
 */
static int arm_dis_alu( int address, int opcode, char *buffer )
{
	int cc, zeros, imm, alu_op, sign, rn, rd, op2;
	int imm_val, imm_rotate;
	int shf_rs, shf_reg_zero, shf_imm_amt, shf_how, shf_by_reg, shf_rm;

	cc      = (opcode>>28) & 0x0f;
	zeros   = (opcode>>26) & 0x03;
	imm     = (opcode>>25) & 0x01;
	alu_op  = (opcode>>21) & 0x0f;
	sign    = (opcode>>20) & 0x01;
	rn      = (opcode>>16) & 0x0f;
	rd      = (opcode>>12) & 0x0f;
	op2     = (opcode>> 0) & 0xfff;

	if (zeros)
		return 0;

	imm_rotate = (op2>>8) & 0x0f;
	imm_val    = (op2>>0) & 0xff;

	shf_rs       = (op2>> 8) & 0x0f;
	shf_reg_zero = (op2>> 7) & 0x01;
	shf_imm_amt  = (op2>> 7) & 0x1f;
	shf_how      = (op2>> 5) & 0x03;
	shf_by_reg   = (op2>> 4) & 0x01;
	shf_rm       = (op2>> 0) & 0x0f;

	if (!imm && shf_by_reg && shf_reg_zero)
		return 0;

	append_string( &buffer, str_alu_opcode[ alu_op ] );
	append_string( &buffer, str_cc[ cc ] );
	if ( (alu_op == 0x8) ||
		 (alu_op == 0x9) ||
		 (alu_op == 0xa) ||
		 (alu_op == 0xb) ) // cmp, cmn, tst, teq
	{
		if (rd==15)
		{
			append_string( &buffer, "p" );
		}
		if (!sign)
		{
			return 0;
		}
	}
	else
	{
		if (sign)
		{
			append_string( &buffer, "s" );
		}
	}
	append_string( &buffer, " " );

	if ( (alu_op == 0x8) ||
		 (alu_op == 0x9) ||
		 (alu_op == 0xa) ||
		 (alu_op == 0xb) ) // cmp, cmn, tst, teq
	{
		append_string( &buffer, str_r[rn] );
		append_string( &buffer, ", " );
	}
	else if ( (alu_op == 0xd) ||
			  (alu_op == 0xf) ) // mov, mvn
	{
		append_string( &buffer, str_r[rd] );
		append_string( &buffer, ", " );
	}
	else
	{
		append_string( &buffer, str_r[rd] );
		append_string( &buffer, ", " );
		append_string( &buffer, str_r[rn] );
		append_string( &buffer, ", " );
	}

	if (imm)
	{
		append_string( &buffer, "#" );
		append_string( &buffer, get_string( rotate_right( imm_val, imm_rotate*2 ) ) );
	}
	else if (shf_by_reg)
	{
		if (shf_reg_zero)
			return 0;
		append_string( &buffer, str_r[shf_rm] );
		append_string( &buffer, ", " );
		append_string( &buffer, str_shf[shf_how] );
		append_string( &buffer, " " );
		append_string( &buffer, str_r[shf_rs] );
	}
	else if ((shf_how==0) && (shf_imm_amt==0))
	{
		append_string( &buffer, str_r[shf_rm] );
	}
	else if ((shf_how==3) && (shf_imm_amt==0))
	{
		append_string( &buffer, str_r[shf_rm] );
		append_string( &buffer, ", rrx" );
	}
	else
	{
		append_string( &buffer, str_r[shf_rm] );
		append_string( &buffer, ", " );
		append_string( &buffer, str_shf[shf_how] );
		append_string( &buffer, " #" );
		append_string( &buffer, get_string( shf_imm_amt ) );
	}
	return 1; 
}

/*f arm_dis_ld_st
 */
static int arm_dis_ld_st( int address, int opcode, char *buffer )
{
	int cc, one, not_imm, pre, up, byte, wb, load, rn, rd, offset;
	int imm_val;
	int shf_imm_amt, shf_how, shf_zero, shf_rm;

	cc      = (opcode>>28) & 0x0f;
	one     = (opcode>>26) & 0x03;
	not_imm = (opcode>>25) & 0x01;
	pre     = (opcode>>24) & 0x01;
	up      = (opcode>>23) & 0x01;
	byte    = (opcode>>22) & 0x01;
	wb      = (opcode>>21) & 0x01;
	load    = (opcode>>20) & 0x01;
	rn      = (opcode>>16) & 0x0f;
	rd      = (opcode>>12) & 0x0f;
	offset  = (opcode>> 0) & 0xfff;

	if (one != 1)
		return 0;

	imm_val    = (offset>>0) & 0xfff;

	shf_imm_amt  = (offset>> 7) & 0x1f;
	shf_how      = (offset>> 5) & 0x01;
	shf_zero     = (offset>> 4) & 0x01;
	shf_rm       = (offset>> 0) & 0x0f;

	if (not_imm && shf_zero)
		return 0;

	if (load)
	{
		append_string( &buffer, "ldr" );
	}
	else
	{
		append_string( &buffer, "str" );
	}
	append_string( &buffer, str_cc[ cc ] );
	if (byte)
	{
		append_string( &buffer, "b" );
	}
	if (wb && !pre)
	{
		append_string( &buffer, "t" );
	}
	append_string( &buffer, " " );
	append_string( &buffer, str_r[rd] );
	append_string( &buffer, ", [" );
	append_string( &buffer, str_r[rn] );

	if (!pre)
	{
		append_string( &buffer, "]" );
	}

	if (!not_imm)
	{
		if (imm_val==0)
		{
		}
		else if (up)
		{
			append_string( &buffer, ", #" );
			append_string( &buffer, get_string( imm_val ) );
		}
		else
		{
			append_string( &buffer, ", #-" );
			append_string( &buffer, get_string( imm_val ) );
		}
	}
	else if ((shf_how==0) && (shf_imm_amt==0))
	{
		if (up)
		{
			append_string( &buffer, ", " );
			append_string( &buffer, str_r[shf_rm] );
		}
		else
		{
			append_string( &buffer, ", -" );
			append_string( &buffer, str_r[shf_rm] );
		}
	}
	else if ((shf_how==3) && (shf_imm_amt==0))
	{
		if (up)
		{
			append_string( &buffer, ", " );
			append_string( &buffer, str_r[shf_rm] );
			append_string( &buffer, ", rrx" );
		}
		else
		{
			append_string( &buffer, ", -" );
			append_string( &buffer, str_r[shf_rm] );
			append_string( &buffer, ", rrx" );
		}
	}
	else
	{
		if (up)
		{
			append_string( &buffer, ", " );
			append_string( &buffer, str_r[shf_rm] );
			append_string( &buffer, ", " );
			append_string( &buffer, str_shf[shf_how] );
			append_string( &buffer, " #" );
			append_string( &buffer, get_string( shf_imm_amt ) );
		}
		else
		{
			append_string( &buffer, ", -" );
			append_string( &buffer, str_r[shf_rm] );
			append_string( &buffer, ", " );
			append_string( &buffer, str_shf[shf_how] );
			append_string( &buffer, " #" );
			append_string( &buffer, get_string( shf_imm_amt ) );
		}
	}

	if (pre)
	{
		append_string( &buffer, "]" );
	}
	if (wb && pre)
	{
		append_string( &buffer, "!" );
	}
	return 1; 
}

/*f arm_dis_ldm_stm
 */
static int arm_dis_ldm_stm( int address, int opcode, char *buffer )
{
	int cc, four, pre, up, psr, wb, load, rn, regs;
	int i, j;

	cc      = (opcode>>28) & 0x0f;
	four    = (opcode>>25) & 0x07;
	pre     = (opcode>>24) & 0x01;
	up      = (opcode>>23) & 0x01;
	psr     = (opcode>>22) & 0x01;
	wb      = (opcode>>21) & 0x01;
	load    = (opcode>>20) & 0x01;
	rn      = (opcode>>16) & 0x0f;
	regs    = (opcode>> 0) & 0xffff;

	if (four != 4)
		return 0;

	if (load)
	{
		append_string( &buffer, "ldm" );
	}
	else
	{
		append_string( &buffer, "stm" );
	}
	append_string( &buffer, str_cc[ cc ] );
	if (load && (rn==13) && !pre && up)
	{
		append_string( &buffer, "fd" );
	}
	else if (!load && (rn==13) && pre && !up)
	{
		append_string( &buffer, "fd" );
	}
	else
	{
		if (up)
		{
			append_string( &buffer, "i" );
		}
		else
		{
			append_string( &buffer, "d" );
		}
		if (pre)
		{
			append_string( &buffer, "b" );
		}
		else
		{
			append_string( &buffer, "a" );
		}
	}

	append_string( &buffer, " " );
	append_string( &buffer, str_r[rn] );
	if (wb)
	{
		append_string( &buffer, "!" );
	}

	append_string( &buffer, ", {" );
	j = 0;
	for (i=0; i<16; i++)
	{
		if ((regs>>i)&1)
		{
			if (j)
			{
				append_string( &buffer, ", " );
			}
			append_string( &buffer, str_r[ i ] );
			j=1;
		}
	}
	append_string( &buffer, "}" );
	if (psr)
	{
		append_string( &buffer, "^" );
	}
	return 1; 
}

/*f arm_dis_mul
 */
static int arm_dis_mul( int address, int opcode, char *buffer )
{
	int cc, zero, nine, accum, sign, rd, rn, rs, rm;

	cc      = (opcode>>28) & 0x0f;
	zero    = (opcode>>22) & 0x3f;
	accum   = (opcode>>21) & 0x01;
	sign    = (opcode>>20) & 0x01;
	rd      = (opcode>>16) & 0x0f;
	rn      = (opcode>>12) & 0x0f;
	rs      = (opcode>>8) & 0x0f;
	nine    = (opcode>>4) & 0x0f;
	rm      = (opcode>>0) & 0x0f;

	if ((zero!=0) || (nine!=9))
		return 0;

    if (accum)
	{
		append_string( &buffer, "mla" );
	}
	else
	{
		append_string( &buffer, "mul" );
	}
	append_string( &buffer, str_cc[ cc ] );
	if (sign)
	{
		append_string( &buffer, "s" );
	}

	append_string( &buffer, " " );
	append_string( &buffer, str_r[rd] );
	append_string( &buffer, ", " );
	append_string( &buffer, str_r[rm] );
	append_string( &buffer, ", " );
	append_string( &buffer, str_r[rs] );
    if (accum)
	{
        append_string( &buffer, ", " );
        append_string( &buffer, str_r[rn] );
	}
	return 1; 
}

/*f arm_disassemble
 */
extern int arm_disassemble( int address, int opcode, char *buffer )
{
	if (arm_dis_coproc( address, opcode, buffer ))
		return 1;
	if (arm_dis_branch( address, opcode, buffer ))
		return 1;
	if (arm_dis_ld_st( address, opcode, buffer ))
		return 1;
	if (arm_dis_ldm_stm( address, opcode, buffer ))
		return 1;
	if (arm_dis_mul( address, opcode, buffer ))
		return 1;
	return arm_dis_alu( address, opcode, buffer );
}

