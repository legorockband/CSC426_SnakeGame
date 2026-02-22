`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/19/2024 07:40:32 PM
// Design Name: Control Unit: Finite State Machine
// Module Name: CU_FSM
// Project Name: HW6
// Description: An FSM that controls the enables functions of the MCU. 
//////////////////////////////////////////////////////////////////////////////////


module CU_FSM(
    input RST,
    input INTR_mstatus,
//    input CSR_mstatus,
    input [6:0] opcode,
    input [2:0] funct3,
    input clk,
    output logic PC_WE, output logic RF_WE,
    output logic memWE2, output logic memRDEN1, output logic memRDEN2,
    output logic reset,
    output logic csr_WE, output logic int_taken, output logic mret_exec);
    
    typedef enum {ST_INIT, ST_FETCH, ST_EXEC, ST_WB, ST_INTR} State_type;
    
    State_type NS,PS;
    
    always_ff @ (posedge clk) begin
        if(RST == 1'b1)
            PS <= ST_INIT;
        else
            PS <= NS;    
    end
    
    always_comb begin 
        PC_WE = 1'b0;
        RF_WE = 1'b0;
        memWE2 = 1'b0;
        memRDEN1 = 1'b0;
        memRDEN2 = 1'b0;
        reset = 1'b0;
        csr_WE = 1'b0;
        int_taken = 1'b0;
        mret_exec = 1'b0;
        case(PS) 
            ST_INIT: begin
                NS = ST_FETCH; 
                reset = 1'b1;
            end
            
            ST_FETCH: begin
                NS = ST_EXEC; 
                memRDEN1 = 1'b1;    
            end
            
            ST_EXEC: begin
                if(opcode == 7'b0000011)    // If there is a load then go to Write Back
                    NS = ST_WB;
                
                else if(opcode != 7'b0000011 && INTR_mstatus == 1'b1)   // Interrup Happen 
                    NS = ST_INTR;
                
                else
                    NS = ST_FETCH;
                    
                case(opcode) 
                    7'b0110111: begin       // LUI
                        RF_WE = 1'b1; 
                        PC_WE = 1'b1;
                    end
                    
                    7'b0010111: begin       // AUIPC
                       RF_WE = 1'b1; 
                       PC_WE = 1'b1;
                    end
                    
                    7'b1101111: begin       // JAL
                        PC_WE = 1'b1;
                        RF_WE = 1'b1;
                    end
                    
                    7'b1100111: begin       // JALR
                        PC_WE = 1'b1;
                        RF_WE = 1'b1;
                    end
                    
                    7'b0000011: begin       // Load Instructions 
                        memRDEN2 = 1'b1;
                    end


                    7'b0010011: begin       // Operation Instructions on Immediate
                        RF_WE = 1'b1; 
                        PC_WE = 1'b1;
                    end
                    
                    7'b1100011: begin       // Branch Instructions
                        PC_WE = 1'b1;
                    end 
                    
                    7'b0100011: begin       // Store Instructions 
                        PC_WE = 1'b1;
                        memWE2 = 1'b1;
                    end
                    
                    7'b0110011: begin       // Operation instructions on registers 
                        RF_WE = 1'b1;
                        PC_WE = 1'b1;
                    end
                    
                    
                    7'b1110011: begin       // CSR Instructions
                        
                        if(funct3 == 3'b000) begin   //MRET 
                            mret_exec = 1'b1;
                            PC_WE = 1'b1;
                        end
                        
                        else begin               //CSRRW, CSRRC, CSRRS
                             PC_WE = 1'b1;
                             RF_WE = 1'b1;
                             csr_WE = 1'b1;                       
                        end
                    
                    end
                    
                    default: begin
                        PC_WE = 1'b1;
                        RF_WE = 1'b1;
                        memWE2 = 1'b1;
                        memRDEN1 = 1'b1;
                        memRDEN2 = 1'b1;
                        reset = 1'b1;
                        csr_WE = 1'b1;
                        int_taken = 1'b1;
                        mret_exec = 1'b1;
                    end 
                       
                endcase
            end 
            
            ST_WB: begin
                if (INTR_mstatus == 1'b1)
                    NS = ST_INTR;
                else begin
                    NS = ST_FETCH;
                    PC_WE = 1'b1;
                    RF_WE = 1'b1;
                end
                
            end
            
            ST_INTR: begin
                NS = ST_FETCH;
                PC_WE = 1'b1;
                int_taken = 1'b1;
            end
            
            default: 
                NS = ST_INIT;
        endcase        
    end
endmodule
