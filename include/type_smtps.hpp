
#ifndef TYPE_SMTPS_HPP
#define TYPE_SMTPS_HPP

#include <stdint.h>

constexpr uint8_t TYPE_SMTPS_LOGIN      = 0;
constexpr uint8_t TYPE_SMTPS_BEGIN      = 1;
constexpr uint8_t TYPE_SMTPS_MESSAGE    = 2;
constexpr uint8_t TYPE_SMTPS_LOGOUT     = 3;
constexpr uint8_t TYPE_SMTPS_SIZE       = 4;

const char* type_smtps_to_string(uint8_t type) noexcept
{
    constexpr static const char* strings[TYPE_SMTPS_SIZE] = 
    {
        "TYPE_SMTPS_LOGIN",
        "TYPE_SMTPS_BEGIN",
        "TYPE_SMTPS_MESSAGE",
        "TYPE_SMTPS_LOGOUT"
    };

    constexpr static const char* error = "Type is unknown.\n";

    return (type < TYPE_SMTPS_SIZE) ? strings[type] : error;
}

#endif

