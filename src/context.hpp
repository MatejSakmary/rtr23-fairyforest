#pragma once
#include "backend/backend.hpp"

struct Context
{
    ff::Instance instance = {};

    Context();
    ~Context();
};