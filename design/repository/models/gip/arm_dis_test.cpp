#include <stdio.h>
#include <stdlib.h>

#include "arm_dis.h"

/*a Main routines
 */
/*f main
 */
extern int main( int argc, char **argv )
{
	char buffer[256];
	int opcode;
	int address;

	while (!feof(stdin))
	{
		if (scanf( "%08x: %08x %*s\n", &address, &opcode)==2)
		{
			printf("%08x: %08x ",address, opcode);
			if (arm_disassemble( address, opcode, buffer ))
			{
				printf("%s",buffer);
			}
			printf("\n");
		}
		else
		{
			int c;
			c=getchar();
			while ((c!=EOF) && (c!='\n'))
			{
				c=getchar();
			}
			c=getchar();
		}
	}
}
