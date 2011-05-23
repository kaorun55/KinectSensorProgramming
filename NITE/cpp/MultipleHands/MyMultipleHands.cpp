/*
 *  MyMultipleHands
 *  MultipleHands
 *
 *  Created by 中村 薫 on 11/03/14.
 *  Copyright 2011 personal. All rights reserved.
 *
 */

#include <iostream>
#include "MyMultipleHands.h"

MyMultipleHands::MyMultipleHands()
    : XnVPointControl("MyMultipleHands")
{
}

void MyMultipleHands::Update(const XnVMultipleHands& hands)
{
//    std::cout << __FUNCTION__ << "," << 
//                hands.NewEntries() << "," << 
//                hands.OldEntries() << "," <<  
//                hands.ActiveEntries() <<  ",";
//    std::cout << std::endl;
//    
//    for (XnVMultipleHands::ConstIterator it = hands.begin();
//         it != hands.end(); ++it ) {
//        std::cout << "ID:" << (*it)->nID << "," <<
//                     "UserID:" << (*it)->nUserID << ",";
//        std::cout << std::endl;
//    }

    
    // アップデートを呼び出した手が、以降の呼び出し対象になる
    XnVPointControl::Update(hands);
}

void MyMultipleHands::OnPointCreate(const XnVHandPointContext *pContext)
{
    std::cout << __FUNCTION__ << std::endl;

    std::cout << "ID:" << pContext->nID << "," <<
                 "UserID:" << pContext->nUserID << ",";
    std::cout << std::endl;
}

void MyMultipleHands::OnPointUpdate(const XnVHandPointContext *pContext)
{
//    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnPointDestroy(XnUInt32 nID)
{
    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnPrimaryPointCreate(const XnVHandPointContext *pContext, const XnPoint3D &ptSessionStarter)
{
    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnPrimaryPointUpdate(const XnVHandPointContext *pContext)
{
//    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnPrimaryPointReplace(XnUInt32 nOldId, const XnVHandPointContext *pContext)
{
    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnPrimaryPointDestroy(XnUInt32 nID)
{
    std::cout << __FUNCTION__ << std::endl;
}

void MyMultipleHands::OnNoPoints()
{
    std::cout << __FUNCTION__ << std::endl;
}

