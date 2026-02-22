`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/01/2024 04:33:34 PM
// Module Name: IMM_GEN
// Project Name: HW4
// Description: Create the immediate with the specific type 
//////////////////////////////////////////////////////////////////////////////////


module IMM_GEN(
    input [31:7] Instruc,
    output logic [31:0] U_TYPE,
    output logic [31:0] I_TYPE,
    output logic [31:0] S_TYPE,
    output logic [31:0] J_TYPE,
    output logic [31:0] B_TYPE);
    
    always_comb begin
        U_TYPE = {Instruc[31:12], {12{1'b0}}};
        I_TYPE = {{21{Instruc[31]}}, Instruc[30:20]}; 
        S_TYPE = {{21{Instruc[31]}}, Instruc[30:25], Instruc[11:7]};
        B_TYPE = {{20{Instruc[31]}}, Instruc[7], Instruc[30:25], Instruc[11:8], 1'b0};
        J_TYPE = {{12{Instruc[31]}}, Instruc[19:12], Instruc[20], Instruc[30:21], 1'b0};
    end
    
endmodule
