////////////////////////////////////////////////////////////////////////////
//
//  Venus Engine Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   _VeViewController.h
//  Version:     v1.00
//  Created:     27/11/2012 by Napoleon
//  Compilers:   Xcode Obj-c
//  Description:
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#import <UIKit/UIKit.h>

@interface VeViewController : UIViewController

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic, assign) CADisplayLink* displayLink;

- (void)startAnimation;
- (void)stopAnimation;

@end