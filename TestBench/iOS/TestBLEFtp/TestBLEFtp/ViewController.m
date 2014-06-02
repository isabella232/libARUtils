//
//  ViewController.m
//  TestBLEFtp
//
//  Created by David on 30/05/2014.
//  Copyright (c) 2014 ___PARROT___. All rights reserved.
//

#import "ViewController.h"
#import "autoTest.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    
    autoTest *test = [[autoTest alloc] init];
    [test testConnection];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
