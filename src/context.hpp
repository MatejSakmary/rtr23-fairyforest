#pragma once
#include "backend/backend.hpp"

struct Context
{
    ff::Instance instance = {};
    ff::Device device = {};

    Context();
    ~Context();
};