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
/**
 * @file ARUTILS_Ftp.c
 * @brief libARUtils Ftp c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#import <CoreBluetooth/CoreBluetooth.h>
#import <libARSAL/ARSAL_CentralManager.h>
#import <CommonCrypto/CommonDigest.h>

#include <libARSAL/ARSAL_Sem.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Singleton.h>
#include <libARSAL/ARSAL_Print.h>
#import <libARSAL/ARSAL_BLEManager.h>
#import <libARSAL/ARSAL_Endianness.h>
#include <curl/curl.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Manager.h"
#include "libARUtils/ARUTILS_Ftp.h"
#include "libARUtils/ARUTILS_FileSystem.h"
#include "ARUTILS_Manager.h"
#include "ARUTILS_BLEFtp.h"


NSString* const kARUTILS_BLEFtp_Getting = @"kARUTILS_BLEFtp_Getting";

#define BLE_MD5_TXT_SIZE           (CC_MD5_DIGEST_LENGTH * 2)
#define BLE_PACKET_MAX_SIZE        132
#define BLE_PACKET_EOF             "End of Transfer"
#define BLE_PACKET_WRITTEN         "FILE WRITTEN"
#define BLE_PACKET_NOT_WRITTEN     "FILE NOT WRITTEN"
#define BLE_PACKET_RENAME_SUCCESS   "Rename successful"
#define BLE_PACKET_RENAME_FROM_SUCCESS   "Rename successful"
#define BLE_PACKET_DELETE_SUCCESS   "Delete successful"
#define BLE_PACKET_
#define BLE_PACKET_BLOCK_GETTING_COUNT     100
#define BLE_PACKET_BLOCK_PUTTING_COUNT     500


//#define BLE_PACKET_WRITE_SLEEP             18000000 /* 18ms */
#define BLE_PACKET_WRITE_SLEEP               20000000

#define ARUTILS_BLEFTP_TAG      "BLEFtp"

//#define ARUTILS_BLEFTP_ENABLE_LOG (1)
#define ARUTILS_BLEFTP_ENABLE_LOG (0)


@interface ARUtils_BLEFtp ()
{
    ARSAL_Mutex_t connectionLock;
}

@property (nonatomic, retain) CBPeripheral *peripheral;
@property (nonatomic, assign) int port;
@property (nonatomic, assign) int connectionCount;

@property (nonatomic, retain) CBCharacteristic *transferring;
@property (nonatomic, retain) CBCharacteristic *getting;
@property (nonatomic, retain) CBCharacteristic *handling;
@property (nonatomic, retain) NSArray *arrayGetting;

@end

@implementation ARUtils_BLEFtp
SYNTHESIZE_SINGLETON_FOR_CLASS(ARUtils_BLEFtp, initBLEFtp)

- (id)initBLEFtp
{
    self = [super init];
    if (self != nil)
    {
        ARSAL_Mutex_Init(&connectionLock);
    }
    return self;
}

- (void)dealloc
{
    ARSAL_Mutex_Destroy(&connectionLock);
}

- (ARSAL_Mutex_t*)getConnectionLock
{
    return &connectionLock;
}

- (eARUTILS_ERROR)registerPeripheral:(CBPeripheral *)peripheral cancelSem:(ARSAL_Sem_t*)cancelSem port:(int)port
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (_connectionCount == 0)
    {
        _peripheral = peripheral;
        _port = port;
        _connectionCount++;
    }
    else if ((_peripheral == peripheral) && (_port == port))
    {
        _connectionCount++;
    }
    else
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    
    return result;
}

- (eARUTILS_ERROR)unregisterPeripheral
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (_connectionCount > 0)
    {
        if (_connectionCount == 1)
        {
            _peripheral = nil;
            _transferring = nil;
            _getting = nil;
            _handling = nil;
            _port = 0;
        }
        
        _connectionCount--;
    }
    else
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    
    return result;
}

- (eARUTILS_ERROR)registerCharacteristics
{
    eARSAL_ERROR resultSal = ARSAL_OK;
    eARSAL_ERROR discoverCharacteristicsResult = ARSAL_OK;
    //eARSAL_ERROR setNotifCharacteristicResult = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    for(int i = 0 ; (i < [[_peripheral services] count]) && (resultSal == ARSAL_OK) && ((_transferring == nil) || (_getting == nil) || (_handling == nil)) ; i++)
    {
        CBService *service = [[_peripheral services] objectAtIndex:i];
#if ARUTILS_BLEFTP_ENABLE_LOG
        NSLog(@"Service : %@, %04x", [service.UUID shortUUID], (unsigned int)service.UUID);
#endif
        
        if([[service.UUID shortUUID] hasPrefix:[NSString stringWithFormat:@"fd%02d", _port]])
        {
            //discoverCharacteristicsResult = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkCharacteristics:nil forService:service];
            
            discoverCharacteristicsResult = ARSAL_OK;
            if (discoverCharacteristicsResult == ARSAL_OK)
            {
                resultSal = ARSAL_OK;
                
                for (CBCharacteristic *characteristic in [service characteristics])
                {
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"CBCharacteristic: %@", characteristic.UUID.shortUUID);
#endif
                    if ([characteristic.UUID.shortUUID hasPrefix:[NSString stringWithFormat:@"fd%02d", _port + 1]])
                    {
                        if ((characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) == CBCharacteristicPropertyWriteWithoutResponse)
                        {
                            _transferring = characteristic;
                        }
                    }
                    else if ([characteristic.UUID.shortUUID hasPrefix:[NSString stringWithFormat:@"fd%02d", _port + 2]])
                    {
                        if (((characteristic.properties & CBCharacteristicPropertyRead) == CBCharacteristicPropertyRead)
                            && ((characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) == CBCharacteristicPropertyWriteWithoutResponse))
                        {
                            _getting = characteristic;
                        }
                        if ((characteristic.properties & CBCharacteristicPropertyNotify) == CBCharacteristicPropertyNotify)
                        {
                            _arrayGetting = [NSArray arrayWithObject:characteristic];
                        }
                    }
                    else if ([characteristic.UUID.shortUUID hasPrefix:[NSString stringWithFormat:@"fd%02d", _port + 3]])
                    {
                        if ((characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) == CBCharacteristicPropertyWriteWithoutResponse)
                        {
                            _handling = characteristic;
                        }
                    }
                }
            }
        }
    }
    
    if ((_transferring != nil) && (_getting != nil) && (_handling != nil))
    {
        result = ARUTILS_OK;
        
        /*if (ret == YES)
        {
            setNotifCharacteristicResult = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) setNotificationCharacteristic:_getting];
            if (setNotifCharacteristicResult != ARSAL_OK)
            {
                ret = NO;
            }
        }*/
        
        if (result == ARUTILS_OK)
        {
            [SINGLETON_FOR_CLASS(ARSAL_BLEManager) registerNotificationCharacteristics:_arrayGetting toKey:kARUTILS_BLEFtp_Getting];
        }
    }
    else
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    
    return result;
}

- (eARUTILS_ERROR)unregisterCharacteristics
{
    eARSAL_ERROR retBLE = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) unregisterNotificationCharacteristics:kARUTILS_BLEFtp_Getting];
    if (retBLE != ARSAL_OK)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    
    return result;
}

- (eARUTILS_ERROR)cancelFile:(ARSAL_Sem_t*)cancelSem
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resutlSys = 0;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    if (cancelSem != NULL)
    {
        resutlSys = ARSAL_Sem_Post(cancelSem);
        
        if (resutlSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    [SINGLETON_FOR_CLASS(ARSAL_BLEManager) cancelReadNotification:kARUTILS_BLEFtp_Getting];
    
    return result;
}

- (eARUTILS_ERROR)resetConnection:(ARSAL_Sem_t*)cancelSem
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    if (cancelSem != NULL)
    {
        while (ARSAL_Sem_Trywait(cancelSem) == 0)
        {
            /* Do Nothing */
        }
    }

    return result;
}

- (eARUTILS_ERROR)listFiles:(NSString*)remotePath resultList:(char **)resultList resultListLen:(uint32_t *)resultListLen
{
    uint8_t *data = NULL;
    uint8_t *oldData = NULL;
    uint32_t dataLen = 0;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    *resultList = NULL;
    *resultListLen = 0;
    
    result = [self sendCommand:"LIS" param:[remotePath UTF8String] characteristic:_handling];
    
    if (result == ARUTILS_OK)
    {
        result = [self readGetData:0 dstFile:NULL data:&data dataLen:&dataLen progressCallback:NULL progressArg:NULL cancelSem:NULL];
        
        if (result == ARUTILS_OK)
        {
            oldData = data;
            data = realloc(oldData, dataLen + 1);
            
            if (data == NULL)
            {
                free(oldData);
                result = ARUTILS_ERROR_ALLOC;
            }
            else
            {
                data[dataLen++] = '\0';
                
                *resultList = (char*)data;
                *resultListLen = dataLen;
            }
        }
    }
    
    if ((result != ARUTILS_OK) && (*resultList != NULL))
    {
        free(*resultList);
        *resultList = 0;
        *resultListLen = 0;
    }
    
    return result;
}

- (eARUTILS_ERROR)sizeFile:(NSString*)remoteFile fileSize:(double*)fileSize
{
    char *resultList = NULL;
    uint32_t resultListLen = 0;
    BOOL found = NO;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s %@", __FUNCTION__, remoteFile);
#endif

    remoteFile = [self normalizeName:remoteFile];
    *fileSize = 0.f;
    NSString *remotePath = [remoteFile stringByDeletingLastPathComponent];
    
    result = [self listFiles:remotePath resultList:&resultList resultListLen:&resultListLen];
    
    if (result == ARUTILS_OK)
    {
        const char *remoteFileName = [[remoteFile lastPathComponent] UTF8String];
        const char *nextItem = NULL;
        const char *fileName = NULL;
        const char *indexItem = NULL;
        int itemLen = 0;
        
        while ((found == NO) && (fileName = ARUTILS_Ftp_List_GetNextItem(resultList, &nextItem, NULL, 0, &indexItem, &itemLen)) != NULL)
        {
            if (strcmp(remoteFileName, fileName) == 0)
            {
                if (ARUTILS_Ftp_List_GetItemSize(indexItem, itemLen, fileSize) == NULL)
                {
                    result = ARUTILS_ERROR_FTP_SIZE;
                }
                else
                {
                    found = YES;
                }
            }
        }
    }
    
    if (found == YES)
    {
        result = ARUTILS_OK;
    }
    else
    {
        result = ARUTILS_ERROR_FTP_SIZE;
    }
    
    return result;
}

- (eARUTILS_ERROR)getFileInternal:(NSString*)remoteFile localFile:(NSString*)localFile data:(uint8_t**)data dataLen:(uint32_t*)dataLen progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg cancelSem:(ARSAL_Sem_t*)cancelSem
{
    FILE *dstFile = NULL;
    double totalSize = 0.f;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s %@", __FUNCTION__, remoteFile);
#endif
    
    remoteFile = [self normalizeName:remoteFile];

    result = [self sizeFile:remoteFile fileSize:&totalSize];
    if (result == ARUTILS_OK)
    {
        result = [self sendCommand:"GET" param:[remoteFile UTF8String] characteristic:_handling];
    }
    
    if ((result == ARUTILS_OK) && (localFile != nil))
    {
        dstFile = fopen([localFile UTF8String], "wb");
        if (dstFile == NULL)
        {
            result = ARUTILS_ERROR_FTP_FILE;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        result = [self readGetData:(uint32_t)totalSize dstFile:dstFile data:data dataLen:dataLen progressCallback:progressCallback progressArg:progressArg cancelSem:cancelSem];
    }
    
    if (dstFile != NULL)
    {
        fclose(dstFile);
    }
    
    return result;
}

- (eARUTILS_ERROR)getFile:(NSString*)remoteFile localFile:(NSString*)localFile progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg cancelSem:(ARSAL_Sem_t*)cancelSem
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    result = [self getFileInternal:remoteFile localFile:localFile data:NULL dataLen:NULL progressCallback:progressCallback progressArg:progressArg cancelSem:cancelSem];
    return result;
}

- (eARUTILS_ERROR)getFileWithBuffer:(NSString*)remoteFile data:(uint8_t**)data dataLen:(uint32_t*)dataLen progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg cancelSem:(ARSAL_Sem_t*)cancelSem
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    result = [self getFileInternal:remoteFile localFile:nil data:data dataLen:dataLen progressCallback:progressCallback progressArg:progressArg cancelSem:cancelSem];
    
    return result;
}

- (eARUTILS_ERROR)abortPutFile:(NSString*)remoteFile
{
    int resumeIndex = 0;
    BOOL resume = NO;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    remoteFile = [self normalizeName:remoteFile];
    
    result = [self readPutResumeIndex:remoteFile resumeIndex:&resumeIndex];
    if ((result == ARUTILS_OK) && (resumeIndex > 0))
    {
        resume = YES;
    }
    else
    {
        resume = NO;
    }
    
    if (result == ARUTILS_OK)
    {
        result = [self sendCommand:"PUT" param:[remoteFile UTF8String] characteristic:_handling];
        
        if (result == ARUTILS_OK)
        {
            result = [self sendPutData:0 srcFile:NULL resumeIndex:0 resume:NO abort:YES progressCallback:NULL progressArg:NULL cancelSem:NULL];
        }
    }
    
    [self deleteFile:remoteFile];
    
    return result;
}

- (eARUTILS_ERROR)putFile:(NSString*)remoteFile localFile:(NSString*)localFile progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg resume:(BOOL)resume cancelSem:(ARSAL_Sem_t*)cancelSem
{
    FILE *srcFile = NULL;
    int resumeIndex = 0;
    uint32_t totalSize = 0;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s %@", __FUNCTION__, remoteFile);
#endif
    
    remoteFile = [self normalizeName:remoteFile];
    
    if (resume == NO)
    {
        [self abortPutFile:remoteFile];
    }
    else
    {
        result = [self readPutResumeIndex:remoteFile resumeIndex:&resumeIndex];
        if (result != ARUTILS_OK)
        {
            result = ARUTILS_OK;
            resumeIndex = 0;
            resume = NO;
        }
        
        if (resumeIndex > 0)
        {
            resume = YES;
        }
    }
    
    result = ARUTILS_FileSystem_GetFileSize([localFile UTF8String], &totalSize);
    
    if (result == ARUTILS_OK)
    {
        result = [self sendCommand:"PUT" param:[remoteFile UTF8String] characteristic:_handling];
    }
    
    if (result == ARUTILS_OK)
    {
        srcFile = fopen([localFile UTF8String], "rb");
        if (srcFile == NULL)
        {
            result = ARUTILS_ERROR_FTP_FILE;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        result = [self sendPutData:totalSize srcFile:srcFile resumeIndex:resumeIndex resume:resume abort:NO progressCallback:progressCallback progressArg:progressArg cancelSem:cancelSem];
    }
    
    if (srcFile != NULL)
    {
        fclose(srcFile);
    }
    
    if (result == ARUTILS_ERROR_FTP_MD5)
    {
        [self abortPutFile:remoteFile];
    }
    
    return result;
}

- (eARUTILS_ERROR)deleteFile:(NSString*)remoteFile
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    result = [self sendCommand:"DEL" param:[remoteFile UTF8String] characteristic:_handling];
    if (result == ARUTILS_OK)
    {
        result = [self readDeleteData];
    }
    
    return result;
}

- (eARUTILS_ERROR)renameFile:(NSString*)oldNamePath newNamePath:(NSString*)newNamePath
{
    eARUTILS_ERROR result = ARUTILS_OK;
    NSString *param = [NSString stringWithFormat:@"%@ %@", oldNamePath, newNamePath];
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    if ([param length] > BLE_PACKET_MAX_SIZE)
    {
        result = [self renameLongFile:oldNamePath newNamePath:newNamePath];
    }
    else
    {
        result = [self renameShortFile:oldNamePath newNamePath:newNamePath];
    }
    
    return result;
}

- (eARUTILS_ERROR)renameLongFile:(NSString*)oldNamePath newNamePath:(NSString*)newNamePath
{
    eARUTILS_ERROR result = ARUTILS_OK;
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    result = [self sendCommand:"RNFR" param:[oldNamePath UTF8String] characteristic:_handling];
    if (result == ARUTILS_OK)
    {
        result = [self readRenameData];
    }
    if (result == ARUTILS_OK)
    {
        result = [self sendCommand:"RNTO" param:[newNamePath UTF8String] characteristic:_handling];
    }
    if (result == ARUTILS_OK)
    {
        result = [self readRenameData];
    }
    
    return result;
}

- (eARUTILS_ERROR)renameShortFile:(NSString*)oldNamePath newNamePath:(NSString*)newNamePath
{
    eARUTILS_ERROR result = ARUTILS_OK;
    NSString *param = [NSString stringWithFormat:@"%@ %@", oldNamePath, newNamePath];
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    result = [self sendCommand:"REN" param:[param UTF8String] characteristic:_handling];
    
    if (result == ARUTILS_OK)
    {
        result = [self readRenameData];
    }
    
    return result;
}

- (eARUTILS_ERROR)sendCommand:(const char *)cmd param:(const char*)param characteristic:(CBCharacteristic *)characteristic
{
    char *command = NULL;
    int len = 0;
    int size;
    BOOL retBLE;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s %s", __FUNCTION__, cmd);
#endif
    
    if (param != NULL)
    {
        size = BLE_PACKET_MAX_SIZE + strlen(param) + 1;
    }
    else
    {
        size = BLE_PACKET_MAX_SIZE;
    }
    
    command = malloc(size);
    if (command == NULL)
    {
        result = ARUTILS_ERROR_ALLOC;
    }
    else
    {
        strncpy(command, cmd, BLE_PACKET_MAX_SIZE);
        command[BLE_PACKET_MAX_SIZE - 1] = '\0';
        
        if (param != NULL)
        {
            strcat(command, param);
        }
        len = strlen(command) + 1;
        
        NSData *data = [NSData dataWithBytes:command length:len];
        retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:characteristic];
        if (retBLE == NO)
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
        
        free(command);
    }
    
    return result;
}

- (eARUTILS_ERROR)sendPutData:(uint32_t)fileSize srcFile:(FILE*)srcFile resumeIndex:(int)resumeIndex resume:(BOOL)resume abort:(BOOL)abort progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg cancelSem:(ARSAL_Sem_t*)cancelSem
{
    uint8_t md5[CC_MD5_DIGEST_LENGTH];
    char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    char md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 3 + 1];
    uint8_t packet[BLE_PACKET_MAX_SIZE];
    CC_MD5_CTX ctxEnd;
    CC_MD5_CTX ctx;
    BOOL retBLE = YES;
    int totalSize = 0;
    int packetCount = 0;
    int totalPacket = 0;
    int packetLen = BLE_PACKET_MAX_SIZE;
    BOOL endFile = NO;
    ARSAL_Sem_t timeSem;
    struct timespec timeout;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    timeout.tv_sec = 0;
    timeout.tv_nsec = BLE_PACKET_WRITE_SLEEP;
    ARSAL_Sem_Init(&timeSem, 0, 0);
    CC_MD5_Init(&ctx);
    CC_MD5_Init(&ctxEnd);
    
    if (abort == YES)
    {
        endFile = YES;
        resumeIndex = 0;
        resume = NO;
    }
    
    do
    {
        if (abort == NO)
        {
            packetLen = (int)fread(packet, sizeof(char), BLE_PACKET_MAX_SIZE, srcFile);
        }
        if (packetLen > 0)
        {
            packetCount++;
            totalPacket++;
            totalSize += packetLen;
            CC_MD5_Update(&ctxEnd, packet, packetLen);
            
            if ((resume == NO) || ((resume == YES) && (totalPacket > resumeIndex)))
            {
                CC_MD5_Update(&ctx, packet, packetLen);
                
                ARSAL_Sem_Timedwait(&timeSem, &timeout);
                NSData *data = [NSData dataWithBytes:packet length:packetLen];
                retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
                if (retBLE == NO)
                {
                    result = ARUTILS_ERROR_FTP_CONNECT;
                }
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"packet %d, %d, %d", packetCount, packetLen, totalSize);
#endif
            }
            else
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"resume %d, %d, %d", packetCount, BLE_PACKET_MAX_SIZE, totalSize);
#endif
            }
            
            if (progressCallback != NULL)
            {
                progressCallback(progressArg, ((float)totalSize / (float)fileSize) * 100.f);
            }
        }
        else
        {
            if (feof(srcFile))
            {
                endFile = YES;
            }
        }
        
        if (cancelSem != NULL)
        {
            result = ARUTILS_BLEFtp_IsCanceledSem(cancelSem);
            if (result != ARUTILS_OK)
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"canceled received");
#endif
            }
        }
        
        if ((result == ARUTILS_OK) && ((packetCount >= BLE_PACKET_BLOCK_PUTTING_COUNT) || ((endFile == YES) && (packetCount > 0))))
        {
            packetCount = 0;
            
            if ((resume == NO) || ((resume == YES) && (totalPacket > resumeIndex)))
            {
                CC_MD5_Final(md5, &ctx);
                CC_MD5_Init(&ctx);
                sprintf(md5Txt, "MD5");
                for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
                {
                    sprintf(&md5Txt[3 + (i * 2)], "%02x", md5[i]);
                }
                md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 3] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"sending md5: %s", md5Txt);
#endif
                
                ARSAL_Sem_Timedwait(&timeSem, &timeout);
                NSData *data = [NSData dataWithBytes:md5Txt length:strlen(md5Txt)];
                retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
                if (retBLE == NO)
                {
                    result = ARUTILS_ERROR_FTP_CONNECT;
                }
                if (result == ARUTILS_OK)
                {
                    result = [self readPutDataWritten];
                }
            }
        }
    }
    while ((result == ARUTILS_OK) && (endFile == NO));
    
    if ((result == ARUTILS_OK) && (endFile == YES))
    {
        ARSAL_Sem_Timedwait(&timeSem, &timeout);
        NSData *data = [[NSData alloc] init];
        retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
        if (retBLE == NO)
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
        if (result == ARUTILS_OK)
        {
            result = [self readPutMd5:md5Msg];
        }
        
        if (result == ARUTILS_OK)
        {
            CC_MD5_Final(md5, &ctxEnd);
            for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&md5Txt[i * 2], "%02x", md5[i]);
            }
            md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
            NSLog(@"md5 end %s", md5Txt);
            NSLog(@"file size %d", totalSize);
#endif
            if (strncmp(md5Msg, md5Txt, CC_MD5_DIGEST_LENGTH * 2) != 0)
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"MD5 End Failed");
#endif
                result = ARUTILS_ERROR_FTP_MD5;
            }
            else
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"MD5 End OK");
#endif
            }
        }
    }
    
    ARSAL_Sem_Destroy(&timeSem);
    
    return result;
}

- (BOOL)readRenameData
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    eARSAL_ERROR retBLE = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 timeout:nil toKey:kARUTILS_BLEFtp_Getting];
    if (retBLE != ARSAL_OK)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    if (result == ARUTILS_OK)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen > 0)
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"%s", packet);
#endif
                if ((packetLen == (strlen(BLE_PACKET_RENAME_SUCCESS) + 1)) && (strncmp((char*)packet, BLE_PACKET_RENAME_SUCCESS, strlen(BLE_PACKET_RENAME_SUCCESS)) == 0))
                {
                    result = ARUTILS_OK;
                }
                else
                {
                    result = ARUTILS_ERROR_FTP_CODE;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_CONNECT;
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
    }
    
    return result;
}

- (BOOL)readDeleteData
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    eARSAL_ERROR retBLE = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 timeout:nil toKey:kARUTILS_BLEFtp_Getting];
    if (retBLE != ARSAL_OK)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    if (result == ARUTILS_OK)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen > 0)
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"%s", packet);
#endif
                if ((packetLen == (strlen(BLE_PACKET_DELETE_SUCCESS) + 1)) && (strncmp((char*)packet, BLE_PACKET_DELETE_SUCCESS, strlen(BLE_PACKET_DELETE_SUCCESS)) == 0))
                {
                    result = ARUTILS_OK;
                }
                else
                {
                    result = ARUTILS_ERROR_FTP_CODE;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_CONNECT;
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
    }
    return result;
}

/*- (eARUTILS_ERROR)readPutResumeIndex:(int*)resumeIndex
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    BOOL retBLE = YES;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readData:_getting];
    
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
    if (retBLE == NO)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    if (result == ARUTILS_OK)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen > 0)
            {
                if (packetLen == 3)
                {
                    int size = (0xFF & packet[0]) | (0xFF00 & (packet[1] << 8)) | (0xFF0000 & (packet[2] << 16));
                    *resumeIndex = size;
                    result = ARUTILS_OK;
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"resume index %d,  %02x, %02x, %02x", size, packet[0], packet[1], packet[2]);
#endif
                }
                else
                {
                    result = ARUTILS_ERROR_FTP_CODE;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_CONNECT;
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
    }
    
    return result;
}*/

- (eARUTILS_ERROR)readPutResumeIndex:(NSString*)remoteFile resumeIndex:(int*)resumeIndex
{
    double fileSize = 0.f;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    result = [self sizeFile:remoteFile fileSize:&fileSize];
    if ((result == ARUTILS_OK) && (fileSize > 0.f))
    {
        *resumeIndex = ((int)fileSize) / BLE_PACKET_MAX_SIZE;
    }
    
    return result;
}

- (BOOL)readPutDataWritten
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    eARSAL_ERROR retBLE = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 timeout:nil toKey:kARUTILS_BLEFtp_Getting];
    if (retBLE != ARSAL_OK)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    if (result == ARUTILS_OK)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen > 0)
            {
                if ((packetLen == (strlen(BLE_PACKET_WRITTEN) + 1)) && (strncmp((char*)packet, BLE_PACKET_WRITTEN, strlen(BLE_PACKET_WRITTEN)) == 0))
                {
                    result = ARUTILS_OK;
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"written OK");
#endif
                }
                else
                {
                    result = ARUTILS_ERROR_FTP_CODE;
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"UNKNOWN written");
#endif
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_CONNECT;
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
    }
    
    return result;
}

- (eARUTILS_ERROR)readPutMd5:(char*)md5Txt
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    eARSAL_ERROR retBLE = ARSAL_OK;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    
    *md5Txt = '\0';
    
    retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 timeout:nil toKey:kARUTILS_BLEFtp_Getting];
    if (retBLE != ARSAL_OK)
    {
        result = ARUTILS_ERROR_FTP_CONNECT;
    }
    if (result == ARUTILS_OK)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen)
            {
                if (packetLen == (CC_MD5_DIGEST_LENGTH * 2))
                {
                    strncpy(md5Txt, (char*)packet, packetLen);//TOFIX len
                    md5Txt[packetLen] = '\0';
                    result = ARUTILS_OK;
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"md5 end received %s", md5Txt);
#endif
                }
                else
                {
                    result = ARUTILS_ERROR_FTP_CODE;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_CONNECT;
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CONNECT;
        }
    }
    
    return result;
}

- (eARUTILS_ERROR)readGetData:(uint32_t)fileSize dstFile:(FILE*)dstFile data:(uint8_t**)data dataLen:(uint32_t*)dataLen progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg cancelSem:(ARSAL_Sem_t*)cancelSem
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    uint8_t md5[CC_MD5_DIGEST_LENGTH];
    char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    char md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    int packetCount = 0;
    int totalSize = 0;
    int totalPacket = 0;
    CC_MD5_CTX ctxEnd;
    CC_MD5_CTX ctx;
    eARSAL_ERROR retBLE = ARSAL_OK;
    BOOL endFile = NO;
    BOOL endMD5 = NO;
    int failedMd5 = 0;
    size_t count;
    uint8_t *oldData;
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
        
    CC_MD5_Init(&ctxEnd);
    while ((result == ARUTILS_OK) && (endMD5 == NO))
    {
        BOOL blockMD5 = NO;
        CC_MD5_Init(&ctx);
        
        do
        {
            if ([receivedNotifications count] == 0)
            {
                retBLE = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 timeout:[NSNumber numberWithFloat:5.0f] toKey:kARUTILS_BLEFtp_Getting];
                
            }
            if (retBLE != ARSAL_OK)
            {
                //no data available
                if (retBLE == ARSAL_ERROR_BLE_TIMEOUT)
                 {
                     result = [self sendCommand:"CANCEL" param:NULL characteristic:_getting];
                     result = ARUTILS_ERROR_FTP_CANCELED;
                    #if ARUTILS_BLEFTP_ENABLE_LOG
                         NSLog(@"ARSAL_ERROR_BLE_TIMEOUT result :%d",result);
                    #endif
                 }
                else
                {
                    if ([SINGLETON_FOR_CLASS(ARSAL_BLEManager) isPeripheralConnected] == NO)
                    {
                        result = ARUTILS_ERROR_FTP_CONNECT;
                    }
                }
            }
            else
            {
                for (int i=0; i<[receivedNotifications count] && (result == ARUTILS_OK) && (blockMD5 == NO) && (endMD5 == NO); i++)
                {
                    ARSALBLEManagerNotificationData *notificationData = receivedNotifications[i];
                    int packetLen = [[notificationData value] length];
                    uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
                    
                    packetCount++;
                    totalPacket++;
                    
                    if (packetLen > 0)
                    {
                        if (endFile == YES)
                        {
                            endMD5 = YES;
                            
                            if (packetLen == (CC_MD5_DIGEST_LENGTH * 2))
                            {
                                strncpy(md5Msg, (char*)packet, CC_MD5_DIGEST_LENGTH * 2);
                                md5Msg[CC_MD5_DIGEST_LENGTH * 2] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"md5 END received %s", packet);
#endif
                            }
                            else
                            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"md5 END Failed SIZE %d", packetLen);
#endif
                                result = ARUTILS_ERROR_FTP_SIZE;
                            }
                        }
                        else if (strncmp((char*)packet, BLE_PACKET_EOF, strlen(BLE_PACKET_EOF)) == 0)
                        {
                            endFile = YES;
                            
                            if (packetLen == (strlen(BLE_PACKET_EOF) + 1))
                            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"END received %d, %s", packetCount, packet);
#endif
                            }
                            else
                            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"END Failed SIZE %d", packetLen);
#endif
                                result = ARUTILS_ERROR_FTP_SIZE;
                            }
                        }
                        else if (strncmp((char*)packet, "MD5", 3) == 0)
                        {
                            if (packetCount > (BLE_PACKET_BLOCK_GETTING_COUNT + 1))
                            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"md5 FAILD packet COUNT %s", packet);
#endif
                            }
                            
                            if (packetLen == ((CC_MD5_DIGEST_LENGTH * 2) + 3))
                            {
                                blockMD5 = YES;
                                strncpy(md5Msg, (char*)(packet + 3), CC_MD5_DIGEST_LENGTH * 2);
                                md5Msg[CC_MD5_DIGEST_LENGTH * 2] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"md5 received %d, %s", packetCount, packet);
#endif
                            }
                            else
                            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                NSLog(@"md5 received Failed SIZE %d", packetLen);
#endif
                                result = ARUTILS_ERROR_FTP_SIZE;
                            }
                        }
                        else
                        {
                            totalSize += packetLen;
                            CC_MD5_Update(&ctx, packet, packetLen);
                            CC_MD5_Update(&ctxEnd, packet, packetLen);
                            
                            if (dstFile != NULL)
                            {
                                count = fwrite(packet, sizeof(char), packetLen, dstFile);
                            
                                if (count != packetLen)
                                {
#if ARUTILS_BLEFTP_ENABLE_LOG
                                    NSLog(@"failed writting file");
#endif
                                    result = ARUTILS_ERROR_FTP_FILE;
                                }
                            }
                            else
                            {
                                oldData = *data;
                                *data = realloc(*data, totalSize * sizeof(uint8_t));
                                if (*data == NULL)
                                {
                                    *data = oldData;
                                    result = ARUTILS_ERROR_ALLOC;
                                }
                                else
                                {
                                    memcpy(&(*data)[totalSize - packetLen], packet, packetLen);
                                    *dataLen += packetLen;
                                }
                            }
                            
                            if (progressCallback != NULL)
                            {
                                progressCallback(progressArg, ((float)totalSize / (float)fileSize) * 100.f);
                            }
#if ARUTILS_BLEFTP_ENABLE_LOG
                            NSLog(@"packet %d, %d, %d", packetCount, packetLen, totalSize);
#endif
                        }
                    }
                    else
                    {
                        //empty packet autorized
                    }
                }
            }
            
            [receivedNotifications removeAllObjects];
        }
        while ((result == ARUTILS_OK) && (blockMD5 == NO) && (endMD5 == NO));
        
        if ((result == ARUTILS_OK) && (blockMD5 == YES))
        {
            blockMD5 = NO;
            packetCount = 0;
            CC_MD5_Final(md5, &ctx);
            for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&md5Txt[i * 2], "%02x", md5[i]);
            }
            md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
            NSLog(@"md5 computed %s", md5Txt);
#endif
            
            if (strncmp(md5Txt, md5Msg, CC_MD5_DIGEST_LENGTH * 2) != 0)
            {
                failedMd5++;
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"MD5 block Failed");
#endif
                //TOFIX in firmware some 1st md5 are failed !!!!!!!!!!!
                //ret = NO;
            }
            else
            {
#if ARUTILS_BLEFTP_ENABLE_LOG
                NSLog(@"MD5 block OK");
#endif
            }
            
            //firmware 1.0.45 protocol dosen't implement cancel today at the and of 100 packets download
            if (cancelSem != NULL)
            {
                result = ARUTILS_BLEFtp_IsCanceledSem(cancelSem);
                if (result != ARUTILS_OK)
                {
#if ARUTILS_BLEFTP_ENABLE_LOG
                    NSLog(@"canceled received");
#endif
                }
            }
            
            if (result == ARUTILS_ERROR_FTP_CANCELED)
            {
                result = [self sendCommand:"CANCEL" param:NULL characteristic:_getting];
            }
            else
            {
                result = [self sendCommand:"MD5 OK" param:NULL characteristic:_getting];
            }
        }
    }

    if ((result == ARUTILS_OK) && (endMD5 == YES))
    {
        CC_MD5_Final(md5, &ctxEnd);
        for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        {
            sprintf(&md5Txt[i * 2], "%02x", md5[i]);
        }
        md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
#if ARUTILS_BLEFTP_ENABLE_LOG
        NSLog(@"md5 END computed %s", md5Txt);
        NSLog(@"file size %d", totalSize);
#endif
        
        if (strncmp(md5Txt, md5Msg, CC_MD5_DIGEST_LENGTH * 2) != 0)
        {
#if ARUTILS_BLEFTP_ENABLE_LOG
            NSLog(@"MD5 end Failed");
#endif
            result = ARUTILS_ERROR_FTP_MD5;
        }
        else
        {
#if ARUTILS_BLEFTP_ENABLE_LOG
            NSLog(@"MD5 end OK");
#endif
        }
      
#if ARUTILS_BLEFTP_ENABLE_LOG
        NSLog(@"Failed block MD5 %d", failedMd5);
#endif
    }
    
    if (cancelSem != NULL)
    {
        result = ARUTILS_BLEFtp_IsCanceledSem(cancelSem);
        if (result != ARUTILS_OK)
        {
#if ARUTILS_BLEFTP_ENABLE_LOG
            NSLog(@"canceled received");
#endif
        }
    }
    
    return result;
}

- (NSString*)normalizeName:(NSString*)name
{
    NSString *newName = name;
    if ((name.length > 0) && [name characterAtIndex:0] != '/')
    {
        newName = [NSString stringWithFormat:@"/%@", name];
    }
    return newName;
}

@end

ARUTILS_BLEFtp_Connection_t * ARUTILS_BLEFtp_Connection_New(ARSAL_Sem_t *cancelSem, ARUTILS_BLEDevice_t device, int port, eARUTILS_ERROR *error)
{
    ARUTILS_BLEFtp_Connection_t *newConnection = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if((port == 0) || ((port % 10) != 1))
    {
        *error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        newConnection = calloc(1, sizeof(ARUTILS_BLEFtp_Connection_t));
        if (newConnection != NULL)
        {
            CBPeripheral *peripheral = (__bridge CBPeripheral *)device;
            ARUtils_BLEFtp *bleFtpObject = SINGLETON_FOR_CLASS(ARUtils_BLEFtp);
            
            result = [bleFtpObject registerPeripheral:peripheral cancelSem:cancelSem port:port];
            if (result == ARUTILS_OK)
            {
                result = [bleFtpObject registerCharacteristics];
            }
            if (result == ARUTILS_OK)
            {
                newConnection->bleFtpObject = (__bridge_retained void *)bleFtpObject;
                newConnection->cancelSem = cancelSem;
            }
        }
    }
    
    if (result != ARUTILS_OK)
    {
        ARUTILS_BLEFtp_Connection_Delete(&newConnection);
    }
        
    *error = result;
    return newConnection;
}

void ARUTILS_BLEFtp_Connection_Delete(ARUTILS_BLEFtp_Connection_t **connectionAddr)
{
    if (connectionAddr != NULL)
    {
        ARUTILS_BLEFtp_Connection_t *connection = *connectionAddr;
        if (connection != NULL)
        {
            ARUtils_BLEFtp *bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
            [bleFtpObject unregisterPeripheral];
            [bleFtpObject unregisterCharacteristics];

            CFRelease(connection->bleFtpObject);
            connection->bleFtpObject = NULL;
            
            free(connection);
        }
        *connectionAddr = NULL;
    }
}

eARUTILS_ERROR ARUTILS_BLEFtp_Connection_Cancel(ARUTILS_BLEFtp_Connection_t *connection)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_BLEFTP_TAG, "");
    
    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        result = [bleFtpObject cancelFile:connection->cancelSem];
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Connection_IsCanceled(ARUTILS_BLEFtp_Connection_t *connection)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Sem_t *cancelSem = connection->cancelSem;
        
        if (cancelSem != NULL)
        {
            int resultSys = ARSAL_Sem_Trywait(cancelSem);
            
            if (resultSys == 0)
            {
                result = ARUTILS_ERROR_FTP_CANCELED;
                
                //give back the signal state lost from trywait
                ARSAL_Sem_Post(cancelSem);
            }
            else if (errno != EAGAIN)
            {
                result = ARUTILS_ERROR_SYSTEM;
            }
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Connection_Reset(ARUTILS_BLEFtp_Connection_t *connection)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Sem_t *cancelSem = connection->cancelSem;
        
        if (cancelSem != NULL)
        {
            while (ARSAL_Sem_Trywait(cancelSem) == 0)
            {
                /* Do Nothing */
            }
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_IsCanceledSem(ARSAL_Sem_t *cancelSem)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (cancelSem == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (cancelSem != NULL)
    {
        int resultSys = ARSAL_Sem_Trywait(cancelSem);
        
        if (resultSys == 0)
        {
            result = ARUTILS_ERROR_FTP_CANCELED;
            
            //give back the signal state lost from trywait
            ARSAL_Sem_Post(cancelSem);
        }
        else if (errno != EAGAIN)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    return result;
}


eARUTILS_ERROR ARUTILS_BLEFtp_List(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, char **resultList, uint32_t *resultListLen)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (resultList == NULL) || (resultListLen == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        *resultList = NULL;
        *resultListLen = 0;
        
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject listFiles:[NSString stringWithUTF8String:remotePath] resultList:resultList resultListLen:resultListLen];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Delete(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject deleteFile:[NSString stringWithUTF8String:remotePath]];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Rename(ARUTILS_BLEFtp_Connection_t *connection, const char *oldNamePath, const char *newNamePath)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject renameFile:[NSString stringWithUTF8String:oldNamePath] newNamePath:[NSString stringWithUTF8String:newNamePath]];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Get_WithBuffer(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject getFileWithBuffer:[NSString stringWithUTF8String:remotePath] data:data dataLen:dataLen progressCallback:progressCallback progressArg:progressArg cancelSem:connection->cancelSem];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Get(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject getFile:[NSString stringWithUTF8String:remotePath] localFile:[NSString stringWithUTF8String:dstFile] progressCallback:progressCallback progressArg:progressArg cancelSem:connection->cancelSem];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Put(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ARSAL_Mutex_Lock([bleFtpObject getConnectionLock]);
        
        result = [bleFtpObject putFile:[NSString stringWithUTF8String:remotePath] localFile:[NSString stringWithUTF8String:srcFile] progressCallback:progressCallback progressArg:progressArg resume:(resume == FTP_RESUME_TRUE) ? YES : NO cancelSem:connection->cancelSem];
        
        ARSAL_Mutex_Unlock([bleFtpObject getConnectionLock]);
    }
    
    return result;
}
/*****************************************
 *
 *             Abstract implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_Manager_InitBLEFtp(ARUTILS_Manager_t *manager, ARUTILS_BLEDevice_t device, int port)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;
    
    if ((manager == NULL) || (manager->connectionObject != NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        resultSys = ARSAL_Sem_Init(&manager->cancelSem, 0, 0);
        if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        manager->connectionObject = ARUTILS_BLEFtp_Connection_New(&manager->cancelSem, device, port, &result);
    }
    
    if (result == ARUTILS_OK)
    {
        manager->ftpConnectionDisconnect = ARUTILS_BLEFtpAL_Connection_Disconnect;
        manager->ftpConnectionReconnect = ARUTILS_BLEFtpAL_Connection_Reconnect;
        manager->ftpConnectionCancel = ARUTILS_BLEFtpAL_Connection_Cancel;
        manager->ftpConnectionIsCanceled = ARUTILS_BLEFtpAL_Connection_IsCanceled;
        manager->ftpConnectionReset = ARUTILS_BLEFtpAL_Connection_Reset;
        manager->ftpList = ARUTILS_BLEFtpAL_List;
        manager->ftpGetWithBuffer = ARUTILS_BLEFtpAL_Get_WithBuffer;
        manager->ftpGet = ARUTILS_BLEFtpAL_Get;
        manager->ftpPut = ARUTILS_BLEFtpAL_Put;
        manager->ftpDelete = ARUTILS_BLEFtpAL_Delete;
        manager->ftpRename = ARUTILS_BLEFtpAL_Rename;
    }
    
    return result;
}

void ARUTILS_Manager_CloseBLEFtp(ARUTILS_Manager_t *manager)
{
    if (manager != NULL)
    {
        ARUTILS_BLEFtp_Connection_Delete((ARUTILS_BLEFtp_Connection_t **)&manager->connectionObject);
        
        ARSAL_Sem_Destroy(&manager->cancelSem);
    }
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Reconnect(ARUTILS_Manager_t *manager)
{
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    return ARUTILS_OK;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Disconnect(ARUTILS_Manager_t *manager)
{
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    return ARUTILS_OK;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Cancel(ARUTILS_Manager_t *manager)
{
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    return ARUTILS_BLEFtp_Connection_Cancel((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_IsCanceled(ARUTILS_Manager_t *manager)
{
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    return ARUTILS_BLEFtp_Connection_IsCanceled((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Reset(ARUTILS_Manager_t *manager)
{
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    return ARUTILS_BLEFtp_Connection_Reset((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_List(ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    result = ARUTILS_BLEFtp_List((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, resultList, resultListLen);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get_WithBuffer(ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    eARUTILS_ERROR result = ARUTILS_OK;

#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    result = ARUTILS_BLEFtp_Get_WithBuffer((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, data, dataLen, progressCallback, progressArg);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get(ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    eARUTILS_ERROR result = ARUTILS_OK;

#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif
    result = ARUTILS_BLEFtp_Get((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, dstFile, progressCallback, progressArg, resume);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Put(ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif    
    result = ARUTILS_BLEFtp_Put((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, srcFile, progressCallback, progressArg, resume);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Delete(ARUTILS_Manager_t *manager, const char *namePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif    
    result = ARUTILS_BLEFtp_Delete((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Rename(ARUTILS_Manager_t *manager, const char *oldNamePath, const char *newNamePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s", __FUNCTION__);
#endif    
    result = ARUTILS_BLEFtp_Rename((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, oldNamePath, newNamePath);
#if ARUTILS_BLEFTP_ENABLE_LOG
    NSLog(@"%s exit", __FUNCTION__);
#endif
    return result;
}

