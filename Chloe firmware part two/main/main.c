#include <stdint.h>
#include <ecrt.h>

// Ethercat definitions
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
#define EC_CMD_SWITCH_ON z              0x0007  /* Step 2: Write Switch On */
#define EC_CMD_ENABLE_OPERATION        0x000F  /* Step 3: Write Enable Operation */

/* Statusword State Verification Values & Mask */
#define EC_STATUS_MASK                 0x006F  /* Mask for statusword evaluation (& 0x006F) */
#define EC_STATUS_READY_TO_SWITCH_ON   0x0231  /* Expected state after Shutdown command */
#define EC_STATUS_SWITCHED_ON          0x0233  /* Expected state after Switch On command */
#define EC_STATUS_OPERATION_ENABLED    0x0237  /* Expected state after Enable Operation command */

#define printFrequency 3;

void app_main(void)
{
    printf("Hello world!\n");

    ecrt_request_master();
    ecrt_master_create_domain();
    ecrt_master_slave_config();
    ec_pdo_entry_reg_t domain_regs[] = {
        {0, EC_OD_CONTROLWORD, 0, &controlword},
        {0, EC_OD_STATUSWORD, 0, &statusword},
        {0, EC_OD_ERROR_CODE, 0, &errorcode},
        {0, EC_OD_MODES_OF_OPERATION, 0, &modeofoperation},
        {0, EC_OD_VELOCITY_ACTUAL_VALUE, 0, &velocityactualvalue},
        {0, EC_OD_MAX_TORQUE, 0, &maxtorque},
        {0, EC_OD_TARGET_VELOCITY, 0, &targetvelocity},
        {}
    };
    ecrt_domain_reg_pdo_entry_list(domain_regs);
    ecrt_master_activate();
    ecrt_domain_data(domain_regs);

    int cycle = 0;
    int status;
    int control;
    int elapsed_time = 0;
    while (1) {
        elapsed_time += 1; // Assuming 1 second per cycle

        clock_nanosleep(CLOCK_MONOTONIC, 0, &(struct timespec){.tv_sec = 1}, NULL);
        ecrt_master_receive();
        ecrt_domain_process();
        status = ecrt_master_state();
        control = ecrt_master_control();
        if ((status & EC_STATUS_MASK) == EC_STATUS_READY_TO_SWITCH_ON) {
            control = EC_CMD_SHUTDOWN;
        } else if ((status & EC_STATUS_MASK) == EC_STATUS_SWITCHED_ON) {
            control = EC_CMD_ENABLE_OPERATION;
        } else if ((status & EC_STATUS_MASK) == EC_STATUS_OPERATION_ENABLED) {
            // Already in operation enabled state, maintain current controlword
        } else {
            // Handle unexpected status
            printf("Unexpected status: 0x%04X\n", status);
        }
        ecrt_master_write(EC_OD_MODES_OF_OPERATION, EC_MODE_CSV);
        if ((status & EC_STATUS_MASK) == EC_STATUS_OPERATION_ENABLED) {
            ecrt_master_write(EC_OD_TARGET_VELOCITY, 1000);
        }
        ecrt_domain_queue();
        ecrt_master_send();
        if (cycle % printFrequency == 0) {
            printf("Statusword: 0x%04X\n", statusword);
            printf("Velocity Actual: %d\n", velocityactualvalue);
        }
        ecrt_master_write(EC_OD_MODES_OF_OPERATION, EC_MODE_CSV);
        ecrt_master_write(EC_OD_MAX_TORQUE, EC_MAX_TORQUE_TEST_LIMIT);
        target_velocity = rpm / 60.0 * 131072;
        // Start with 60 RPM
        if (elapsed_time <= 3) {
            target_velocity = 60 / 60.0 * 131072; // 60 RPM
        } else if (elapsed_time >= 3)
        {
            target_velocity = 0 / 60.0 * 131072;
        }
        while (elapsed_time >= 6) {
            elapsed_time = 0; // Reset elapsed time after 6 seconds
        }
        ecrt_master_write(EC_OD_TARGET_VELOCITY, target_velocity);
        cycle++;
    }
}
