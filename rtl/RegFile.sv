`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 01/24/2024 03:58:20 PM
// Module Name: RegFile
// Project Name: HW 3 
// Description: A device that takes data and stores it to a specificed register from the input. 
//////////////////////////////////////////////////////////////////////////////////


module RegFile(
    input RF_en,
    input [4:0] adr1,
    input [4:0] adr2,
    input [4:0] w_adr,          
    input [31:0] w_data,        
    input clk,
    output reg [31:0] rs1,
    output reg [31:0] rs2
    );
    
    logic [31:0] memory [0:31];
    initial begin    
        for (int i = 0; i < 32; i = i +1)
            memory[i] = 0;                  //initialize all addresses to 0 
    end
    
    always_comb begin
        rs1 = memory[adr1];                //set rs1 to the right register from memory 
        rs2 = memory[adr2];
    end
    
    always_ff @(posedge clk) begin 
        memory[0] <= 0;
        if (RF_en == 1'b1 && w_adr != 0) begin
            memory[w_adr] <= w_data;    // if the write address isn't zero write the data
        end
      
    end
endmodule