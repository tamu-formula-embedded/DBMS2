/****************************************************************************
CAN Read Demo for the SparkFun CAN Bus Shield.
 
Written by Stephen McCoy.
Original tutorial available here:
http://www.instructables.com/id/CAN-Bus-Sniffing-and-Broadcasting-with-Arduino
Used with permission 2016. License CC By SA.
 
Distributed as-is; no warranty is given.
*************************************************************************/
 
#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
 
 
//********************************Send EEPROM Requests**************************//
void sendEEPROMQuery(int setting, tCAN msg_ojb) {
    msg_ojb.id = 0x543; //formatted in HEX
    msg_ojb.header.rtr = 0;
    msg_ojb.header.length = 8; //formatted in DEC
 
    msg_ojb.data[0] = setting & 0xFF;
    msg_ojb.data[1] = 0x00;
    msg_ojb.data[2] = 0x00;
    msg_ojb.data[3] = 0x00;
    msg_ojb.data[4] = 0x00;
    msg_ojb.data[5] = 0x00;
    msg_ojb.data[6] = 0x00;
    msg_ojb.data[7] = 0x00;
 
    mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
    mcp2515_send_message(&msg_ojb);
    // Serial.println("Sent");
}
 
void setEEPROMValue(int setting, int val, tCAN msg_ojb) {
    msg_ojb.id = 0x542; //formatted in HEX
    msg_ojb.header.rtr = 0;
    msg_ojb.header.length = 8; //formatted in DEC
 
    msg_ojb.data[0] = setting & 0xFF;
    msg_ojb.data[1] = 0x00;
    msg_ojb.data[2] = 0x00;
    msg_ojb.data[3] = 0x00;
    msg_ojb.data[4] = 0x00;
    msg_ojb.data[5] = 0x00;
    msg_ojb.data[6] = 0x00;
    msg_ojb.data[7] = 0x00;
 
    memcpy(msg_ojb.data+4, &val, 4);
 
    mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
    mcp2515_send_message(&msg_ojb);
    // Serial.println("Sent");
}
 
void sendHeartbeat(tCAN msg_ojb) {
    msg_ojb.id = 0x541; //formatted in HEX
    msg_ojb.header.rtr = 0;
    msg_ojb.header.length = 8;
 
    memset(msg_ojb.data, 0, 8);
 
    mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
    mcp2515_send_message(&msg_ojb);
}
 
//********************************Setup Loop*********************************//
 
void setup() {
    Serial.begin(115200); // For debug use
    Serial.println("CAN Read - Testing reception of CAN Bus message");
    delay(1000);
 
    if (Canbus.init(CANSPEED_1000)) // Initialise MCP2515 CAN controller at the
                                   // specified speed
        Serial.println("CAN Init ok");
    else
        Serial.println("Can't init CAN");
 
    delay(1000);
}
 
//********************************Main Loop*********************************//
 
void loop() {
    tCAN message;
 
    //sendEEPROMQuery(0x02, message);
    //setEEPROMValue(0x02, 5000, message);//requests max voltage in mV
 
    sendHeartbeat(message);

    if (mcp2515_check_message())
    {
        if (mcp2515_get_message(&message))
        {
            if (message.id != 0x501)
            {
                Serial.print("(CAN ID: ");
                Serial.print(message.id, HEX);
                Serial.print(")");
 
                if (message.id == 0x50B)
                {
                    Serial.print("ERROR file: ");
                    for (int i = 0; i < 5; i++)
                    {
                        char msg_byte = message.data[i];
                        Serial.print(msg_byte);
                    }
                    Serial.print(" Line: ");
                    Serial.print((message.data[5] << 8) + message.data[6]);
                    Serial.print(" Errno: ");
 
                    Serial.println(message.data[7]);
                }
                else if (message.id == 0x502)
                {
                    Serial.print("LOG");
                    for (int i = 0; i < 8; i++)
                    {
                        char msg_byte = message.data[i];
                        Serial.print(msg_byte);
                    }
                }
                else
                {
                    for (int i = 0; i < message.header.length; i++)
                    {
                        Serial.print(message.data[i], HEX);
                        Serial.print(" ");
                    }
                    Serial.println("");
                }
            }
        }
    }
}