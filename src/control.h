//
// Created by Rasanja Dampriya on 8/22/25.
//

#pragma once

// Start the switch -> PiKVM control task.
// Reads the switch on GPIO14, drives LED on GPIO13.
// On boot: reads PiKVM ATX state and sets LED accordingly.
// On switch ON: power on PiKVM and confirm -> LED on.
// On switch OFF: power off PiKVM and confirm -> LED off.
void start_control_task(void);
