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

#include <libARSAL/ARSAL_Sem.h>
#include <libARSAL/ARSAL_Singleton.h>
#include <libARSAL/ARSAL_Print.h>
#include <curl/curl.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Ftp.h"
#include "libARUtils/ARUTILS_FileSystem.h"
#include "libARUtils/ARUTILS_BLEFtp.h"

struct ARUTILS_BLEFtp_Connection_t
{
    ARSAL_Sem_t *cancelSem;
    void *bleFtpObject;
};



#import <CoreBluetooth/CoreBluetooth.h>
#import <libARSAL/ARSAL_CentralManager.h>
#import <CommonCrypto/CommonDigest.h>

//#import "ARNETWORKAL_Manager.h"
#import <libARSAL/ARSAL_BLEManager.h>
//#import "ARNETWORKAL_BLENetwork.h"
#import "ARUTILS_BLEFtp.h"
#import <libARSAL/ARSAL_Endianness.h>

NSString* const kARUTILS_BLEFtp_Getting = @"kARUTILS_BLEFtp_Getting";

#define BLE_MD5_TXT_SIZE           (CC_MD5_DIGEST_LENGTH * 2)
#define BLE_PACKET_MAX_SIZE        132
#define BLE_PACKET_EOF             "End of Transfer"
#define BLE_PACKET_WRITTEN         "FILE WRITTEN"
#define BLE_PACKET_NOT_WRITTEN     "FILE NOT WRITTEN"
#define BLE_PACKET_
#define BLE_PACKET_BLOCK_GETTING_COUNT     100
#define BLE_PACKET_BLOCK_PUTTING_COUNT     500

//#define BLE_PACKET_WRITE_SLEEP             15000000
#define BLE_PACKET_WRITE_SLEEP             20000000

#define ARUTILS_BLEFTP_TAG      "BLEFtp"

@interface  ARUtils_BLEFtp ()
{
    
}

@property (nonatomic, assign) ARSAL_Sem_t* cancelSem;
@property (nonatomic, retain) CBPeripheral *peripheral;

@property (nonatomic, retain) CBCharacteristic *transferring;
@property (nonatomic, retain) CBCharacteristic *getting;
@property (nonatomic, retain) CBCharacteristic *handling;

@property (nonatomic, retain) NSArray *arrayGetting;

@end

@implementation ARUtils_BLEFtp

- (id)initWithPeripheral:(CBPeripheral *)peripheral cancelSem:(ARSAL_Sem_t*)cancelSem;
{
    self = [super init];
    if (self != nil)
    {
        _peripheral = peripheral;
        _cancelSem = cancelSem;
    }
    return self;
}

- (BOOL)registerCharacteristics
{
    eARSAL_ERROR result = ARSAL_OK;
    eARSAL_ERROR discoverCharacteristicsResult = ARSAL_OK;
    eARSAL_ERROR setNotifCharacteristicResult = ARSAL_OK;
    BOOL ret = NO;
    
    for(int i = 0 ; (i < [[_peripheral services] count]) && (result == ARSAL_OK) && ((_transferring == nil) || (_getting == nil) || (_handling == nil)) ; i++)
    {
        CBService *service = [[_peripheral services] objectAtIndex:i];
        NSLog(@"Service : %@, %04x", [service.UUID representativeString], (unsigned int)service.UUID);
        
        if([[service.UUID representativeString] hasPrefix:@"fd"])
        {
            discoverCharacteristicsResult = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkCharacteristics:nil forService:service];
            
            if (discoverCharacteristicsResult == ARSAL_OK)
            {
                result = ARSAL_OK;
                
                for (CBCharacteristic *characteristic in [service characteristics])
                {
                    NSLog(@"CBCharacteristic: %@", characteristic.UUID.representativeString);
                    
                    if ([characteristic.UUID.representativeString isEqualToString:@"fd01"])
                    {
                        if ((characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) == CBCharacteristicPropertyWriteWithoutResponse)
                        {
                            _transferring = characteristic;
                        }
                    }
                    else if ([characteristic.UUID.representativeString isEqualToString:@"fd02"])
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
                    else if ([characteristic.UUID.representativeString isEqualToString:@"fd03"])
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
        result = ARSAL_OK;
        ret = YES;
        
        if (ret == YES)
        {
            setNotifCharacteristicResult = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) setNotificationCharacteristic:_getting];
            if (setNotifCharacteristicResult != ARSAL_OK)
            {
                ret = NO;
            }
        }
        
        if (ret == YES)
        {
            [SINGLETON_FOR_CLASS(ARSAL_BLEManager) registerNotificationCharacteristics:_arrayGetting toKey:kARUTILS_BLEFtp_Getting];
        }
    }
    else
    {
        result = ARSAL_ERROR_BLE_CHARACTERISTICS_DISCOVERING;
        ret = NO;
    }
    
    return ret;
}

- (BOOL)unregisterCharacteristics
{
    BOOL ret = NO;
    
    return ret;
}

- (BOOL)listFiles:(NSString*)remotePath list:(NSMutableString*)list
{
    BOOL ret = NO;
    
    ret = [self sendCommand:"LIS" param:[remotePath UTF8String] characteristic:_handling];
    
    if (ret == YES)
    {
        ret = [self readDataList:list];
    }
    
    return ret;
}

- (BOOL)sizeFile:(NSString*)remoteFile fileSize:(double*)fileSize
{
    NSMutableString *list = [[NSMutableString alloc] init];
    BOOL ret = NO;

    *fileSize = 0.f;
    
    ret = [self listFiles:remoteFile list:list];
    
    if (ret == YES)
    {
        const char *filePath = [remoteFile UTF8String];
        const char *dataList = [list UTF8String];
        const char *nextItem = NULL;
        const char *fileName = NULL;
        const char *indexItem = NULL;
        int itemLen = 0;
        BOOL found = NO;
        
        while ((found == NO) &&(fileName = ARUTILS_Ftp_List_GetNextItem(dataList, &nextItem, NULL, 0, &indexItem, &itemLen)) != NULL)
        {
            if (strcmp(filePath, fileName) == 0)
            {
                found = YES;
                ARUTILS_Ftp_List_GetItemSize(indexItem, itemLen, fileSize);
            }
        }
        
        if (found == YES)
        {
            ret = YES;
        }
    }
    
    return ret;
}

- (BOOL)getFile:(NSString*)remoteFile localFile:(NSString*)localFile progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg
{
    FILE *dstFile = NULL;
    BOOL ret = NO;
    double totalSize = 0.f;
    
    ret = [self sizeFile:remoteFile fileSize:&totalSize];
    if (ret == YES)
    {
        ret = [self sendCommand:"GET" param:[remoteFile UTF8String] characteristic:_handling];
    }
    
    if (ret == YES)
    {
        dstFile = fopen([localFile UTF8String], "wb");
        if (dstFile == NULL)
        {
            ret = NO;
        }
    }
    
    if (ret == YES)
    {
        ret = [self readGetData:(uint32_t)totalSize dstFile:dstFile progressCallback:progressCallback progressArg:progressArg];
    }
    
    if (dstFile != NULL)
    {
        fclose(dstFile);
    }
    
    return ret;
}

- (BOOL)abortPutFile:(NSString*)remoteFile
{
    //char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    //uint8_t packet[BLE_PACKET_MAX_SIZE];
    int resumeIndex = 0;
    BOOL resume = NO;
    BOOL ret = YES;
    
    ret = [self readPutResumeIndex:&resumeIndex];
    if ((ret == YES) && (resumeIndex > 0))
    {
        resume = YES;
    }
    else
    {
        resume = NO;
    }
    
    if (resume == YES)
    {
        ret = [self sendCommand:"PUT" param:[remoteFile UTF8String] characteristic:_handling];
        
        /*if (ret == YES)
         {
         NSData *data = [NSData dataWithBytes:packet length:BLE_PACKET_MAX_SIZE];
         ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
         }
         
         if (ret == YES)
         {
         NSData *data = [[NSData alloc] init];
         ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
         }
         
         if (ret == YES)
         {
         ret = [self readPutMd5:md5Msg];
         }*/
        if (ret == YES)
        {
            ret = [self sendPutData:0 srcFile:NULL resumeIndex:0 resume:NO abort:YES progressCallback:NULL progressArg:NULL];
        }
    }
    
    return ret;
}

- (BOOL)putFile:(NSString*)remoteFile localFile:(NSString*)localFile progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg
{
    eARUTILS_ERROR error;
    FILE *srcFile = NULL;
    int resumeIndex = 0;
    BOOL resume = YES;
    BOOL ret = YES;
    uint32_t totalSize = 0;
    
    error = ARUTILS_FileSystem_GetFileSize([localFile UTF8String], &totalSize);
    if (error != ARUTILS_OK)
    {
        ret = NO;
    }
    
    if (ret == YES)
    {
        ret = [self readPutResumeIndex:&resumeIndex];
        if (ret == NO)
        {
            ret = YES;
            resumeIndex = 0;
            resume = NO;
        }
    }
    
    if (resumeIndex > 0)
    {
        resume = YES;
    }
    
    if (ret == YES)
    {
        ret = [self sendCommand:"PUT" param:[remoteFile UTF8String] characteristic:_handling];
    }
    
    srcFile = fopen([localFile UTF8String], "rb");
    if (srcFile == NULL)
    {
        ret = NO;
    }
    
    if (ret == YES)
    {
        ret = [self sendPutData:totalSize srcFile:srcFile resumeIndex:resumeIndex resume:resume abort:NO progressCallback:progressCallback progressArg:progressArg];
    }
    
    if (srcFile != NULL)
    {
        fclose(srcFile);
    }
    
    return ret;
}

- (BOOL)deleteFile:(NSString*)remoteFile
{
    BOOL ret = YES;
    
    ret = [self sendCommand:"DEL" param:[remoteFile UTF8String] characteristic:_handling];
    
    return ret;
}

- (BOOL)sendCommand:(const char *)cmd param:(const char*)param characteristic:(CBCharacteristic *)characteristic
{
    char command[BLE_PACKET_MAX_SIZE];
    BOOL ret = YES;
    int len = 0;
    
    strncpy(command, cmd, BLE_PACKET_MAX_SIZE);
    command[BLE_PACKET_MAX_SIZE - 1] = '\0';
    
    if (param != NULL)
    {
        strncat(command, param, BLE_PACKET_MAX_SIZE - strlen(command));
    }
    len = strlen(command) + 1;
    
    NSData *data = [NSData dataWithBytes:command length:len];
    ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:characteristic];
    
    return ret;
}

- (BOOL)sendPutData:(uint32_t)fileSize srcFile:(FILE*)srcFile resumeIndex:(int)resumeIndex resume:(BOOL)resume abort:(BOOL)abort progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg
{
    uint8_t md5[CC_MD5_DIGEST_LENGTH];
    char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    char md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 3 + 1];
    uint8_t packet[BLE_PACKET_MAX_SIZE];
    CC_MD5_CTX ctxEnd;
    CC_MD5_CTX ctx;
    BOOL ret = YES;
    int totalSize = 0;
    int packetCount = 0;
    int totalPacket = 0;
    int packetLen = BLE_PACKET_MAX_SIZE;
    BOOL endFile = NO;
    ARSAL_Sem_t timeSem;
    struct timespec timeout;
    eARUTILS_ERROR error;
    
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
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
                
                if (progressCallback != NULL)
                {
                    progressCallback(progressArg, (fileSize / totalSize) * 100);
                }
                
                error = ARUTILS_BLEFtp_IsCanceledSem(_cancelSem);
                if (error != ARUTILS_OK)
                {
                    ret = NO;
                }
                
                NSLog(@"packet %d, %d, %d", packetCount, packetLen, totalSize);
            }
            else
            {
                NSLog(@"resume %d, %d, %d", packetCount, BLE_PACKET_MAX_SIZE, totalSize);
            }
        }
        else
        {
            if (feof(srcFile))
            {
                endFile = YES;
            }
        }
        
        if ((ret == YES) && ((packetCount >= BLE_PACKET_BLOCK_PUTTING_COUNT) || ((endFile == YES) && (packetCount > 0))))
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
                
                NSLog(@"sending md5: %s", md5Txt);
                
                ARSAL_Sem_Timedwait(&timeSem, &timeout);
                NSData *data = [NSData dataWithBytes:md5Txt length:strlen(md5Txt)];
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
                if (ret == YES)
                {
                    ret = [self readPutDataWritten];
                }
            }
        }
    }
    while ((ret == YES) && (endFile == NO));
    
    if ((ret == YES) && (endFile == YES))
    {
        ARSAL_Sem_Timedwait(&timeSem, &timeout);
        NSData *data = [[NSData alloc] init];
        ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) writeData:data toCharacteristic:_transferring];
        if (ret == YES)
        {
            ret = [self readPutMd5:md5Msg];
        }
        
        if (ret == YES)
        {
            CC_MD5_Final(md5, &ctxEnd);
            for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&md5Txt[i * 2], "%02x", md5[i]);
            }
            md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
            NSLog(@"md5 end %s", md5Txt);
            NSLog(@"file size %d", totalSize);
            
            if (strncmp(md5Msg, md5Txt, CC_MD5_DIGEST_LENGTH * 2) != 0)
            {
                NSLog(@"MD5 End Failed");
                ret = NO;
            }
            else
            {
                NSLog(@"MD5 End OK");
            }
        }
    }
    
    ARSAL_Sem_Destroy(&timeSem);
    
    return ret;
}

- (BOOL)readPutResumeIndex:(int*)resumeIndex
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    BOOL ret = YES;
    
    [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readData:_getting];
    
    ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
    if (ret == YES)
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
                    NSLog(@"resume index %d,  %02x, %02x, %02x", size, packet[0], packet[1], packet[2]);
                }
                else
                {
                    ret = NO;
                }
            }
            else
            {
                ret = NO;
            }
        }
        else
        {
            ret = NO;
        }
    }
    
    return ret;
}

- (BOOL)readPutDataWritten
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    BOOL ret = NO;
    
    ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
    if (ret == YES)
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
                    ret = YES;
                    NSLog(@"written OK");
                }
                else if ((packetLen == (strlen(BLE_PACKET_NOT_WRITTEN) + 1)) && (strncmp((char*)packet, BLE_PACKET_NOT_WRITTEN, strlen(BLE_PACKET_NOT_WRITTEN)) == 0))
                {
                    ret = NO;
                    NSLog(@"NOT written");
                }
                else
                {
                    ret = NO;
                    NSLog(@"UNKNOWN written");
                }
            }
            else
            {
                ret = NO;
                NSLog(@"UNKNOWN written");
            }
        }
        else
        {
            ret = NO;
            NSLog(@"UNKNOWN written");
        }
    }
    
    return ret;
}

- (BOOL)readPutMd5:(char*)md5Txt
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    BOOL ret = YES;
    
    *md5Txt = '\0';
    
    ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
    if (ret == YES)
    {
        if ([receivedNotifications count] > 0)
        {
            ARSALBLEManagerNotificationData *notificationData = receivedNotifications[0];
            int packetLen = [[notificationData value] length];
            uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
            
            if (packetLen == (CC_MD5_DIGEST_LENGTH * 2))
            {
                strncpy(md5Txt, (char*)packet, packetLen);//TOFIX len
                md5Txt[packetLen] = '\0';
                NSLog(@"md5 end received %s", md5Txt);
            }
            else
            {
                ret = NO;
            }
        }
        else
        {
            ret = NO;
        }
    }
    
    return ret;
}

- (BOOL)readGetData:(uint32_t)fileSize dstFile:(FILE*)dstFile progressCallback:(ARUTILS_Ftp_ProgressCallback_t)progressCallback progressArg:(void *)progressArg
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    //NSMutableArray *removeNotifications = [NSMutableArray array];
    uint8_t md5[CC_MD5_DIGEST_LENGTH];
    char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    char md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    int packetCount = 0;
    int totalSize = 0;
    int totalPacket = 0;
    CC_MD5_CTX ctxEnd;
    CC_MD5_CTX ctx;
    BOOL ret = YES;
    BOOL endFile = NO;
    BOOL endMD5 = NO;
    int failedMd5 = 0;
    size_t count;
    eARUTILS_ERROR error;
    
    CC_MD5_Init(&ctxEnd);
    while ((ret == YES) && (endMD5 == NO))
    {
        BOOL blockMD5 = NO;
        CC_MD5_Init(&ctx);
        
        do
        {
            if ([receivedNotifications count] == 0)
            {
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
            }
            if (ret == NO)
            {
                //no data available
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) isPeripheralConnected];
            }
            else
            {
                for (int i=0; i<[receivedNotifications count] && (ret == YES) && (blockMD5 == NO) && (endMD5 == NO); i++)
                {
                    ARSALBLEManagerNotificationData *notificationData = receivedNotifications[i];
                    int packetLen = [[notificationData value] length];
                    uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
                    
                    packetCount++;
                    totalPacket++;
                    //[removeNotifications addObject:notificationData];
                    
                    if (packetLen > 0)
                    {
                        if (endFile == YES)
                        {
                            endMD5 = YES;
                            
                            if (packetLen == (CC_MD5_DIGEST_LENGTH * 2))
                            {
                                strncpy(md5Msg, (char*)packet, CC_MD5_DIGEST_LENGTH * 2);
                                md5Msg[CC_MD5_DIGEST_LENGTH * 2] = '\0';
                                NSLog(@"md5 END received %s", packet);
                            }
                            else
                            {
                                NSLog(@"md5 END Failed SIZE %d", packetLen);
                                ret = NO;
                            }
                        }
                        else if (strncmp((char*)packet, BLE_PACKET_EOF, strlen(BLE_PACKET_EOF)) == 0)
                        {
                            endFile = YES;
                            
                            if (packetLen == (strlen(BLE_PACKET_EOF) + 1))
                            {
                                NSLog(@"END received %d, %s", packetCount, packet);
                            }
                            else
                            {
                                NSLog(@"END Failed SIZE %d", packetLen);
                                ret = NO;
                            }
                        }
                        else if (strncmp((char*)packet, "MD5", 3) == 0)
                        {
                            if (packetCount > (BLE_PACKET_BLOCK_GETTING_COUNT + 1))
                            {
                                NSLog(@"md5 FAILD packet COUNT %s", packet);
                            }
                            
                            if (packetLen == ((CC_MD5_DIGEST_LENGTH * 2) + 3))
                            {
                                blockMD5 = YES;
                                strncpy(md5Msg, (char*)(packet + 3), CC_MD5_DIGEST_LENGTH * 2);
                                md5Msg[CC_MD5_DIGEST_LENGTH * 2] = '\0';
                                
                                NSLog(@"md5 received %d, %s", packetCount, packet);
                            }
                            else
                            {
                                NSLog(@"md5 received Failed SIZE %d", packetLen);
                                ret = NO;
                            }
                        }
                        else
                        {
                            totalSize += packetLen;
                            CC_MD5_Update(&ctx, packet, packetLen);
                            CC_MD5_Update(&ctxEnd, packet, packetLen);
                            count = fwrite(packet, sizeof(char), packetLen, dstFile);
                            if (count != packetLen)
                            {
                                NSLog(@"failed writting file");
                                ret = NO;
                            }
                            
                            if (progressCallback != NULL)
                            {
                                progressCallback(progressArg, (fileSize / totalSize) * 100);
                            }
                            
                            error = ARUTILS_BLEFtp_IsCanceledSem(_cancelSem);
                            if (error != ARUTILS_OK)
                            {
                                ret = NO;
                            }
                            
                            //NSLog(@"packet %d, %d, %d", packetCount, packetLen, totalSize);
                        }
                    }
                    else
                    {
                        //empty packet autorized
                    }
                }
            }
            
            //[receivedNotifications removeObjectsInArray:removeNotifications];
            //[removeNotifications removeAllObjects];
            [receivedNotifications removeAllObjects];
        }
        while ((ret == YES) && (blockMD5 == NO) && (endMD5 == NO));
        
        if ((ret == YES) && (blockMD5 == YES))
        {
            blockMD5 = NO;
            packetCount = 0;
            CC_MD5_Final(md5, &ctx);
            for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&md5Txt[i * 2], "%02x", md5[i]);
            }
            md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
            NSLog(@"md5 computed %s", md5Txt);
            
            if (strncmp(md5Txt, md5Msg, CC_MD5_DIGEST_LENGTH * 2) != 0)
            {
                failedMd5++;
                NSLog(@"MD5 block Failed");
                //TOFIX some 1st md5 are failed !!!!!!!!!!!
                //ret = NO;
            }
            else
            {
                NSLog(@"MD5 block OK");
            }
            
            ret = [self sendCommand:"MD5 OK" param:NULL characteristic:_getting];
        }
    }
    
    if ((ret == YES) && (endMD5 == YES))
    {
        CC_MD5_Final(md5, &ctxEnd);
        for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        {
            sprintf(&md5Txt[i * 2], "%02x", md5[i]);
        }
        md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
        NSLog(@"md5 END computed %s", md5Txt);
        NSLog(@"file size %d", totalSize);
        
        if (strncmp(md5Txt, md5Msg, CC_MD5_DIGEST_LENGTH * 2) != 0)
        {
            NSLog(@"MD5 end Failed");
            ret = NO;
        }
        else
        {
            NSLog(@"MD5 end OK");
        }
        
        NSLog(@"Failed block MD5 %d", failedMd5);
    }
    else
    {
        ret = NO;
    }
    
    return ret;
}

- (BOOL)readDataList:(NSMutableString*)list
{
    NSMutableArray *receivedNotifications = [NSMutableArray array];
    //NSMutableArray *removeNotifications = [NSMutableArray array];
    uint8_t md5[CC_MD5_DIGEST_LENGTH];
    char md5Msg[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    char md5Txt[(CC_MD5_DIGEST_LENGTH * 2) + 1];
    CC_MD5_CTX ctxEnd;
    int packetCount = 0;
    int totalSize = 0;
    BOOL endFile = NO;
    //BOOL endMD5 = NO;
    BOOL ret = YES;
    
    CC_MD5_Init(&ctxEnd);
    
    while ((ret == YES) && (endFile == NO) /*&& (endMD5 == NO)*/)
    {
        do
        {
            if ([receivedNotifications count] == 0)
            {
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) readNotificationData:receivedNotifications maxCount:1 toKey:kARUTILS_BLEFtp_Getting];
            }
            if (ret == NO)
            {
                //no data available
                ret = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) isPeripheralConnected];
            }
            else
            {
                for (int i=0; i<[receivedNotifications count] && (ret == YES) && (endFile == NO) /*&& (endMD5 == NO)*/; i++)
                {
                    ARSALBLEManagerNotificationData *notificationData = receivedNotifications[i];
                    int packetLen = [[notificationData value] length];
                    uint8_t *packet = (uint8_t *)[[notificationData value] bytes];
                    
                    packetCount++;
                    //[removeNotifications addObject:notificationData];
                    
                    NSLog(@"%s", packet);
                    
                    if (packetLen > 0)
                    {
                        /*if (endFile == YES)
                         {
                         endMD5 = YES;
                         strncpy(md5Msg, (char*)packet, CC_MD5_DIGEST_LENGTH * 2);
                         md5Msg[CC_MD5_DIGEST_LENGTH * 2] = '\0';
                         
                         NSLog(@"md5 END received %s", packet);
                         }
                         else*/ if (strncmp((char*)packet, BLE_PACKET_EOF, strlen(BLE_PACKET_EOF)) == 0)
                         {
                             endFile = YES;
                             
                             if (packetLen == (strlen(BLE_PACKET_EOF) + 1))
                             {
                                 NSLog(@"END received %d, %s", packetCount, packet);
                             }
                             else
                             {
                                 NSLog(@"END Failed SIZE %d", packetLen);
                                 ret = NO;
                             }
                         }
                         else
                         {
                             totalSize += packetLen;
                             
                             char txt[BLE_PACKET_MAX_SIZE + 1];
                             CC_MD5_Update(&ctxEnd, packet, packetLen);
                             strncpy(txt, (char*)packet, (packetLen <= BLE_PACKET_MAX_SIZE) ? packetLen : BLE_PACKET_MAX_SIZE);
                             txt[BLE_PACKET_MAX_SIZE] = '\0';
                             [list appendString:[NSString stringWithCString:txt encoding:NSASCIIStringEncoding]];
                             
                             //NSLog(@"%s", packet);
                             NSLog(@"packet %d, %d, %d", packetCount, packetLen, totalSize);
                         }
                    }
                    else
                    {
                        endFile = YES;
                        NSLog(@"end received");
                    }
                }
            }
            
            //[receivedNotifications removeObjectsInArray:removeNotifications];
            //[removeNotifications removeAllObjects];
            [receivedNotifications removeAllObjects];
        }
        while ((ret == YES) && (endFile == NO) /*&& (endMD5 == NO)*/);
    }
    
    if ((ret == YES) && (endFile == YES) /*&& (endMD5 == YES)*/)
    {
        CC_MD5_Final(md5, &ctxEnd);
        for (int i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        {
            sprintf(&md5Txt[i * 2], "%02x", md5[i]);
        }
        md5Txt[CC_MD5_DIGEST_LENGTH * 2] = '\0';
        NSLog(@"md5 END computed %s", md5Txt);
        
        if (strncmp(md5Txt, md5Msg, CC_MD5_DIGEST_LENGTH * 2) != 0)
        {
            NSLog(@"MD5 End Failed");
            ret = NO;
        }
        else
        {
            NSLog(@"MD5 End OK");
        }
        
        NSLog(@"end of");
    }
    else
    {
        ret = NO;
    }
    
    return ret;
}

@end

ARUTILS_BLEFtp_Connection_t * ARUTILS_BLEFtp_Connection_New(ARSAL_Sem_t *cancelSem, ARUTILS_BLEDevice_t device, eARUTILS_ERROR *error)
{
    ARUTILS_BLEFtp_Connection_t *newConnection = NULL;
    
    newConnection = calloc(1, sizeof(ARUTILS_BLEFtp_Connection_t));
    if (newConnection != NULL)
    {
        CBPeripheral *peripheral = (__bridge CBPeripheral *)device;
        ARUtils_BLEFtp *bleFtpObject = [[ARUtils_BLEFtp alloc] initWithPeripheral:peripheral cancelSem:cancelSem];
        [bleFtpObject registerCharacteristics];
        
        newConnection->bleFtpObject = (__bridge_retained void *)bleFtpObject;
    }
    
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
    eARUTILS_ERROR result = ARUTILS_OK;
    int resutlSys = 0;
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_BLEFTP_TAG, "");
    
    if ((connection == NULL) || (connection->cancelSem == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        resutlSys = ARSAL_Sem_Post(connection->cancelSem);
        
        if (resutlSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_IsCanceled(ARUTILS_BLEFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if ((connection != NULL) && (connection->cancelSem != NULL))
    {
        int resultSys = ARSAL_Sem_Trywait(connection->cancelSem);
        
        if (resultSys == 0)
        {
            result = ARUTILS_ERROR_FTP_CANCELED;
            
            //give back the signal state lost from trywait
            ARSAL_Sem_Post(connection->cancelSem);
        }
        else if (errno != EAGAIN)
        {
            result = ARUTILS_ERROR_SYSTEM;
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
    
    if ((cancelSem != NULL))
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
    BOOL ret = NO;
    
    if ((connection == NULL) || (resultList == NULL) || (resultListLen == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        *resultList = NULL;
        *resultListLen = 0;
        
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        NSMutableString *list = [[NSMutableString alloc] init];
        
        ret = [bleFtpObject listFiles:[NSString stringWithUTF8String:remotePath] list:list];
        if (ret == NO)
        {
            result = ARUTILS_ERROR_BLE_FAILED;
        }
        else
        {
            if (list.length > 0)
            {
                const char *dataList = [list UTF8String];
                int dataLen = list.length;
            
                char *stringList = malloc(dataLen + 1);
                if (stringList == NULL)
                {
                    result = ARUTILS_ERROR_ALLOC;
                }
            
                if (result == ARUTILS_OK)
                {
                    memcpy(stringList, dataList, dataLen);
                    stringList[dataLen] = '\0';
                    
                    *resultList = stringList;
                    *resultListLen = dataLen + 1;
                }
            }
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Delete(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    BOOL ret = NO;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        ret = [bleFtpObject deleteFile:[NSString stringWithUTF8String:remotePath]];
        if (ret == NO)
        {
            result = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Get_WithBuffer(ARUTILS_BLEFtp_Connection_t *connection, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    BOOL ret = NO;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        
        //ret = [bleFtpObject getFileWithBuffer:[NSString stringWithUTF8String:remotePath] localFile:[NSString stringWithUTF8String:dstFile]];
        if (ret == NO)
        {
            result = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Get(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    BOOL ret = NO;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        
        ret = [bleFtpObject getFile:[NSString stringWithUTF8String:remotePath] localFile:[NSString stringWithUTF8String:dstFile] progressCallback:progressCallback progressArg:progressArg];
        if (ret == NO)
        {
            result = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtp_Put(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARUtils_BLEFtp *bleFtpObject = nil;
    eARUTILS_ERROR result = ARUTILS_OK;
    BOOL ret = NO;
    
    if ((connection == NULL) || (connection->bleFtpObject == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        bleFtpObject = (__bridge ARUtils_BLEFtp *)connection->bleFtpObject;
        
        ret = [bleFtpObject putFile:[NSString stringWithUTF8String:remotePath] localFile:[NSString stringWithUTF8String:srcFile] progressCallback:progressCallback progressArg:progressArg];
        if (ret == NO)
        {
            result = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    return result;
}
/*****************************************
 *
 *             Abstract implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_Manager_NewBLEFtp(ARUTILS_Manager_t *manager, ARSAL_Sem_t *cancelSem, ARUTILS_BLEDevice_t device)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if ((manager == NULL) || (manager->connectionObject != NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        manager->connectionObject = ARUTILS_BLEFtp_Connection_New(cancelSem, device, &result);
    }
    
    if (result == ARUTILS_OK)
    {
        manager->ftpConnectionCancel = ARUTILS_BLEFtpAL_Connection_Cancel;
        manager->ftpList = ARUTILS_BLEFtpAL_List;
        manager->ftpGetWithBuffer = ARUTILS_BLEFtpAL_Get_WithBuffer;
        manager->ftpGet = ARUTILS_BLEFtpAL_Get;
        manager->ftpPut = ARUTILS_BLEFtpAL_Put;
        manager->ftpDelete = ARUTILS_BLEFtpAL_Delete;
    }
    
    return result;
}

void ARUTILS_Manager_DeleteBLEFtp(ARUTILS_Manager_t *manager)
{
    if (manager != NULL)
    {
        ARUTILS_BLEFtp_Connection_Delete((ARUTILS_BLEFtp_Connection_t **)&manager->connectionObject);
    }
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Cancel(ARUTILS_Manager_t *manager)
{
    return ARUTILS_BLEFtp_Connection_Cancel((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_List(ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    return ARUTILS_BLEFtp_List((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, resultList, resultListLen);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get_WithBuffer(ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_BLEFtp_Get_WithBuffer((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, data, dataLen, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get(ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_BLEFtp_Get((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, dstFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Put(ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_BLEFtp_Put((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, srcFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Delete(ARUTILS_Manager_t *manager, const char *namePath)
{
    return ARUTILS_BLEFtp_Delete((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath);
}

