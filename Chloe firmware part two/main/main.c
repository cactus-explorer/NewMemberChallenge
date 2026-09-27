#include <stdint.h>
#include <stdio.h>
#include <time.h>
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
#define EC_CMD_SWITCH_ON               0x0007  /* Step 2: Write Switch On */
#define EC_CMD_ENABLE_OPERATION        0x000F  /* Step 3: Write Enable Operation */

/* Statusword State Verification Values & Mask */
#define EC_STATUS_MASK                 0x006F  /* Mask for statusword evaluation (& 0x006F) */
#define EC_STATUS_READY_TO_SWITCH_ON   0x0231  /* Expected state after Shutdown command */
#define EC_STATUS_SWITCHED_ON          0x0233  /* Expected state after Switch On command */
#define EC_STATUS_OPERATION_ENABLED    0x0237  /* Expected state after Enable Operation command */

#define PRINT_FREQ 3

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
    domain = ecrt_master_create_domain(master);
    sc = ecrt_master_slave_config(master, 0, 0, VENDOR_ID, PRODUCT_CODE);


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

    ecrt_master_activate(master);

    int cycle = 0;
    
    while (1) {
        clock_nanosleep(CLOCK_MONOTONIC, 0, &(struct timespec){.tv_sec = 1}, NULL);
        
        if ((statusword & EC_STATUS_MASK) == EC_STATUS_READY_TO_SWITCH_ON) {
            controlword = EC_CMD_SHUTDOWN;
        } else if ((statusword & EC_STATUS_MASK) == EC_STATUS_SWITCHED_ON) {
            controlword = EC_CMD_ENABLE_OPERATION;
        } else if ((statusword & EC_STATUS_MASK) == EC_STATUS_OPERATION_ENABLED) {
            // Operational state maintained
        } else {
            printf("Unexpected status: 0x%04X\n", statusword);
        }

        modeofoperation = EC_MODE_CSV;
        maxtorque = EC_MAX_TORQUE_TEST_LIMIT;

        // Use cycle modulo 6 to repeat a 6-second pattern
        if ((cycle % 6) < 3) {
            target_velocity = (int32_t)(60 / 60.0 * 131072); // 60 RPM
        } else {
            target_velocity = 0;
        }

        if (cycle % PRINT_FREQ == 0) {
            printf("Statusword: 0x%04X\n", statusword);
            printf("Velocity Actual: %d\n", velocityactualvalue);
        }

        cycle++;
    }
}
