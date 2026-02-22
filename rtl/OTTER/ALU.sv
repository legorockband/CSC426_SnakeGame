`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/01/2024 03:28:08 PM
// Module Name: ALU
// Project Name: HW4
// Description: A large mux which selects specific operations like add, subtract, and or
//////////////////////////////////////////////////////////////////////////////////


module ALU(
    input logic [31:0] srcA,
    input logic[31:0] srcB,
    input [3:0] alu_fun,
    output reg [31:0] alu_result);
    
    logic signed [31:0] srcAs, srcBs;          //for signed values
    always_comb begin
    srcAs = srcA;
    srcBs = srcB;
        case(alu_fun)
            4'b0000: begin                //ADD
//                if(srcAs > alu_result || srcBs >= alu_result)
//                    alu_result = 'hFFFFFFFF;
//                else
                    alu_result = srcAs + srcBs;
            end
            
            4'b0001:                //SLL
                alu_result = srcA << srcB[4:0];
                
            4'b0010: begin                //SLT
                if (srcAs < srcBs)
                    alu_result = 1'b1;
                else 
                    alu_result = 1'b0;
            end
            
            4'b0011: begin               //SLTU
                if (srcA < srcB) 
                    alu_result = 1'b1;
                else
                    alu_result = 1'b0;
            end
            4'b0100:                //XOR
                alu_result = srcA ^ srcB;
            
            4'b0101:                //SRL
                alu_result = srcA >> srcB[4:0];
            
            4'b0110:                //OR
                alu_result = srcA | srcB;
            
            4'b0111:                //AND    
                alu_result = srcA & srcB;
            
            4'b1000: begin                //SUB                     
              alu_result = srcAs - srcBs;
            end
            
            4'b1001:                //LUI-COPY
                alu_result = srcA;
            
            4'b1101: begin               //SRA
                alu_result = srcAs >>> srcB[4:0]; 
            end
            
            default: alu_result = 1'b0;
         endcase
    end
endmodule
