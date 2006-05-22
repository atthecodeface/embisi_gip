//  gcc sine_table.c -o sines -lm -Wall
// The theory is that you store a set of steps in a single ROM word
// The ROM word has a full value for a coarse resolution
// plus 1, 3, 7, or 15 deltas for fine resolutions

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define PI (3.14159265)
#define STEPS (14)
#define MAX_STEPS_PER_ROM (4)
#define RESOLUTION (15)

typedef struct
{
    unsigned int base_value;
    unsigned int delta;
    unsigned long error_bits;
} t_rom_entry;

static t_rom_entry rom[8*1024];

static int sine_from_rom( int sine_resolution,
                          int input_resolution,
                          int log_substeps,
                          int phase_in )
{
    int quadrature;
    int phase;

    int coarse_phase;
    int fine_phase;

    unsigned int base_value;
    unsigned int delta;
    unsigned long error_bits;

    int delta_sum;
    int carry_in;
    int value;

    phase = phase_in &~ ((-1)<<input_resolution);
    quadrature = phase>>(input_resolution-2);
    phase = phase &~ ((-1)<<(input_resolution-2));
    if (quadrature&1) phase=~phase; // phase = (1<<(input_resolution-2))-1-phase, which is just an arithmetic not
    phase = phase &~ ((-1)<<(input_resolution-2));

    coarse_phase = phase>>log_substeps;
    fine_phase = phase ^ (coarse_phase<<log_substeps);

    base_value = rom[coarse_phase].base_value;
    delta      = rom[coarse_phase].delta;
    error_bits = rom[coarse_phase].error_bits;
    
    delta_sum = 0;
    if ((log_substeps>0) && ((fine_phase>>(log_substeps-1))&1))
        delta_sum = delta<<1;
    if ((log_substeps>1) && ((fine_phase>>(log_substeps-2))&1))
        delta_sum += delta;
    if ((log_substeps>2) && ((fine_phase>>(log_substeps-3))&1))
        delta_sum += delta>>1;
    if ((log_substeps>3) && ((fine_phase>>(log_substeps-4))&1))
        delta_sum += delta>>2;
    if ((log_substeps>4) && ((fine_phase>>(log_substeps-5))&1))
        delta_sum += delta>>3;
    if ((log_substeps>5) && ((fine_phase>>(log_substeps-6))&1))
        delta_sum += delta>>4;

    carry_in = (error_bits>>((1<<log_substeps)-1-fine_phase))&1;

    int rom_value = base_value*2 + (delta*fine_phase)/(1<<(log_substeps-1));
    if (delta_sum&1) carry_in++;
    value = (base_value<<1) + (delta_sum>>1) + carry_in;

    if (quadrature&2)
        value = -value;

    if (0)
    {
        printf("Phase in %04x, fine_phase %04x, coarse phase %02x, base_value %04x, delta_sum %02x, carry in %01x, value %04x (%04x)\n",
               phase_in,
               fine_phase,
               coarse_phase,
               base_value,
               delta_sum,
               carry_in,
               value, rom_value );
    }

    return value;
}

static void test_rom( int sine_resolution, // include sign bit
                       int input_resolution, // include both quadrature bits too
                      int log_substeps,
                      int range_reduction )
{
    int i, total_error_count;
    total_error_count = 0;
    for (i=0; i<(1<<input_resolution); i++)
    {
        double wt;
        int exp_value, rom_value, error;
        wt = ((i+0.5)/(1<<(input_resolution-2))) * PI/2.0;
        exp_value = sin(wt)*((1<<(sine_resolution-1))-range_reduction);
        rom_value = sine_from_rom( sine_resolution, input_resolution, log_substeps, i );
        error = exp_value - rom_value;
        if (error!=0)
        {
            //printf( "%04x: %04x : %04x\n", i, rom_value, exp_value );
            total_error_count ++;
        }
    }
    printf("Total errors %d\n", total_error_count );
}

static void build_rom( int sine_resolution, // include sign bit
                       int input_resolution, // include both quadrature bits too
                       int log_substeps,
                       int range_reduction, // tune to get fewest errors if required
                       FILE *print_rom_file )
{
    int rom_depth;
    int rom_base_resolution;
    int error_count;
    int max_delta;
    int i, j;
    int total_error_count;

    int range;

    rom_depth = input_resolution-2-log_substeps;
    rom_base_resolution = sine_resolution-2;
    range = (1<<(rom_base_resolution+1))-range_reduction;

    total_error_count = 0;
    max_delta = 0;
    for (i=0; i<(1<<rom_depth); i++)
    {
        double wt;
        int rom_base_value, rom_centre_value, rom_delta;
        int error;
        unsigned long error_bits;
        int exp_value;
        int rom_value;
        int rbo, rdo, best_rbo, best_rdo, min_error_count;
        // i corresponds to a quarter sine wave position i/(1<<(rom_depth+log_substeps))
        // i of 0 then is actually expecting to yield sin((i+0.5)/(1<<(rom_depth+log_substeps)))
        wt = (((i<<log_substeps)+0.5)/(1<<(rom_depth+log_substeps))) * PI/2.0;
        rom_base_value = sin(wt)*range/2;
        wt = (((i<<log_substeps)+(1<<(log_substeps-1))+0.5)/(1<<(rom_depth+log_substeps))) * PI/2.0;
        rom_centre_value = sin(wt)*range; // one more bit here, as this will not be soaked up by the error bit
        rom_delta = rom_centre_value-2*rom_base_value; // so that delta has one more bit too
        min_error_count = 32;
        for (rbo=-2; rbo<3; rbo++)
        {
            for (rdo=-5; rdo<6; rdo++)
            {
                if (rom_delta+rdo<0) continue;
                error_count = 0;
                for (j=0; j<(1<<log_substeps); j++)
                {
                    int k;
                    int delta_sum;
                    wt = (((i<<log_substeps)+j+0.5)/(1<<(rom_depth+log_substeps))) * PI/2.0;
                    exp_value = sin(wt)*range;
                    delta_sum = 0;
                    for (k=0; k<log_substeps; k++)
                        if ((j>>(log_substeps-1-k))&1)
                            delta_sum += ((rom_delta+rdo)<<1)>>k;
                    if (delta_sum&1) delta_sum+=1;
                    delta_sum>>=1;
                    rom_value = (rom_base_value+rbo)*2 + delta_sum;
                    error = exp_value - rom_value;
                    if ((error<0) || (error>1)) error_count++;
                }
                if (error_count<min_error_count)
                {
                    best_rbo = rbo;
                    best_rdo = rdo;
                    min_error_count = error_count;
                }
            }
        }
        //printf("Best rbo %d rdo %d\n", best_rbo, best_rdo );
        rom_base_value += best_rbo;
        rom_delta += best_rdo;
        error_count = 0;
        error_bits = 0;
        for (j=0; j<(1<<log_substeps); j++)
        {
            int k;
            int delta_sum;
            wt = (((i<<log_substeps)+j+0.5)/(1<<(rom_depth+log_substeps))) * PI/2.0;
            exp_value = sin(wt)*range;
            delta_sum = 0;
            for (k=0; k<log_substeps; k++)
                if ((j>>(log_substeps-1-k))&1)
                    delta_sum += (rom_delta<<1)>>k;
            if (delta_sum&1) delta_sum+=1;
            delta_sum>>=1;
            rom_value = rom_base_value*2 + delta_sum;
            error = exp_value - rom_value;
            if ((error<0) || (error>1)) error_count++;
            if ((error<0) || (error>1))
            {
                //printf( "%04x: %2d : %04x : %04x : %04x : %04x\n", i, j, rom_base_value, rom_delta, rom_value, exp_value );
            }
            error_bits <<= 1;
            if (error>0) error_bits |= 1;
        }
        if (rom_delta>max_delta) max_delta=rom_delta; // remember this as we have to store this many bits!
        //printf("Rom entry %4x has total error count %d\n", i, error_count);
        if (print_rom_file)
        {
            fprintf( print_rom_file, "{0x%04x, 0x%02x, 0x%08lx}, //%04x \n", rom_base_value, rom_delta, error_bits, i );
        }
        rom[i].base_value = rom_base_value;
        rom[i].delta = rom_delta;
        rom[i].error_bits = error_bits;
        total_error_count += error_count;
    }
    for (i=max_delta, j=0; i>0; i>>=1,j++);
    printf("Base resolution %d, substeps %d, total error count %d max delta %d, rom %d by %d\n", rom_base_resolution, 1<<log_substeps, total_error_count, max_delta, 1<<(input_resolution-2-log_substeps), rom_base_resolution+(1<<log_substeps)+j);
}

extern int main( int argc, char **argv )
{
    int steps_per_rom;
    int rom_depth, rom_width, rom_size;
    int i, j;
    int last_j;
    int base, offset, max_offset;
    int print;
    int substeps;
    double s, wt;
    int total_errors, max_error;

    //build_rom( 16, 16, 4, stdout );
    //build_rom( 16, 16, 3, NULL );
    //build_rom( 16, 16, 2, NULL );
    //build_rom( 15, 16, 4, NULL );
    FILE *f = fopen("rom.txt","w");
    build_rom( 16, 16, 4, 1, f );
    test_rom( 16, 16, 4, 1 );
    exit(0);
    print = 0;
    for (substeps=0; substeps<7; substeps++)
    {
        for (steps_per_rom=1; steps_per_rom<=(1<<MAX_STEPS_PER_ROM); steps_per_rom<<=1)
        {
            max_offset = -1;
            base = 0;
            max_error = -1;
            total_errors = 0;
            for (i=0; i<(1<<STEPS); i++)
            {
                wt = ((i+0.5)/(1<<STEPS)) * PI/2.0;
                s = sin(wt);
                j = (int) (s*(1<<RESOLUTION));
                if ((i%(steps_per_rom<<substeps))==0)
                {
                    if (substeps)
                    {
                        offset = j-base;
                        if (offset>max_offset)
                        {
                            max_offset = offset;
                        }
                    }
                    base = j;
                }
                if (i%(1<<substeps)==0)
                {
                    offset = j-base;
                    if (offset>max_offset)
                    {
                        max_offset = offset;
                    }
                    last_j = j;
                }
                else
                {
                    int next_step_i, next_j, calc_j, error;
                    double next_wt, next_s;
                    double posn;
                    // Find error between j and the interp of the last to the next
                    next_step_i = (1<<substeps)*(1+(i/(1<<substeps)));
                    posn = (((i%(1<<substeps))+0.0)/(1<<substeps));
                    next_wt = ((next_step_i+0.5)/(1<<STEPS)) * PI/2.0;
                    next_s = sin(next_wt);
                    next_j = (int) (next_s*(1<<RESOLUTION));
                    calc_j = 4*posn*next_j + 4*(1-posn)*last_j;
                    error = (4*j-calc_j);
                    if (error<0) error=-error;
                    //if (error>=2) printf("%4d:Error in %04x of %04x\n", total_errors, i, error );
                    if (error>max_error) max_error=error;
                    if (error!=0) total_errors++;
                }
                if (print) printf("%04x %10.4lf %04x %04x %04x %04x\n", i, s, base, j, offset, max_offset );
            }
            for (i=max_offset, j=0; i>0; i>>=1,j++);
            rom_depth = (1<<STEPS)/(steps_per_rom<<substeps);
            rom_width = RESOLUTION + (steps_per_rom-1)*j;
            if (substeps)
            {
                rom_width = RESOLUTION + steps_per_rom*j;
            }
            rom_size = rom_depth * rom_width;
            printf("Resolution %d, Steps %d, Steps per rom %d, Substeps per rom %d, max offset %02x, max offset bits %d, total errors %d, max error %d, ROM size %dx%d (%4.1lfkbits)\n", RESOLUTION, STEPS, steps_per_rom, 1<<substeps, max_offset, j, total_errors, max_error, rom_depth, rom_width, rom_size/1024.0 );
        }
    }
    return 0;
}
