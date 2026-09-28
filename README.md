# EtherCAT CiA 402 Real-Time Motor Controller

This project is a real-time C application that interfaces with industrial motor drives using the EtherCAT protocol and the IgH EtherCAT Master stack. It programmatically manages the CiA 402 drive profile state machine, safely handling device initialization, fault resets, and operational state transitions, while executing a 1 kHz control loop in Cyclic Synchronous Velocity (CSV) mode. The codebase demonstrates core competencies in industrial automation, embedded systems programming, and deterministic real-time communication for robotic or motion control hardware.

This module contains the solution for the WiscoHumanoids firmware team new member challenge. WiscoHumanoids is the undergraduate humanoid robotics student organization at the University of Wisconsin-Madison, established to design and build a custom 31-degree-of-freedom humanoid robotic platform.

## Features

* **Industrial Protocol Integration**: Communicates directly over EtherCAT using the IgH Master stack.

* **CiA 402 State Machine**: Automated management of drive status sequences (Shutdown, Switch On, Enable Operation, and Fault Reset).

* **Deterministic 1 kHz Control Loop**: Precise real-time timing using `clock_nanosleep` and Process Data Object (PDO) mapping.

* **Cyclic Synchronous Velocity (CSV)**: Safely commands targeted velocities with configurable torque limits.

## Prerequisites

* Linux environment with an installed and configured IgH EtherCAT Master stack.

* GCC compiler.

* Compatible EtherCAT slave hardware configured according to the vendor ID and product code specified in the source.

## Getting Started

1. Navigate to the project directory:

   ```
   cd "Chloe firmware part two"
   
   ```

2. Compile the source file, linking against the EtherCAT library:

   ```
   gcc main/main.c -lethercat
   
   ```

3. Run the compiled binary (requires root or appropriate network permissions to access the Ethernet interface):

   ```
   sudo ./a.out
   
   ```
