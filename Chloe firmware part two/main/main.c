#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ecrt.h>

/* EtherCAT CiA 402 Object Dictionary Indexes */
#define EC_OD_CONTROLWORD              0x6040
#define EC_OD_STATUSWORD               0x6041
#define EC_OD_ERROR_CODE               0x603F
#define EC_OD_MODES_OF_OPERATION       0x6060
#define EC_OD_VELOCITY_ACTUAL_VALUE    0x606C
#define EC_OD_MAX_TORQUE               0x6072
#define EC_OD_TARGET_VELOCITY          0x60FF

/* Operation Modes and Standard Values */
#define EC_MODE_CSV                    9       /* Cyclic Sync Velocity mode */
#define EC_ERROR_CODE_NONE             0       /* 0 = No fault present */
#define EC_MAX_TORQUE_TEST_LIMIT       500     /* 500 = 50.0% torque limit for testing */

/* Enable Sequence Controlword Commands */
#define EC_CMD_SHUTDOWN                0x0006  /* Step 1: Write Shutdown */
#define EC_CMD_SWITCH_ON               0x0007  /* Step 2: Write Switch On */
#define EC_CMD_ENABLE_OPERATION        0x000F  /* Step 3: Write Enable Operation */
#define EC_CMD_FAULT_RESET             0x0080  /* Bit 7: Fault reset */

/* Statusword State Verification Values & Mask */
#define EC_STATUS_MASK                 0x006F  /* Mask for statusword evaluation (& 0x006F) */
#define EC_STATUS_READY_TO_SWITCH_ON   0x0231  /* Expected state after Shutdown command */
#define EC_STATUS_SWITCHED_ON          0x0233  /* Expected state after Switch On command */
#define EC_STATUS_OPERATION_ENABLED    0x0237  /* Expected state after Enable Operation command */

#define PRINT_FREQ 1000 // Print output every 1000 cycles (1 second at 1kHz)

#define VENDOR_ID      0x00000000
#define PRODUCT_CODE   0x00000000

int main(void)
{
    // Declare variables mapped to PDO entries
    uint16_t controlword = 0;
    uint16_t statusword = 0;
    uint32_t errorcode = 0;
    int8_t modeofoperation = 0;
    int32_t velocityactualvalue = 0;
    uint16_t maxtorque = 0;
    int32_t target_velocity = 0;

    // Process data offsets for PDO registration
    unsigned int off_controlword;
    unsigned int off_statusword;
    unsigned int off_errorcode;
    unsigned int off_modeofoperation;
    unsigned int off_velocityactualvalue;
    unsigned int off_maxtorque;
    unsigned int off_target_velocity;

    ec_master_t *master = NULL;
    ec_domain_t *domain = NULL;
    ec_slave_config_t *sc = NULL;

    master = ecrt_request_master(0);
    if (!master) {
        fprintf(stderr, "Failed to obtain master 0.\n");
        return 1;
    }

    domain = ecrt_master_create_domain(master);
    if (!domain) {
        fprintf(stderr, "Failed to create domain.\n");
        return 1;
    }

    sc = ecrt_master_slave_config(master, 0, 0, VENDOR_ID, PRODUCT_CODE);
    if (!sc) {
        fprintf(stderr, "Failed to get slave configuration.\n");
        return 1;
    }

    ec_pdo_entry_reg_t domain_regs[] = {
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_CONTROLWORD, 0, &off_controlword},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_STATUSWORD, 0, &off_statusword},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_ERROR_CODE, 0, &off_errorcode},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_MODES_OF_OPERATION, 0, &off_modeofoperation},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_VELOCITY_ACTUAL_VALUE, 0, &off_velocityactualvalue},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_MAX_TORQUE, 0, &off_maxtorque},
        {0, 0, VENDOR_ID, PRODUCT_CODE, EC_OD_TARGET_VELOCITY, 0, &off_target_velocity},
        {}
    };

    if (ecrt_domain_reg_pdo_entry_list(domain, domain_regs)) {
        fprintf(stderr, "PDO entry registration failed!\n");
        return 1;
    }

    if (ecrt_master_activate(master)) {
        fprintf(stderr, "Failed to activate master.\n");
        return 1;
    }

    uint8_t *domain_pd = ecrt_domain_data(domain);
    if (!domain_pd) {
        fprintf(stderr, "Failed to get domain data pointer.\n");
        return 1;
    }

    unsigned long cycle = 0;
    int state_machine_step = 0; // 0: Shutdown, 1: Switch On, 2: Enable Operation, 3: Running
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1 ms interval

    while (1) {
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        
        ecrt_master_receive(master);
        ecrt_domain_process(domain);

        // Read feedback from process data
        statusword = EC_READ_U16(domain_pd + off_statusword);
        errorcode = EC_READ_U32(domain_pd + off_errorcode);
        velocityactualvalue = EC_READ_S32(domain_pd + off_velocityactualvalue);

        // CiA 402 State Machine Sequence Management
        if (state_machine_step == 0) {
            controlword = EC_CMD_SHUTDOWN;
            if ((statusword & EC_STATUS_MASK) == EC_STATUS_READY_TO_SWITCH_ON) {
                state_machine_step = 1;
            } else if (errorcode != EC_ERROR_CODE_NONE) {
                printf("Fault detected during shutdown. Error code: 0x%04X\n", errorcode);
                controlword = EC_CMD_FAULT_RESET;
            }
        } else if (state_machine_step == 1) {
            controlword = EC_CMD_SWITCH_ON;
            if ((statusword & EC_STATUS_MASK) == EC_STATUS_SWITCHED_ON) {
                state_machine_step = 2;
            } else if (errorcode != EC_ERROR_CODE_NONE) {
                printf("Fault detected during switch on. Error code: 0x%04X\n", errorcode);
                controlword = EC_CMD_FAULT_RESET;
                state_machine_step = 0;
            }
        } else if (state_machine_step == 2) {
            controlword = EC_CMD_ENABLE_OPERATION;
            if ((statusword & EC_STATUS_MASK) == EC_STATUS_OPERATION_ENABLED) {
                state_machine_step = 3;
                cycle = 0; // Reset cycle count for active run duration timing
            } else if (errorcode != EC_ERROR_CODE_NONE) {
                printf("Fault detected during enable operation. Error code: 0x%04X\n", errorcode);
                controlword = EC_CMD_FAULT_RESET;
                state_machine_step = 0;
            }
        } else if (state_machine_step == 3) {
            if (errorcode != EC_ERROR_CODE_NONE) {
                printf("Fault detected during operation. Error code: 0x%04X\n", errorcode);
                controlword = EC_CMD_FAULT_RESET;
                state_machine_step = 0;
                target_velocity = 0;
            } else {
                // Run at 60 RPM for 3 seconds (3000 cycles at 1 kHz), then execute controlled exit
                if (cycle < 3000) {
                    target_velocity = (int32_t)(60.0 / 60.0 * 131072);
                } else {
                    target_velocity = 0;
                    controlword = EC_CMD_SWITCH_ON;
                    
                    // Transmit final state before exiting
                    EC_WRITE_U16(domain_pd + off_controlword, controlword);
                    EC_WRITE_S8(domain_pd + off_modeofoperation, EC_MODE_CSV);
                    EC_WRITE_U16(domain_pd + off_maxtorque, EC_MAX_TORQUE_TEST_LIMIT);
                    EC_WRITE_S32(domain_pd + off_target_velocity, target_velocity);
                    
                    ecrt_domain_queue(domain);
                    ecrt_master_send(master);
                    
                    printf("Execution cycle complete. Exiting safely.\n");
                    break;
                }
            }
        }

        modeofoperation = EC_MODE_CSV;
        maxtorque = EC_MAX_TORQUE_TEST_LIMIT;

        EC_WRITE_U16(domain_pd + off_controlword, controlword);
        EC_WRITE_S8(domain_pd + off_modeofoperation, modeofoperation);
        EC_WRITE_U16(domain_pd + off_maxtorque, maxtorque);
        EC_WRITE_S32(domain_pd + off_target_velocity, target_velocity);

        ecrt_domain_queue(domain);
        ecrt_master_send(master);

        if (cycle % PRINT_FREQ == 0) {
            printf("Statusword: 0x%04X | Velocity Actual: %d | Target Velocity: %d\n", 
                   statusword, velocityactualvalue, target_velocity);
        }

        cycle++;
    }

    return 0;
}