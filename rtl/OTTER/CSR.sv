`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly Slo CPE 233
// Engineer: Troy Renner
// 
// Create Date: 03/07/2024 11:14:03 AM
// Design Name: Control Service Unit
// Module Name: CSR
// Project Name: MCU
// Description: 
//////////////////////////////////////////////////////////////////////////////////


module CSR(
    input reset,
    input mret_exec,
    input int_taken,
    input [11:0] CSR_ADDR,
    input CSR_WE,
    input [31:0] PC,
    input [31:0] WD,
    input clk,
    output logic CSR_mstatus_3,
    output logic [31:0] CSR_mepc,
    output logic [31:0] CSR_mtvec,
    output logic [31:0] RD);
    
    logic [31:0] csr_reg [0:2];     //Setup 0 = MSTATUS, 1 = MTVEC, and 2 = MEPC registers
    logic [31:0] CSR_mstatus;
    //logic [31:0] temp_CSR_mstatus;   // Used to swap bit 7 and 3 for MSTATUS
    
    initial begin    
        for (int i = 0; i < 3; i = i + 1)
            csr_reg[i] = 0;                  //initialize all addresses to 0 
    end
    
    always_comb begin
        CSR_mstatus = csr_reg[0];
        CSR_mtvec = csr_reg[1];
        CSR_mepc = csr_reg[2];
        
        CSR_mstatus_3 = csr_reg[0][3];

        
        if(CSR_ADDR ==  12'b001100000000) 
            RD = CSR_mstatus;      // MSTATUS
            
        else if(CSR_ADDR == 12'b001100000101)     
            RD = CSR_mtvec;      // MTVEC
            
        else if (CSR_ADDR == 12'b001101000001)
            RD = CSR_mepc;      // MEPC;
            
        else
            RD = 32'b0;
    end
    
    
    always_ff @(posedge clk) begin
        if(reset == 1'b1) begin
            csr_reg[2] <= 32'b0;
            csr_reg[1] <= 32'b0;
            csr_reg[0] <= 32'b0;
            //RD <= 32'b0;
        end
        
        if (int_taken == 1'b1) begin
            csr_reg[2] <= PC;           //MEPC
            
            //temp_CSR_mstatus <= csr_reg[0];      // Temp mstatus for swapping bit 7 with 3. 
            //csr_reg[0][3] <= csr_reg[0][7];     // Swap MSTATUS bit 3 with 7 
            csr_reg[0][7] <= csr_reg[0][3];
            csr_reg[0][3] <= 1'b0; 
        end
        
        else if(mret_exec == 1'b1) begin
            csr_reg[0][3] <= csr_reg[0][7];
            csr_reg[0][7] <= 1'b0;
          end
        
        else if(CSR_WE == 1'b1) begin
            if(CSR_ADDR == 12'b001100000000) begin     // Address for MSTATUS 
                csr_reg[0] <= WD;
//                RD <= CSR_mstatus_3;
            end   
            
            else if(CSR_ADDR == 12'b001100000101) begin     // If the address is for MTVEC
                csr_reg[1] <= WD;
//                RD <= CSR_mtvec;
            end
            
            else if(CSR_ADDR == 12'b001101000001) begin     // If the address is for MEPC
                csr_reg[2] <= WD;
//                RD <= CSR_mepc;
            end
        end  
        
    end
    
endmodule
