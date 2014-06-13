//
//  autoTest.m
//  ARTest
//
//  Created by David on 13/05/2014.
//  Copyright (c) 2014 ___PARROT___. All rights reserved.
//

#import <libARSAL/ARSAL_CentralManager.h>

#import <libARDiscovery/ARDISCOVERY_BonjourDiscovery.h>
//#import "ARNETWORKAL_BLEManager.h"
//#import "ARNETWORKAL_BLENetwork.h"
//#import "ARNETWORKAL_Manager.h"
#include <libARSAL/ARSAL_Error.h>
#include <libARSAL/ARSAL_Singleton.h>
#include <libARSAL/ARSAL_BLEManager.h>
#import "ARUTILS_BLEFtp.h"

#include <libARSAL/ARSAL_MD5_Manager.h>

#import "autoTest.h"



@implementation autoTest

- (void)testMd5
{
    eARSAL_ERROR result = ARSAL_OK;
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    const char* doc = [[paths lastObject] cString];
    char path[512];
    uint8_t md5[16];
    //char md5Txt[33];
    
    ARSAL_MD5_Manager_t *manager = ARSAL_MD5_Manager_New(&result);
    
    ARSAL_MD5_Manager_Init(manager);
    
    strcpy(path, doc);
    strcat(path, "/txt.txt");
    
    FILE *file = fopen(path, "wb");
    fwrite("123\n", 1, 4, file);
    fflush(file);
    fclose(file);
    
    result = ARSAL_MD5_Manager_Compute(manager, path, md5, sizeof(md5));
    
    result = ARSAL_MD5_Manager_Check(manager, path, "ba1f2511fc30423bdbb183fe33f3dd0f");
    
    ARSAL_MD5_Manager_Close(manager);
    
    ARSAL_MD5_Manager_Delete(&manager);
    
}

- (void)testConnection
{
    //[self performSelectorInBackground:@selector(testScann) withObject:nil];
    
    NSThread *thread = [[NSThread alloc] initWithTarget:self selector:@selector(testScann) object:nil];
    [thread start];
    
    //[self testMd5];
}

- (void)testScann
{
    ARBLEService *foundService = nil;
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //const char* doc = [[paths lastObject] cString];
    NSString *doc = [paths lastObject];
    
    [[ARDiscovery sharedInstance] start];
    
    while (foundService == nil)
    {
        [NSThread sleepForTimeInterval:1];
        
        for (ARService *obj in [[ARDiscovery sharedInstance] getCurrentListOfDevicesServices])
        {
            if ([obj.service isKindOfClass:[ARBLEService class]])
            {
                ARBLEService *serviceIdx = (ARBLEService *)obj.service;
                NSLog(@"%@", serviceIdx.peripheral.name);
                //NSString *NAME = @"Delos_DF";
                //NSString *NAME = @"Mykonos_BLE";
                //NSString *NAME = @"Mykonos_DF";
                //NSString *NAME = @"RS_Rouge";
                //NSString *NAME = @"RS_W000207";
                NSString *NAME = @"RS_W000159";
                if ([serviceIdx.peripheral.name isEqualToString:NAME])
                {
                    NSLog(@"%@", serviceIdx.peripheral);
                    foundService = serviceIdx;
                    break;
                }
            }
        }
    }
    
    //[[ARDiscovery sharedInstance] stop];
    ARSAL_Sem_t cancelSem;
    eARSAL_ERROR result = ARSAL_OK;
    ARSAL_CentralManager *centralManager = foundService.centralManager;
    CBPeripheral *peripheral = foundService.peripheral;
    BOOL ret = NO;
    
    ARSAL_Sem_Init(&cancelSem, 0, 0);
    
    //ARUtils_BLEFtp *bleFtp = [[ARUtils_BLEFtp alloc] init];
    
    [NSThread sleepForTimeInterval:1];
    
    result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) connectToPeripheral:foundService.peripheral withCentralManager:centralManager];
    
    if  (result == ARSAL_OK)
    {
        result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkServices:nil];
    }
    
    //CBService *service = [[_peripheral services]
    /*for (CBService *service in [peripheral services])
    {
        NSLog(@"Service : %@, %04x", [service.UUID representativeString], (unsigned int)service.UUID);
        if([[service.UUID representativeString] hasPrefix:@"f"])
        {
            result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkCharacteristics:nil forService:service];
            
            for (CBCharacteristic *characteristic in [service characteristics])
            {
                NSLog(@"Service : %@, %04x", [characteristic.UUID representativeString], (unsigned int)characteristic.UUID);
            }
        }
    }*/
    for(int i = 0 ; (i < [[peripheral services] count]) && (result == ARSAL_OK) ; i++)
    {
        CBService *service = [[peripheral services] objectAtIndex:i];
        NSLog(@"Service : %@, %04x", [service.UUID representativeString], (unsigned int)service.UUID);
        
        if([[service.UUID representativeString] hasPrefix:@"f"])
        {
            result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkCharacteristics:nil forService:service];
            
            for(int j = 0 ; (j < [[service characteristics] count]) && (result == ARSAL_OK) ; j++)
            {
                CBCharacteristic *characteristic = [[service characteristics] objectAtIndex:j];
                NSLog(@"Characteristic : %@, %04x", [characteristic.UUID representativeString], (unsigned int)characteristic.UUID);
                
                if ([[characteristic.UUID representativeString] hasPrefix:@"fd23"]
                    || [[characteristic.UUID representativeString] hasPrefix:@"fd53"])
                {
                    result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) setNotificationCharacteristic:characteristic];
                    NSLog(@"==REGISTERED Characteristic : %@, %04x", [characteristic.UUID representativeString], (unsigned int)characteristic.UUID);
                }
            }
        }
    }

    if  (result == ARSAL_OK)
    {
        //[bleFtp initWithManager:(__bridge ARNETWORKAL_BLEDeviceManager_t)centralManager device:(__bridge ARNETWORKAL_BLEDevice_t)peripheral];
        //[bleFtp initWithManager:SINGLETON_FOR_CLASS(ARSAL_BLEManager) centralManager:centralManager peripheral:peripheral delegate:nil obj:self];
        //ARUtils_BLEFtp *bleFtp = [[ARUtils_BLEFtp alloc] initWithPeripheral:peripheral cancelSem:NULL port:51];
        ARUtils_BLEFtp *bleFtp = [[ARUtils_BLEFtp alloc] initWithPeripheral:peripheral cancelSem:&cancelSem port:21];
        
        ret = [bleFtp registerCharacteristics];
        
        /*if (ret == YES)
        {
            NSMutableString *list = [[NSMutableString alloc] init];
            ret = [bleFtp listFiles:@"/update/" list:list];
            
            NSLog(@"LIST: %@", list);
        }*/
        
        if (ret == YES)
        {
            //NSMutableString *list = [[NSMutableString alloc] init];
            char *resultList = NULL;
            uint32_t resultListLen = 0;
            ret = [bleFtp listFiles:@"/internal_000/Rolling_Spider/media" resultList:&resultList resultListLen:&resultListLen];
            //ret = [bleFtp listFiles:@"/internal_000/Rolling_Spider/thumb" resultList:&resultList resultListLen:&resultListLen];
            //ret = [bleFtp listFiles:@"/internal_000/Rolling_Spider/" resultList:&resultList resultListLen:&resultListLen];
            //ret = [bleFtp listFiles:@"/Rolling_Spider" resultList:&resultList resultListLen:&resultListLen];
            //ret = [bleFtp listFiles:@"/" resultList:&resultList resultListLen:&resultListLen];
            
            NSLog(@"LIST: %s", resultList);
        }
        
        /*if (ret == YES)
        {
            //ret = [bleFtp renameFile:@"/internal_000/Rolling_Spider/media/Rolling_Spider_2014-01-21T160101+0100_3902B87F947BE865A9D137CFA63492B8.jpg" newNamePath:@"/internal_000/Rolling_Spider/media/Rolling_Spider_2014-01-21T160101+0100_3.jpg"];
            ret = [bleFtp renameFile:@"/internal_000/Rolling_Spider/media/test12.jpg" newNamePath:@"/internal_000/Rolling_Spider/media/test13.jpg"];
        }*/
        
        /*if (ret == YES)
        {
            //NSMutableString *list = [[NSMutableString alloc] init];
            char *resultList = NULL;
            uint32_t resultListLen = 0;
            ret = [bleFtp listFiles:@"/internal_000/Rolling_Spider/media" resultList:&resultList resultListLen:&resultListLen];

            NSLog(@"LIST: %s", resultList);
        }*/
        
        if (ret == YES)
        {
            NSString *localFile = [NSString stringWithFormat:@"%@/test.jpg", doc];
            
            //ret = [bleFtp getFile:@"/update/test.jpg" localFile:localFile];
            //ret = [bleFtp getFile:@"/update/test.txt" localFile:localFile];
            //ret = [bleFtp getFile:@"/update/a.txt" localFile:localFile];
            //ret = [bleFtp getFile:@"/update/program.plf" localFile:localFile progressCallback:NULL progressArg:NULL];
            
            //ARSAL_Sem_Post(&cancelSem);
            //ret = [bleFtp cancelFile];
            
            //ret = [bleFtp getFile:@"/internal_000/Rolling_Spider/media/Rolling_Spider_2014-01-21T160101+0100_3902B87F947BE865A9D137CFA63492B8.jpg" localFile:localFile progressCallback:NULL progressArg:NULL];
            ret = [bleFtp getFile:@"/internal_000/Rolling_Spider/media/Rolling_Spider_2014-01-21T160101+0100_3.jpg" localFile:localFile progressCallback:NULL progressArg:NULL];
            //ret = [bleFtp getFile:@"/internal_000/Rolling_Spider/media/test1.jpg" localFile:localFile progressCallback:NULL progressArg:NULL];
        }
        
        /*if (ret == YES)
        {
            uint8_t *data = NULL;
            uint32_t dataLen = 0;
            
            ret = [bleFtp getFileWithBuffer:@"/internal_000/Rolling_Spider/thumb/Rolling_Spider_2014-01-21T160101+0100_3902B87F947BE865A9D137CFA63492B8.jpg" data:&data dataLen:&dataLen progressCallback:NULL progressArg:NULL];
        }*/
        
        /*if (ret == YES)
        {
            ret = [bleFtp renameFile:@"/a.txt" newNamePath:@"/b.txt"];
        }*/
        
        /*if (ret == YES)
        {
            ret = [bleFtp abortPutFile:@"/update/program.plf"];
            if (ret == YES)
            {
                ret = [bleFtp deleteFile:@"/update/program.plf"];
            }
        }*/
        
        /*if (ret == YES)
        {
            NSString *localFile = [NSString stringWithFormat:@"%@/test2.txt", doc];
            
            FILE* src = fopen([localFile UTF8String], "wb");
            //fwrite("0123456789", 1, 10, src);
            char block[132];
            memset(block, '0', sizeof(block));
            //for (int i=0; i<((500 * 10) + 2); i++)
            //for (int i=0; i<((500 * 152)); i++)
            for (int i=0; i<((500 * 6)); i++)
            //for (int i=0; i<((500 * 1)); i++)
            {
                fwrite(block, 1, sizeof(block), src);
            }
            //fwrite(block, 1, 52, src);
            fflush(src);
            fclose(src);
            
            ret = [bleFtp putFile:@"program.plf" localFile:localFile progressCallback:NULL progressArg:NULL resume:NO];
        }*/
        
        /*if (ret == YES)
        {
            ret = [bleFtp deleteFile:@"/update/test2.txt"];
        }*/
        
        ret = [bleFtp unregisterCharacteristics];
    }
    
    [SINGLETON_FOR_CLASS(ARSAL_BLEManager) disconnectPeripheral:peripheral withCentralManager:centralManager];
    
    [[ARDiscovery sharedInstance] stop];
    
    ARSAL_Sem_Destroy(&cancelSem);
}

@end
