`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/08/2024 04:39:23 PM
// Design Name: Branch Address Generator 
// Module Name: BRANCH_ADR
// Project Name: HW 5
// Description: Takes in an immediate instruction, pc count, and rs1, and outputs the correct location based on the instruction
//////////////////////////////////////////////////////////////////////////////////


module BRANCH_ADR(
    input signed [31:0] J_TYPE,
    input signed [31:0] B_TYPE,
    input signed [31:0] I_TYPE,
    input [31:0] rs1,
    input [31:0] PC,
    output logic [31:0] JALR,
    output logic [31:0] JAL,
    output logic [31:0] BRANCH);
    
    always_comb begin
        BRANCH = B_TYPE + PC;
        JAL = J_TYPE + PC;
        JALR = I_TYPE + rs1;
    end
endmodule
