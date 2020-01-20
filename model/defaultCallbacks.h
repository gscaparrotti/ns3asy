/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_DEFAULT_CALLBACKS
#define NS3ASY_DEFAULT_CALLBACKS

void ConnectionAccepted(const char receiverIp[], unsigned int receiverPort, const char senderIp[],
		unsigned int senderPort, double time);

void PacketReceived(const char ip[], unsigned int port, double time);

void PacketRead(const char receiverIp[], unsigned int receiverPort, const char senderIp[],
		unsigned int senderPort, const unsigned char payload[], unsigned int payloadLength, double time);

void PacketSent(const char senderIp[], unsigned int senderPort, const char receiverIp[],
		unsigned int receiverPort, const unsigned char payload[], unsigned int payloadLength, double time);

#endif
