/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*a Defines
 */
#define MAX_SYMBOL_SIZE (256)
#define HASH_TABLE_SIZE (0x1000)
#define SYMADDR_HASH(n) (((n>>2) ^ (n >> 12)) & (HASH_TABLE_SIZE-1))

/*a Types
 */
/*t t_symbol
 */
typedef struct t_symbol
{
    t_symbol *next;
    unsigned int address;
    char *name;
} t_symbol;

/*a Statics
 */
static t_symbol *symbols_hash_table[ HASH_TABLE_SIZE ];

/*a Lookup functions
 */
/*f symbol_lookup
 */
extern const char *symbol_lookup(unsigned int addr)
{
    int addr_hash;
    t_symbol *s;

    addr_hash = SYMADDR_HASH(addr);
    for (s = symbols_hash_table[addr_hash]; s; s = s->next)
    {
        if (s->address == addr)
        {
            return s->name;
        }
    }
    return NULL;
}

/*a Initialization
 */
/*f symbol_initialize
  Returns number of symbols imported - 0 for error
 */
extern int symbol_initialize( char *filename )
{
     FILE *fp;
     char line [2048];
     int address, hash;
     char name[MAX_SYMBOL_SIZE];
     t_symbol *symbol;
     int count;

     fp = fopen( filename, "r" );
     if (!fp)
          return 0;

     count = 0;
     while (fgets (line, sizeof(line), fp))
     {
          symbol = (t_symbol *)malloc(sizeof(t_symbol));
          if (!symbol)
               return 0;
          if (sscanf (line, "%x %*s %s", &address, name)==2)
          {
               hash = SYMADDR_HASH(address);
               symbol->next = symbols_hash_table[hash];
               symbols_hash_table[hash] = symbol;
               symbol->address = address;
               symbol->name = (char *)malloc(strlen(name)+1);
               if (!symbol->name)
                    return 0;
               strcpy( symbol->name, name );
               count++;
          }
     }
     return count;
}


