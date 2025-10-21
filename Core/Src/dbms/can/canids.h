#ifndef _CANIDS_H_
#define _CANIDS_H_

#define CANID_ISENSE_COMMAND        0x411

#define CANID_TX_HEARTBEAT          0x501
#define CANID_CONSOLE_C0            0x502 // No compression
#define CANID_CONSOLE_C3            0x505 // Aggressive compression -- Huffman encoding
#define CANID_METRIC                0x506
#define CANID_CELLSTATE_VOLTS       0x507
#define CANID_CELLSTATE_TEMPS       0x508
#define CANID_FATAL_ERROR           0x50B // = SOB = Son Of a Bitch

#define CANID_ISENSE_CURRENT        0x521
#define CANID_ISENSE_VOLTAGE1       0x522
#define CANID_ISENSE_VOLTAGE2       0x523
#define CANID_ISENSE_VOLTAGE3       0x524

#define CANID_ISENSE_POWER          0x526
#define CANID_ISENSE_CHARGE         0x527
#define CANID_ISENSE_ENERGY         0x528

#define CANID_BLACKBOX_OLD          0x529
#define CANID_BLACKBOX_NEW          0x530

#define CANID_TX_GET_CONFIG         0x533
#define CANID_TX_CFG_ACK            0x532

#define CANID_RX_TELEMBEAT          0x540
#define CANID_RX_HEARTBEAT          0x541
#define CANID_RX_SET_CONFIG         0x542
#define CANID_RX_GET_CONFIG         0x543
#define CANID_RX_CLEAR_FAULTS       0x544
#define CANID_RX_SET_INITIAL_CHARGE 0x545
#define CANID_RX_BLACKBOX_REQUEST   0x546
#define CANID_RX_BLACKBOX_SAVE      0x547

#define CANID_DEBUG_OVERWRITE_TEMPS 0x581

#define CANID_ELCON_A               0x1806E5F4
#define CANID_ELCON_B               0x18FF50E5

#endif
