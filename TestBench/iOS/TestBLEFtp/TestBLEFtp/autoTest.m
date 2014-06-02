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


#import "autoTest.h"



@implementation autoTest

- (void)testConnection
{
    //[self performSelectorInBackground:@selector(testScann) withObject:nil];
    
    NSThread *thread = [[NSThread alloc] initWithTarget:self selector:@selector(testScann) object:nil];
    [thread start];
}

- (void)testScann
{
    ARBLEService *service = nil;
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //const char* doc = [[paths lastObject] cString];
    NSString *doc = [paths lastObject];
    
    [[ARDiscovery sharedInstance] start];
    
    while (service == nil)
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
                NSString *NAME = @"Mykonos_DF";
                if ([serviceIdx.peripheral.name isEqualToString:NAME])
                {
                    NSLog(@"%@", serviceIdx.peripheral);
                    service = serviceIdx;
                    break;
                }
            }
        }
    }
    
    //[[ARDiscovery sharedInstance] stop];
    
    eARSAL_ERROR result = ARSAL_OK;
    ARSAL_CentralManager *centralManager = service.centralManager;
    CBPeripheral *peripheral = service.peripheral;
    BOOL ret = NO;
    
    //ARUtils_BLEFtp *bleFtp = [[ARUtils_BLEFtp alloc] init];
    
    [NSThread sleepForTimeInterval:1];
    
    result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) connectToPeripheral:service.peripheral withCentralManager:centralManager];
    
    if  (result == ARSAL_OK)
    {
        result = [SINGLETON_FOR_CLASS(ARSAL_BLEManager) discoverNetworkServices:nil];
    }

    if  (result == ARSAL_OK)
    {
        //[bleFtp initWithManager:(__bridge ARNETWORKAL_BLEDeviceManager_t)centralManager device:(__bridge ARNETWORKAL_BLEDevice_t)peripheral];
        //[bleFtp initWithManager:SINGLETON_FOR_CLASS(ARSAL_BLEManager) centralManager:centralManager peripheral:peripheral delegate:nil obj:self];
        ARUtils_BLEFtp *bleFtp = [[ARUtils_BLEFtp alloc] initWithPeripheral:peripheral cancelSem:NULL port:0];
        
        ret = [bleFtp registerCharacteristics];
        
        /*if (ret == YES)
        {
            NSMutableString *list = [[NSMutableString alloc] init];
            ret = [bleFtp listFiles:@"/update/" list:list];
            
            NSLog(@"LIST: %@", list);
        }*/
        
        if (ret == YES)
        {
            NSString *localFile = [NSString stringWithFormat:@"%@/test.jpg", doc];
            
            //ret = [bleFtp getFile:@"/update/test.jpg" localFile:localFile];
            //ret = [bleFtp getFile:@"/update/test.txt" localFile:localFile];
            //ret = [bleFtp getFile:@"/update/a.txt" localFile:localFile];
            ret = [bleFtp getFile:@"/update/program.plf" localFile:localFile progressCallback:NULL progressArg:NULL];
        }
        
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
            for (int i=0; i<((500 * 1)); i++)
            {
                fwrite(block, 1, sizeof(block), src);
            }
            //fwrite(block, 1, 52, src);
            fflush(src);
            fclose(src);
            
            ret = [bleFtp putFile:@"/update/program.plf" localFile:localFile];
        }*/
        
        /*if (ret == YES)
        {
            ret = [bleFtp deleteFile:@"/update/test2.txt"];
        }*/
    }
    
    
    result = result;
    
    [SINGLETON_FOR_CLASS(ARSAL_BLEManager) disconnectPeripheral:peripheral withCentralManager:centralManager];
    
    [[ARDiscovery sharedInstance] stop];
}

@end
