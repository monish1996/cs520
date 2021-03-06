/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"



/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES = 1;
int DISPLAY = 1;

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    case OPCODE_LDR:
    case OPCODE_STR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }
    case OPCODE_CMP:
    {
        printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }
    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    case OPCODE_LOAD:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    {
        printf("%s", stage->opcode_str);
        break;
    }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}



static void
architectural_register_display(const APEX_CPU *cpu){
    printf("\n\t=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\t\n");
    for(int i=0; i<REG_FILE_SIZE; i++){
        if(cpu->regs_valid[i] == 1){
            printf("|\t REG[%-2d] \t|\t Value=%-3d \t|\t Status = VALID \t|\n",i,cpu->regs[i]);
        }else{
            printf("|\t REG[%-2d] \t|\t Value=%-3d \t|\t Status = INVALID \t|\n",i,cpu->regs[i]);
        }
    }
}

static void
display_data_memory(const APEX_CPU *cpu){
    printf("\n\t============== STATE OF DATA MEMORY =============\t\n");
    for(int i=0; i<DATA_MEMORY_SIZE; i++){
        printf("|\t MEM[%-2d] \t|\t Data Value=%-3d \t|\n",i,cpu->data_memory[i]);
    }  
}


/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{

    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn && !cpu->fetch.stage_stalling)//normal execution
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;
            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;
        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        
        /* Copy data from fetch latch to decode latch*/
        if(cpu->fetch.stage_stalling == FALSE){
            cpu->pc += 4;
            cpu->decode = cpu->fetch;
        }
 
        
        if (ENABLE_DEBUG_MESSAGES)
        {

            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if(cpu->fetch.opcode == OPCODE_HALT){
            cpu->fetch.has_insn = FALSE;
        }
    }
    else if(cpu->fetch.has_insn == TRUE && cpu->fetch.stage_stalling == TRUE){
         /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;
            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        
        
        cpu->fetch.has_insn = FALSE;
        
        if (ENABLE_DEBUG_MESSAGES)
        {

            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if(cpu->fetch.opcode == OPCODE_HALT){
            cpu->fetch.has_insn = FALSE;
        }
    }
    else if(cpu->fetch.has_insn == FALSE && cpu->fetch.stage_stalling == FALSE){
        cpu->fetch.has_insn = TRUE;
        cpu->decode =cpu->fetch;
        cpu->pc +=4;
    }

}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_LDR:
            {
                //printf("valid fields for rs1 is %d and valid field for rs2 is %d\n",cpu->regs_valid[cpu->decode.rs1] , cpu->regs_valid[cpu->decode.rs2]);
                if (cpu->regs_valid[cpu->decode.rs1] == 1 && cpu->regs_valid[cpu->decode.rs2] == 1)
                {
                   // printf("No stalling\n");
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.stage_stalling = FALSE;
                    cpu->fetch.stage_stalling = FALSE;
                    cpu->regs_valid[cpu->decode.rd] = 0;
                }
                else
                {
                   // printf("stalling decode stage \n");
                    cpu->decode.stage_stalling = TRUE;
                    cpu->fetch.stage_stalling = TRUE;
                }
                break;
            }
        case OPCODE_CMP:
        case OPCODE_STR:
            {
            
            if (cpu->regs_valid[cpu->decode.rs1] == 1  && cpu->regs_valid[cpu->decode.rs2] == 1)
                {
                   // printf("No stalling\n");
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.stage_stalling = FALSE;
                    cpu->fetch.stage_stalling = FALSE;
                }
                else
                {
                    //printf("stalling decode stage \n");
                    cpu->decode.stage_stalling = TRUE;
                    cpu->fetch.stage_stalling = TRUE;
                }
                break;
            }
        case OPCODE_LOAD:
        case OPCODE_SUBL:
        case OPCODE_ADDL:
            {
                if(cpu->regs_valid[cpu->decode.rs1] == 1){
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.stage_stalling = FALSE;
                    cpu->fetch.stage_stalling = FALSE;
                    cpu->regs_valid[cpu->decode.rd] = 0;
                }else
                {
                   // printf("stalling decode stage \n");
                    cpu->decode.stage_stalling = TRUE;
                    cpu->fetch.stage_stalling = TRUE;
                }
                break;
            }
        case OPCODE_STORE:
            {
                if(cpu->regs_valid[cpu->decode.rs1] == 1){
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.stage_stalling = FALSE;
                    cpu->fetch.stage_stalling = FALSE;
                }else
                {
                    cpu->decode.stage_stalling = TRUE;
                    cpu->fetch.stage_stalling = TRUE;
                }
                break;
            }

        case OPCODE_BNZ:
            {
                break;
            }
        case OPCODE_BZ:
            {
                break;
            }
        case OPCODE_MOVC:
            {
                cpu->regs_valid[cpu->decode.rd] = 0;
                /* MOVC doesn't have register operands */
                break;
            }
        }
        /* Copy data from decode latch to execute latch*/
        if(cpu->decode.stage_stalling == FALSE){
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }

}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
        case OPCODE_ADD:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_SUB:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_MUL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }

        case OPCODE_DIV:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value / cpu->execute.rs2_value;
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_ADDL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_SUBL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
            break;
        }
        case OPCODE_AND:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
            
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_OR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
            
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_XOR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
            
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_STR:{
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
            break;
        }
        case OPCODE_LDR:{
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
            break;
        }
        case OPCODE_STORE:{
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_LOAD:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_CMP:
        {
            if(cpu->execute.rs1_value == cpu->execute.rs2_value){
                cpu->zero_flag = TRUE;
            }else{
               cpu->zero_flag = FALSE;             }
        }

        case OPCODE_BZ:
        {
            if (cpu->zero_flag == TRUE)
            {
                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            }
            break;
        }

        case OPCODE_BNZ:
        {
            if (cpu->zero_flag == FALSE)
            {
                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            }
            break;
        }

        case OPCODE_MOVC:
        {
            cpu->execute.result_buffer = cpu->execute.imm;
            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    //printf("memory pc instruction :%d\n",cpu->pc);
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
        case OPCODE_ADDL:
        case OPCODE_ADD:
        {
            /* No work for ADD */
            break;
        }
        case OPCODE_SUBL:
        case OPCODE_SUB:
        {
            /* No work for SUB */
            break;
        }
        case OPCODE_DIV:
        {
            /* No work for DIV */
            break;
        }   
        case OPCODE_MUL:
        {
            /* No work for MUL */
            break;
        }
        case OPCODE_STORE:
        case OPCODE_STR:
        {  
            //printf("STORE value %d at memory address %d\n",cpu->regs[cpu->memory.rd],cpu->memory.memory_address);
            cpu->data_memory[cpu->memory.memory_address] = cpu->regs[cpu->memory.rd];
            break;
        } 
        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
            /* No work for LDR */
            //printf("Load value from data memory %d\n",cpu->data_memory[cpu->memory.memory_address]);
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
           // printf("Load value from data memory %d\n",cpu->memory.result_buffer);
            break;
        }   
        
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    //printf("wb instruction :%d\n",cpu->pc);
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
        case OPCODE_ADDL:
        case OPCODE_ADD:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
        case OPCODE_SUBL:
        case OPCODE_SUB:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
        case OPCODE_MUL:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
        case OPCODE_DIV:
        {
            
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;            
        }
        case OPCODE_STORE:
        case OPCODE_STR:
        {
            //no operation
            break;
        }
        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
           cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
           cpu->regs_valid[cpu->writeback.rd] = 1;
           break;
        }  
        case OPCODE_AND:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
        case OPCODE_OR:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
         case OPCODE_XOR:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }

        case OPCODE_MOVC:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;
            break;
        }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename, const char *keywords,const int cycles)
{
    if(strcmp(keywords,"simulate")==0){
       ENABLE_DEBUG_MESSAGES = 0;
       DISPLAY = 1;
    }
    else if(strcmp(keywords,"display")==0){
       ENABLE_DEBUG_MESSAGES = 1;
       DISPLAY = 1;
    }
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    for (int i = 0; i < REG_FILE_SIZE; i++)
    {
        cpu->regs_valid[i] = 1;
    }

    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    cpu->code_memory_size = cycles;
    
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu)
{
    //char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);
        //print_reg_file(cpu);

        // if (cpu->single_step)
        // {
        //     printf("Press any key to advance CPU Clock or <q> to quit:\n");
        //     scanf("%c", &user_prompt_val);

        //     if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
        //     {
        //         printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
        //         break;
        //     }
        // }
        cpu->clock++;
    }
    if(DISPLAY){
        architectural_register_display(cpu);
        display_data_memory(cpu);
    }
}


/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}