static int factorial_add( int a )
{
    if (a<=0)
        return 0;
    return a+factorial_add(a-1);
}

static int factorial_mult( int a )
{
    if (a<=0)
        return 1;
    return a*factorial_mult(a-1);
}

extern int test_entry_point()
{
    if (factorial_add(0)!=0)
        return 1;
    if (factorial_add(2)!=3)
        return 2;
    if (factorial_add(20)!=210)
        return 3;
    if (factorial_mult(1)!=1)
        return 4;
    if (factorial_mult(2)!=2)
        return 5;
    if (factorial_mult(3)!=6)
        return 6;
    if (factorial_mult(6)!=720)
        return 7;
    if (factorial_mult(11)!=0x02611500)
        return 8;
    if (factorial_mult(12)!=0x1c8cfc00) // last that fits in 32 bits
        return 9;
    if (factorial_mult(14)!=0x4c3b2800)
        return 10;
    return 0;
}
