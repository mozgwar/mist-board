#include <verilated.h>          // Defines common routines
#include "Vsdram.h"
#include "verilated_vcd_c.h"

#include "edge.h"

#include <iostream>
#include <string>


Vsdram *uut;     // Instantiation of module
Edge DRAM_CLK;
Edge wb_clk;
VerilatedVcdC* tfp = NULL;

vluint64_t main_time = 0;       // Current simulation time
// This is a 64-bit integer to reduce wrap over issues and
// allow modulus.  You can also use a double, if you wish.
double sc_time_stamp () {       // Called by $time in Verilog
    return main_time;           // converts to double, to match
    // what SystemC does
}

void tick()
{
    if (!Verilated::gotFinish())
    {
        if (main_time > 32)
        {
            uut->RESET = 0;   // Deassert reset
        }

        uut->DRAM_CLK = uut->DRAM_CLK ? 0 : 1;       // Toggle clock
        DRAM_CLK.Update(uut->DRAM_CLK);
        
        if ((main_time % 4) == 0)
        {
            uut->wb_clk = uut->wb_clk ? 0 : 1;       // Toggle clock
            wb_clk.Update(uut->wb_clk);
        }

        uut->eval();            // Evaluate model

        if (tfp != NULL)
        {
            tfp->dump(main_time);
        }

        main_time++;            // Time passes...
    }
}

void wait_ready()
{   
    while (!Verilated::gotFinish())
    {
        tick();
        
        if (uut->wb_ready)
        {
            break;
        }
    }
}

void wait_notclk()
{
    while (!Verilated::gotFinish())
    {
        tick();
        if (!wb_clk.PosEdge())
        {
            break;
        }
    }
}


void wait_ack()
{   
    while (!Verilated::gotFinish())
    {
        tick();
        
        if (wb_clk.PosEdge() && uut->wb_ack)
        {
            break;
        }
    }
}

void wait_nack()
{   
    while (!Verilated::gotFinish())
    {        
        if (wb_clk.PosEdge() && !uut->wb_ack)
        {
            break;
        }
        
        tick();
    }
}

int wb_read32(int address)
{
    wait_ready();
    wait_nack();
    
    while (!Verilated::gotFinish())
    {
        if (wb_clk.PosEdge())
        {
            uut->wb_adr = address;
            uut->wb_sel = 0xF;
            uut->wb_cyc = 1;
            uut->wb_stb = 1;
            uut->wb_we  = 0;
            uut->wb_cti = 0;
            break;
        }
        
        tick();            
    }
    
    wait_ack();
    
    uut->wb_cyc = 0;
    uut->wb_stb = 0;
    uut->wb_we  = 0;
    uut->wb_cti = 0;

    
    return uut->wb_dat_o;
}


void wb_read32x4(int address, int result[4])
{
    wait_ready();
    wait_nack();
    uut->wb_cti = 0x2;
   
    while (!Verilated::gotFinish())
    {
        if (wb_clk.PosEdge())
        {
            uut->wb_adr = address;
            uut->wb_sel = 0xF;
            uut->wb_cyc = 1;
            uut->wb_stb = 1;
            uut->wb_we  = 0;
            
            break;
        }
        
        tick();            
    }    

     wait_ack();
    result[0] = uut->wb_dat_o;   
        
    wait_notclk();
     
     wait_ack();
    
     result[1] = uut->wb_dat_o;   

    wait_notclk();

 wait_ack();
    
     result[2] = uut->wb_dat_o;   

    wait_notclk();


 wait_ack();
    
     result[3] = uut->wb_dat_o;   

    wait_notclk();


    uut->wb_cyc = 0;
    uut->wb_stb = 0;
    uut->wb_we  = 0;
    uut->wb_cti = 0;

    return;
}


int main(int argc, char** argv) 
{
    bool vcdTrace = true;
    
    uut = new Vsdram;      // Create instance    

    Verilated::commandArgs(argc, argv);   // Remember args

    uut->eval();
    uut->eval();

    if (vcdTrace)
    {
        Verilated::traceEverOn(true);
        tfp = new VerilatedVcdC;
        uut->trace(tfp, 99);
        std::string vcdname =  "sdram.vcd";
        tfp->open(vcdname.c_str());
    }

    uut->RESET = 1;

    uut->eval();
    
    tick();
    tick();
    wait_ready();

    std::cout << std::hex << wb_read32(0) << std::dec << std::endl;
    std::cout << std::hex << wb_read32(4) << std::dec << std::endl;
    std::cout << std::hex << wb_read32(8) << std::dec << std::endl;
    std::cout << std::hex << wb_read32(12) << std::dec << std::endl;
    
    int result[4] = {0,0};
    wb_read32x4(0, result);

    std::cout << std::hex << result[0] << std::dec << std::endl;
    std::cout << std::hex << result[1] << std::dec << std::endl;
    std::cout << std::hex << result[2] << std::dec << std::endl;
    std::cout << std::hex << result[3] << std::dec << std::endl;


    for (int i=0; i < 64; i++)
    {   
        tick();
    }

    uut->final();               // Done simulating

    if (tfp != NULL)
    {
        tfp->close();
        delete tfp;
    }

    delete uut;
}
