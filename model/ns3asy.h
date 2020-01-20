/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_H
#define NS3ASY_H

#include "topology.h"

extern "C" void SetNodesCount(unsigned int nodesCount);

extern "C" void AddLink(unsigned int sourceIndex, unsigned int destinationIndex);

extern "C" void setUdp(bool isUdp);

extern "C" bool isUdp();

extern "C" int FinalizeSimulationSetup();

extern "C" int FinalizeWithWifiPhy();

extern "C" void SchedulePacketsSending(unsigned int senderIndex, unsigned int nPackets, const char* payload, int length);

extern "C" void ResumeSimulation(double delay);

extern "C" void StopSimulation();

extern "C" int getNodesCount();

extern "C" int getReceiversN(unsigned int sender);

extern "C" int getReceiverAt(unsigned int sender, unsigned int receiverIndex);

extern "C" char* getIpAddressFromIndex(unsigned int index);

extern "C" int getIndexFromIpAddress(const char* ip);

extern "C" void SetOnReceiveFtn(void (*ftn)(const char[], unsigned int, double));

extern "C" void SetOnPacketReadFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double));

extern "C" void SetOnAcceptFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, double));

extern "C" void SetOnSendFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double));


#endif
