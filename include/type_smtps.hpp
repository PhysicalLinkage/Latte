
#ifndef TYPE_SMTPS_HPP
#define TYPE_SMTPS_HPP

#include <stdint.h>

#include <dhl.hpp>
#include <cmac.hpp>
#include <smtps.hpp>

constexpr uint8_t TYPE_SMTPS_CONTACT    = 0;
constexpr uint8_t TYPE_SMTPS_AUTH       = 1;
constexpr uint8_t TYPE_SMTPS_MESSAGE    = 2;
constexpr uint8_t TYPE_SMTPS_LOGOUT     = 3;
constexpr uint8_t TYPE_SMTPS_SIZE       = 4;

const char* type_smtps_to_string(uint8_t type) noexcept
{
    constexpr static const char* strings[TYPE_SMTPS_SIZE] = 
    {
        "TYPE_SMTPS_CONTACT",
        "TYPE_SMTPS_AUTH",
        "TYPE_SMTPS_MESSAGE",
        "TYPE_SMTPS_LOGOUT"
    };

    constexpr static const char* error = "Type is unknown.\n";

    return (type < TYPE_SMTPS_SIZE) ? strings[type] : error;
}

constexpr uint16_t SMTPS_CONTACT_NONCE_BYTES = 12;

constexpr uint16_t SMTPS_TYPE_BYTES = 1;
constexpr uint16_t SMTPS_CONTACT_REQUEST_BYTES = SMTPS_TYPE_BYTES;
constexpr uint16_t SMTPS_CONTACT_REPLY_BYTES = SMTPS_TYPE_BYTES
    + SMTPS_CONTACT_NONCE_BYTES
    + DHL_KEY_SIZE;
constexpr uint16_t SMTPS_AUTH_REQUEST_BYTES = SMTPS_CONTACT_REPLY_BYTES;
constexpr uint16_t SMTPS_AUTH_REPLY_BYTES = SMTPS_TYPE_BYTES
    + HeaderSMTPSSize();
constexpr uint16_t SMTPS_LOGOUT_REQUEST_BYTES = SMTPS_TYPE_BYTES
    + HeaderSMTPSSize();
#endif

