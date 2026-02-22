`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly CPE 233
// Engineer: Troy Renner
// 
// Create Date: 02/19/2024 10:06:32 PM
// Module Name: otter_mcu
// Project Name: HW6
// Description: A combination of multiple components used in the OTTER MCU to form the MCU  
//////////////////////////////////////////////////////////////////////////////////


module otter_mcu(
    input [31:0] IOBUS_IN,
    input RST,
    input INTR,
    input CLK,
    output logic [31:0] IOBUS_OUT,
    output logic [31:0] IOBUS_ADDR,
    output logic IOBUS_WR);
    
    logic [31:0] jalr, jal, branch;         // Used for the mux next to PC
    
    logic [31:0] PC_OUTPUT;                 // The output signal from the PC
    logic [31:0] PC_4_OUTPUT;                 // The output signal from the PC

    
    logic [31:0] IR;                        // Output machine code from MEM_DOUT1
    logic [31:0] sDOUT2;                    // Output register for MEM_DOUT2
    
    logic [31:0] RS1, RS2;                  // Output resigter values from the RegFile
    logic [31:0] sw_data;                   // Output from the mux going into the RegFile
    
    logic [31:0] U_TYPE_IMM, I_TYPE_IMM, 
    S_TYPE_IMM, J_TYPE_IMM, B_TYPE_IMM;     // All of the immediate values from IMM_GEN
        
    logic sEQ, sLT, sLTU;                   // Branch Condition output signals    
    
    logic [31:0] ALU_srcB, ALU_srcA;        // The outputs from the muxs before the ALU
    
    logic [3:0] sALU_FUN;                   // Outputs from the Decoder
    logic [2:0] sPC_SEL, SsrcB_SEL;
    logic [1:0] SsrcA_SEL, sRF_SEL;    
    
    logic sPC_WE, sRF_WE, smemWE2,          // FSM Output signals 
    smemRDEN1, smemRDEN2, PC_RESET, 
    scsr_WE, sint_taken, smret_exec;

    
    logic [31:0] sALU_RESULT;               // Result from the ALU    
    
    logic [31:0] sMEPC, sMTVEC, sCSR_RD;             // Result from CSR for MEPC and MTVEC
    
   
    
    CSR csr_module(.reset(PC_RESET), .mret_exec(smret_exec), .int_taken(sint_taken), .CSR_ADDR(IR[31:20]),  
    .CSR_WE(scsr_WE), .PC(PC_OUTPUT), .WD(sALU_RESULT), .clk(CLK), 
    .CSR_mstatus_3(sMSTATUS_3), .CSR_mepc(sMEPC), .CSR_mtvec(sMTVEC), .RD(sCSR_RD));
    
    
    BRANCH_ADR BRANCH_ADR(.J_TYPE(J_TYPE_IMM), .B_TYPE(B_TYPE_IMM), .I_TYPE(I_TYPE_IMM), .rs1(RS1), .PC(PC_OUTPUT),
    .JALR(jalr), .JAL(jal), .BRANCH(branch));
    
    
    
    Program_Counter PC_Module(.JALR(jalr), .Branch(branch), .JAL(jal), .MTVEC(sMTVEC), .MEPC(sMEPC), .PC_SEL(sPC_SEL), .PC_RST(PC_RESET), .PC_WE(sPC_WE), .clk(CLK),
    .PC_4(PC_4_OUTPUT), .PC_COUNT(PC_OUTPUT));
    
    
    
    Memory mem(.MEM_CLK(CLK), .MEM_RDEN1(smemRDEN1), .MEM_RDEN2(smemRDEN2), . MEM_WE2(smemWE2), 
    .MEM_ADDR1(PC_OUTPUT[15:2]), .MEM_ADDR2(sALU_RESULT), .MEM_DIN2(RS2), .MEM_SIZE(IR[13:12]), .MEM_SIGN(IR[14]), .IO_IN(IOBUS_IN),
    .IO_WR(IOBUS_WR), .MEM_DOUT1(IR), .MEM_DOUT2(sDOUT2));
    
    
    always_comb begin
        case(sRF_SEL)
            2'b00: sw_data = PC_4_OUTPUT;
            2'b01: sw_data = sCSR_RD;                 // Unknown signal for now
            2'b10: sw_data = sDOUT2;
            2'b11: sw_data = sALU_RESULT;
        endcase
    end
    
    RegFile RF(.RF_en(sRF_WE), .adr1(IR[19:15]), .adr2(IR[24:20]), .w_adr(IR[11:7]), .w_data(sw_data), .clk(CLK),
    .rs1(RS1), .rs2(RS2)); 
    
    
    
    IMM_GEN immediate_gen(.Instruc(IR[31:7]), 
    .U_TYPE(U_TYPE_IMM), .I_TYPE(I_TYPE_IMM), . S_TYPE(S_TYPE_IMM), .J_TYPE(J_TYPE_IMM), .B_TYPE(B_TYPE_IMM));
    
    
    
    BRANCH_COND branch_condition_generator(.rs1(RS1), .rs2(RS2), 
    .br_eq(sEQ), .br_lt(sLT), .br_ltu(sLTU));
    
    
    
    CU_DCDR CU_Decoder(.funct3(IR[14:12]), .opcode(IR[6:0]), .ir_30(IR[30]), .int_taken(sint_taken), 
    .br_eq(sEQ), .br_lt(sLT), .br_ltu(sLTU), 
    .ALU_FUN(sALU_FUN), .srcA_SEL(SsrcA_SEL), .srcB_SEL(SsrcB_SEL), .PC_SEL(sPC_SEL), .RF_SEL(sRF_SEL));
    
    
    assign INTR_ANDED = INTR & sMSTATUS_3;
    
    CU_FSM CU_Finite_State(.RST(RST), .INTR_mstatus(INTR_ANDED), .opcode(IR[6:0]), .funct3(IR[14:12]), .clk(CLK), 
    .PC_WE(sPC_WE), .RF_WE(sRF_WE), .memWE2(smemWE2), .memRDEN1(smemRDEN1), .memRDEN2(smemRDEN2), .reset(PC_RESET), 
    .csr_WE(scsr_WE), .int_taken(sint_taken), .mret_exec(smret_exec));
    
    
    
    always_comb begin
        case(SsrcA_SEL)
            2'b00: ALU_srcA = RS1;
            2'b01: ALU_srcA = U_TYPE_IMM;
            2'b10: ALU_srcA = ~RS1;
            default: ALU_srcA = 3'bzzz;
        endcase
        
        case(SsrcB_SEL)
            3'b000: ALU_srcB = RS2;
            3'b001: ALU_srcB = I_TYPE_IMM;
            3'b010: ALU_srcB = S_TYPE_IMM;
            3'b011: ALU_srcB = PC_OUTPUT;
            3'b100: ALU_srcB = sCSR_RD;               
            default: ALU_srcB = 3'bzzz;
        endcase
    end
    
    
    ALU alu(.srcA(ALU_srcA), .srcB(ALU_srcB), .alu_fun(sALU_FUN), 
    .alu_result(sALU_RESULT));
    
    assign IOBUS_ADDR = sALU_RESULT;
    assign IOBUS_OUT = RS2; 
    
endmodule
