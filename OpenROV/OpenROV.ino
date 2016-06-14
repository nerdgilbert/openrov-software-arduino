#include "pb_encode.h"
#include "pb_decode.h"
#include "Master.pb.h"


void setup()
{
    Serial.begin(9600);
    randomSeed(analogRead(A0));
}


void loop()
{
    /* This is the buffer where we will store our message. */
    uint8_t buffer[128];
    size_t message_length;
    bool status;
    
    /* Encode our message */
    {
        /* Allocate space on the stack to store the message data.
         *
         * Nanopb generates simple struct definitions for all the messages.
         * - check out the contents of simple.pb.h!
         * It is a good idea to always initialize your structures
         * so that you do not have garbage data from RAM in there.
         */
        IntMessage message = IntMessage_init_zero;
        
        /* Create a stream that will write to our buffer. */
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        
        /* Fill in the lucky number */
        message.x = 42;
        message.y = 43;
        message.z = 44;

        
        /* Now we are ready to encode the message! */
        status = pb_encode(&stream, IntMessage_fields, &message);
        message_length = stream.bytes_written;
        
        /* Then just check for any errors.. */
        if (!status)
        {
            //printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
            Serial.println("Encoding failed!");
        }
    }
    
    /* Now we could transmit the message over network, store it in a file or
     * wrap it to a pigeon's leg.
     */

    /* But because we are lazy, we will just decode it immediately. */
    
    {
        /* Allocate space for the decoded message. */
        IntMessage message = IntMessage_init_zero;
        
        /* Create a stream that reads from the buffer. */
        pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);
        
        /* Now we are ready to decode the message. */
        status = pb_decode(&stream, IntMessage_fields, &message);
        
        /* Check for errors... */
        if (!status)
        {
            //printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            Serial.println("Decoding failed.");
        }
        
        /* Print the data contained in the message. */
        Serial.print(message.x);
        Serial.print(", ");
        Serial.print(message.y);
        Serial.print(", ");
        Serial.println(message.z);
        
    }
}
