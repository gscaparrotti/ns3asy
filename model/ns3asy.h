/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_H
#define NS3ASY_H

#include "topology.h"

extern "C" void SetNodesCount(unsigned int nodesCount);

extern "C" void AddLink(unsigned int sourceIndex, unsigned int destinationIndex);

extern "C" int FinalizeSimulationSetup();

extern "C" void SchedulePacketsSending(unsigned int senderIndex, unsigned int nPackets);

extern "C" void ResumeSimulation(double delay);

extern "C" void StopSimulation();

extern "C" void SetOnReceiveFtn(void (*ftn)(const char[], unsigned int));

extern "C" void SetOnPacketReadFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));

extern "C" void SetOnAcceptFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int));

extern "C" void SetOnSendFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));


#endif
