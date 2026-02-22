`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 01/19/2024 08:14:45 AM
// Module Name: MUX
// Project Name: HW 2
// Description: A Mux with 6 32 bit inputs and outputs one of the 6 inputs
// Revision 0.01 - File Created
//////////////////////////////////////////////////////////////////////////////////


module MUX(
    input [31:0] PC_4,
    input [31:0] JALR,
    input [31:0] Branch,
    input [31:0] JAL,
    input [31:0] MTVEC,
    input [31:0] MEPC,
    input [2:0] PC_SEL,
    output logic [31:0] out);
        
    always_comb begin
        case (PC_SEL)
            3'b000: out = PC_4;
            3'b001: out = JALR;
            3'b010: out = Branch;
            3'b011: out = JAL;
            3'b100: out = MTVEC;
            3'b101: out = MEPC;
            default: out = 1'b0;
        endcase
    end
endmodule
