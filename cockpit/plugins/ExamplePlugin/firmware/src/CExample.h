#pragma once

#include <Arduino.h>

#include "pb_decode.h"
#include "pb_encode.h"

#include "ExampleMessage.pb.h"

class CExample 
{
    public:
        void Intialize();
        void Update( );
}