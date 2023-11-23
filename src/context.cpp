#include "context.hpp"

Context::Context() :
    instance{},
    device{instance.create_device()}
{

}

Context::~Context()
{

};