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

//********************************Setup Loop*********************************//

void setup() {
    Serial.begin(115200); // For debug use
    Serial.println("CAN Read - Testing reception of CAN Bus message");
    delay(1000);

    if (Canbus.init(CANSPEED_500)) // Initialise MCP2515 CAN controller at the
                                   // specified speed
        Serial.println("CAN Init ok");
    else
        Serial.println("Can't init CAN");

    delay(1000);
}

//********************************Main Loop*********************************//

void loop() {
    tCAN message;

    // message.id = 0x631; //formatted in HEX
    //       message.header.rtr = 0;
    //       message.header.length = 8; //formatted in DEC
    //       message.data[0] = 0x00;
    // message.data[1] = 0xFF;
    // message.data[2] = 0x00;
    // message.data[3] = 0xFF; //formatted in HEX
    // message.data[4] = 0x00;
    // message.data[5] = 0xFF;
    // message.data[6] = 0x00;
    // message.data[7] = 0xFF;

    // mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
    // mcp2515_send_message(&message);
    // Serial.println("Sent");

    delay(10);
    if (mcp2515_check_message()) 
    {
        if (mcp2515_get_message(&message)) 
        {
            Serial.print("(CAN ID: ");
            Serial.print(message.id, HEX);
            Serial.print(")");

            if (message.id == 0x50B) 
            {
                Serial.print("ERROR file: ");
                for (int i = 0; i < 5; i++) 
                {
                    Serial.print(message.data[i], ASCII);
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
                    Serial.print(message.data[i], ASCII);
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
