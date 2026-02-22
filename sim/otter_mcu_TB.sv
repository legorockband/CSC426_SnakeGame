`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/20/2024 11:25:13 AM
// Design Name: 
// Module Name: otter_mcu_TB
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module otter_mcu_TB();

    logic sRST, sINTR, sCLK;
    logic [31:0] sIOBUS_IN, sIOBUS_OUT, sIOBUS_ADDR;
    logic sIOBUS_WR;
    
    
    otter_mcu UUT(.IOBUS_IN(sIOBUS_IN), .RST(sRST) , .INTR(sINTR), .CLK(sCLK),
    .IOBUS_OUT(sIOBUS_OUT), .IOBUS_ADDR(sIOBUS_ADDR), .IOBUS_WR(sIOBUS_WR));
    
    always #10 sCLK = ~sCLK;
    initial sCLK = 1'b0;
     
    initial begin
        sIOBUS_IN = 32'b1; sINTR = 1'b0; sRST = 1'b1; 
        #20
        sRST = 1'b0;
        #2000;
        sINTR = 1'b1;
        #40;
    
    end
    
    
    
endmodule
