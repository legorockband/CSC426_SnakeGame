`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/06/2024 04:05:24 PM
// Design Name: Branch Conditional Generator 
// Module Name: BRANCH_COND
// Project Name: HW5
// Description: Compares two resigters and outputs the correct condition
//////////////////////////////////////////////////////////////////////////////////


module BRANCH_COND(
    input [31:0] rs1,
    input [31:0] rs2,
    output logic br_eq,
    output logic br_lt,
    output logic br_ltu
);
    
    logic signed [31:0] Srs1;
    logic signed [31:0] Srs2;
    
    always_comb begin
        br_lt = 1'b0;
        br_ltu = 1'b0;
        br_eq = 1'b0;
        
        Srs1 = rs1;
        Srs2 = rs2;
        
        if (rs1 == rs2) begin  //Is the unsigned rs1 value equal to unsigned rs2
            br_eq = 1'b1;  
        end
        
        if(rs1[31] == 1'b1 && rs2[31] == 1'b1 && rs1 < rs2)
            br_lt = 1'b1;
        
        else if (rs1[31] == 1'b1 && rs2[31] != 1'b1 && rs1 > rs2)     //If value is signed then compare
            br_lt = 1'b1;
        
        else if (rs1[31] != 1'b1 && rs2[31] != 1'b1 && rs1 < rs2)
            br_lt = 1'b1;
        
                       
        if (rs1 < rs2) begin   //Is the unsigned rs1 value less than unsigned rs2
            br_ltu = 1'b1;  
        end        
    end
endmodule
