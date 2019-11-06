/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_H
#define NS3ASY_H


extern "C" int RunSimulation();

extern "C" void SetTcpCongestionWindowCallback(void (*callback)(unsigned int, unsigned int));

extern "C" void SetOnReceiveFtn(void (*ftn)(const char[], unsigned int));

extern "C" void SetOnPacketReadFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));

extern "C" void SetOnAcceptFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int));

extern "C" void SetOnSendFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));


#endif
