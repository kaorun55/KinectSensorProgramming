/*
 *  MyMultipleHands.h
 *  MultipleHands
 *
 *  Created by 中村 薫 on 11/03/14.
 *  Copyright 2011 personal. All rights reserved.
 *
 */

#ifndef MYMULTIPLEHANDS_H_INCLUDE
#define MYMULTIPLEHANDS_H_INCLUDE

#include "XnVPointControl.h"

class MyMultipleHands : public XnVPointControl
{
public:
    
    MyMultipleHands();
    
    void Update(const XnVMultipleHands& hands);
    void OnPointCreate(const XnVHandPointContext *pContext);
    void OnPointUpdate(const XnVHandPointContext *pContext);
    void OnPointDestroy(XnUInt32 nID);
    void OnPrimaryPointCreate(const XnVHandPointContext *pContext, const XnPoint3D &ptSessionStarter);
    void OnPrimaryPointUpdate(const XnVHandPointContext *pContext);
    void OnPrimaryPointReplace(XnUInt32 nOldId, const XnVHandPointContext *pContext);
    void OnPrimaryPointDestroy(XnUInt32 nID);
    void OnNoPoints();
};

#endif // #ifndef MYMULTIPLEHANDS_H_INCLUDE
