`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 01/19/2024 08:28:32 AM
// Module Name: PC_REG
// Project Name: HW 2 
// Description: A register with RST and enable inputs and takes a 32 bit input 
// Revision 0.01 - File Created
//////////////////////////////////////////////////////////////////////////////////

module PC_REG(
    input [31:0] PC_DIN, 
    input PC_RST,
    input PC_WE, 
    input clk,
    output logic [31:0] PC_COUNT);
    
    always_ff @(posedge clk) begin
        if (PC_RST == 1'b1) 
            PC_COUNT <= 0;
        
        else if (PC_WE == 1'b1) 
            PC_COUNT <= PC_DIN;
                
    end
    

endmodule 
