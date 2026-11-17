#pragma once
#include "Arduino.h"
#include <iostream>
#include "ThingerClient.h"
protoson::dynamic_memory_allocator test_allocator;
protoson::memory_allocator& protoson::pool=test_allocator;
