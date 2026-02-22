`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 01/19/2024 04:18:23 PM
// Module Name: Program_Counter
// Project Name: HW2 
// Description: Top level module that connects the MUX and PC_REG together 
// Revision 0.01 - File Created
//////////////////////////////////////////////////////////////////////////////////


module Program_Counter(          
    input [31:0] JALR,
    input [31:0] Branch,
    input [31:0] JAL,
    input [31:0] MTVEC,
    input [31:0] MEPC,
    input [2:0] PC_SEL,
    input PC_RST,
    input PC_WE,
    input clk, 
    output logic [31:0] PC_4,        //This is the PC + 4 input for the mux and the output for the top level module
    output logic [31:0] PC_COUNT);
    
    logic [31:0] Mux_PC;
    assign PC_4 = PC_COUNT + 4;
    
    PC_REG PC (.PC_DIN(Mux_PC), .PC_RST(PC_RST), .PC_WE(PC_WE), .clk(clk), .PC_COUNT(PC_COUNT));
    
    MUX MUX0 (.PC_4(PC_4), .JALR(JALR), .Branch(Branch), .JAL(JAL), .MTVEC(MTVEC), .MEPC(MEPC), .PC_SEL(PC_SEL), .out(Mux_PC));
    
    
    
endmodule
