/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#import <libARUtils/ARUTILS_OBJCManager.h>


eARUTILS_ERROR ARUTILS_Manager_InitFtp(ARUTILS_Manager_t *manager, ARService *service, int port, const char *username, const char *password)
{
    eARUTILS_ERROR ret;
    if (!manager || !service)
        return ARUTILS_ERROR_BAD_PARAMETER;

    switch (service.network_type) {
    case ARDISCOVERY_NETWORK_TYPE_NET:
    {
        NSString *ip = [[ARDiscovery sharedInstance] convertNSNetServiceToIp:service];
        ret = ARUTILS_Manager_InitWifiFtp(manager, [ip UTF8String], port, username, password);
    }
    break;
    case ARDISCOVERY_NETWORK_TYPE_BLE:
    {
        ARUTILS_BLEDevice_t *ble_device = (__bridge ARUTILS_BLEDevice_t)((ARBLEService *)service.service).peripheral;
        ret = ARUTILS_Manager_InitBLEFtp(manager, ble_device, port);
    }
    break;
    case ARDISCOVERY_NETWORK_TYPE_USBMUX:
    {
        struct mux_ctx *mux = ((ARUSBService*)service.service).usbMux;
        ret = ARUTILS_Manager_InitWifiFtpOverMux(manager, NULL, port, mux, username, password);
    }
    break;
    default:
        ret = ARUTILS_ERROR_BAD_PARAMETER;
        break;
    }

    return ret;
}


void ARUTILS_Manager_CloseFtp(ARUTILS_Manager_t *manager, ARService *service)
{
    if (!manager || !service)
        return;

    switch(service.network_type) {
    case ARDISCOVERY_NETWORK_TYPE_NET:
    case ARDISCOVERY_NETWORK_TYPE_USBMUX:
        ARUTILS_Manager_CloseWifiFtp(manager);
        break;
    case ARDISCOVERY_NETWORK_TYPE_BLE:
        ARUTILS_Manager_CloseBLEFtp(manager);
        break;
    default:
        break;
    }
}
