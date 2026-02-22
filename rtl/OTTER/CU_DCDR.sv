`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/18/2024 08:06:13 PM
// Design Name: Control Unit Decoder
// Module Name: CU_DCDR
// Project Name: HW6 
// Description: A device that controls all of the mux selectors for all of the other devices 
//////////////////////////////////////////////////////////////////////////////////

module CU_DCDR(
    input [2:0] funct3,
    input [6:0] opcode,
    input ir_30,
    input int_taken,
    input br_eq,
    input br_lt,
    input br_ltu,
    output logic [3:0] ALU_FUN,
    output logic [1:0] srcA_SEL,
    output logic [2:0] srcB_SEL,
    output logic [2:0] PC_SEL,
    output logic [1:0] RF_SEL);
        
    always_comb begin
        ALU_FUN = 1'b0;
        srcA_SEL = 1'b0;
        srcB_SEL = 1'b0;
        PC_SEL = 1'b0;
        RF_SEL = 1'b0;
  
        case(opcode) 
            7'b0110111: begin       //LUI
                ALU_FUN = 4'b1001;      // LUI
                srcA_SEL = 2'b01;       // U TYPE immediate
                RF_SEL = 2'b11;
            end
            
            7'b0010111: begin       // AUIPC
                ALU_FUN = 4'b0000;      // ADD
                srcA_SEL = 2'b01;       // U TYPE immediate
                srcB_SEL = 3'b011;      // PC 
                RF_SEL = 2'b11;
            end
            
            7'b1101111: begin       //JAL
                PC_SEL = 3'b011;
                RF_SEL = 2'b00;
            end
            
            7'b1100111: begin       //JALR
                PC_SEL = 3'b001;
                RF_SEL = 2'b00;
            end
            
            7'b0000011: begin        // All load instructions 
                ALU_FUN = 4'b0000;      // Add
                srcA_SEL = 2'b00;       // Rs1
                srcB_SEL = 3'b001;       // I type immediate 
                RF_SEL = 2'b10;
                end
                
            7'b0010011: begin        // Operation Immediate Instructions
                srcA_SEL = 2'b00;       // Rs1
                srcB_SEL = 3'b001;      // I type immediate 
                RF_SEL = 2'b11;         // Write to mux 3
                case(funct3)
                    3'b000: ALU_FUN = 4'b0000;  //ADDI
                    3'b001: ALU_FUN = 4'b0001;  //SLLI
                    3'b010: ALU_FUN = 4'b0010;  //SLTI
                    3'b011: ALU_FUN = 4'b0011;  //SLTIU
                    3'b100: ALU_FUN = 4'b0100;  //XORI
                    3'b101: begin
                        if (ir_30 == 1'b0)
                            ALU_FUN = 4'b0101;  //SRLI
                        else
                            ALU_FUN = 4'b1101;  //SRAI
                    end
                    3'b110: ALU_FUN = 4'b0110;  //ORI
                    3'b111: ALU_FUN = 4'b0111;  //ANDI
                endcase
            end
            
            7'b1100011: begin        // Branch Instructions
                case(funct3)
                    3'b000: begin       // BEQ
                        if(br_eq == 1'b1)
                            PC_SEL = 3'b010;
                         else
                            PC_SEL = 3'b000;
                    end
                    3'b001: begin       // BNE
                        if(br_eq == 1'b0)
                            PC_SEL = 3'b010;
                        else
                            PC_SEL = 3'b000;
                    
                    end
                    
                    3'b100: begin       // BLT
                        if (br_lt == 1'b1)
                            PC_SEL = 3'b010;
                        else 
                            PC_SEL = 3'b000;
                    end
                    
                    3'b101: begin       //BGE
                        if (br_lt == 1'b0)
                            PC_SEL = 3'b010;
                        else 
                            PC_SEL = 3'b000;
                    end
                    
                    3'b110: begin       //BLTU
                        if (br_ltu == 1'b1)
                            PC_SEL = 3'b010;
                        else 
                            PC_SEL = 3'b000;
                    end
                    
                    3'b111: begin       //BGEU
                        if (br_ltu == 1'b0)
                            PC_SEL = 3'b010;
                        else 
                            PC_SEL = 3'b000;
                    end
                endcase
            end       
            
            7'b0100011: begin       //Store Insturction 
                ALU_FUN = 4'b0000;      // Add
                srcA_SEL = 2'b00;       // Rs1
                srcB_SEL = 3'b010;      // S type immediate 
            end        
            
            7'b0110011: begin        //Operation with just registers 
                srcA_SEL = 2'b00;       // RS1
                srcB_SEL = 3'b000;      // RS2 
                RF_SEL = 2'b11;         // Write to mux 3
                ALU_FUN = {ir_30, funct3};
            end
           
            7'b1110011: begin        // CSR instructions opcode 
                PC_SEL = 3'b000;
                srcA_SEL = 2'b00;
                RF_SEL = 2'b01;
                if(funct3 == 3'b001) begin      //CSRRW
                    ALU_FUN = 4'b1001;
                end
                
                else if(funct3 == 3'b010) begin //CSRRS
                    srcB_SEL = 3'b100;
                    ALU_FUN = 4'b0110;
                end
                
                else if(funct3 == 3'b011) begin //CSRRC
                    srcA_SEL = 2'b10;
                    srcB_SEL = 3'b100;
                    ALU_FUN = 4'b0111;
                end
                
                else begin                      //MRET
                    PC_SEL = 3'b101;
                end
            end
            
            default: begin
                ALU_FUN = 4'b1111; 
                srcA_SEL = 2'b11;       
                srcB_SEL = 3'b111;       
                RF_SEL = 2'b11;
            end     
                    
        endcase
        
        if(int_taken == 1'b1)
            PC_SEL = 3'b100;        // MTVEC
            
    end
    
endmodule
