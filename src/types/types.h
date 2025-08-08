#pragma once

#include <Arduino.h>
#include <vector>

struct Reading { 
    float t = NAN; 
    float h = NAN; 
    unsigned long timestamp = 0;
};

using ReadingBatch = std::vector<Reading>;
