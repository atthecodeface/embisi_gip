/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"

/*a Types
 */
/*t t_fact_fn
 */
typedef int (*t_fact_fn)( int n );

/*t t_test_vector
 */
typedef struct t_test_vector
{
    int value;
    t_fact_fn fact_fn;
    int result;
} t_test_vector;

/*a Test functions
 */
/*f factorial_add
 */
static int factorial_add( int a )
{
    int result;
    if (a<=0)
        return 0;
    result=a+factorial_add(a-1);
    return result;
}

/*f factorial_mult
 */
static int factorial_mult( int a )
{
    if (a<=0)
        return 1;
    return a*factorial_mult(a-1);
}

/*a Statics test variables
 */
/*v test_vectors - set of tests to perform
 */
static const t_test_vector test_vectors[] = 
{
    //Val  fn  result
    {  0, factorial_add,    0 },
    {  2, factorial_add,    3 },
    { 20, factorial_add,  210 },
    {  1, factorial_mult, 1 },
    {  3, factorial_mult, 6 },
    {  6, factorial_mult, 720 },
    { 11, factorial_mult, 0x02611500 },
    { 12, factorial_mult, 0x1c8cfc00 },
    { 14, factorial_mult, 0x4c3b2800 },
    { 0, NULL, 0 },
};

/*a Main test entry
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    int test, result;
    int failure;

    failure = 0;
    for (i=0; test_vectors[i].fact_fn; i++)
    {
        result = test_vectors[i].fact_fn(test_vectors[i].value);
        dprintf("Factorial %d gives %08x, expected %08x\n", test_vectors[i].value, result, test_vectors[i].result );
        if (result!=test_vectors[i].result)
        {
            if (failure==0)
            {
                failure=i;
            }
        }
    }
    dprintf( "Factorial tests complete, failures %d", failure, 0, 0 );
    return failure;
}
