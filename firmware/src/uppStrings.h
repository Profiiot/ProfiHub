//
// Created by Benjamin Skirlo on 21/12/16.
//

#ifndef FIRMWARE_UPPSTRINGS_H
#define FIRMWARE_UPPSTRINGS_H

#define UPP_ACKNOWLEDGE      "UPP-ACK"
#define UPP_ERROR            "UPP-ERR"
#define UPP_HUB_HANDSHAKE    "UPP-HUB"
#define UPP_FILTER_HANDSHAKE "UPP-FILTER"
#define UPP_INIT_END         "UPP-READY"
/**
 * UPP-LIST [name :String] [length :Number]
 */
#define UPP_LIST             "UPP-LIST"
#define UPP_LIST_END         "UPP-END"
/**
 * UPP-ENTRY [name :String] [pictogram :Base64]
 */
#define UPP_ENTRY            "UPP-ENTRY"
#define UPP_SELECT_SENSOR    "UPP-SELECT"
#define UPP_START_IO         "UPP-START"
#endif //FIRMWARE_UPPSTRINGS_H

