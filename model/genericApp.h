/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_GENERIC_APP
#define NS3ASY_GENERIC_APP

#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

struct ConnectionInfoData {
	char senderIp[16];
	char receiverIp[16];
	unsigned int senderPort;
	unsigned int receiverPort;
};

class ConnectionInfo: public Object {
public:
	ConnectionInfo();
	//we use the default destructor
	void SetSenderAddress(Address sender);
	void SetReceiverAddress(Address receiver);
	ConnectionInfoData Get();
	static Address FromSocket(Ptr<Socket> socket);
private:
	const char* IpAsStringFromAddress(Address address);
	unsigned int portFromAddress(Address address);
	ConnectionInfoData cid;
};

class GenericApp: public Application {
public:
	GenericApp();
	//this is the destructor. It's declared as virtual so that subclasses can specify their own destructor
	//and have it actually called at runtime
	virtual ~GenericApp();
	void Setup(Ptr<Socket> socket);
	void ConnectToPeerAndSendPackets(Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
	void SetOnReceiveFunction(void (*onReceiveFtn)(const char[], unsigned int));
	void SetOnPacketReadFunction(void (*m_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));
	void SetOnAcceptFunction(void (*onReceiveFtn)(const char[], unsigned int, const char[], unsigned int));
	void SetOnSendFunction(void (*m_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int));

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ScheduleTx(void);
	void SendPacket(void);
	void OnReceive(Ptr<Socket> socket);
	void OnAccept(Ptr<Socket> socket, const Address &from);

	Ptr<Socket> m_socket;
	Address m_peer;
	uint32_t m_packetSize;
	uint32_t m_nPackets;
	DataRate m_dataRate;
	EventId m_sendEvent;
	bool m_running;
	uint32_t m_packetsSent;
	//first two parameters: who calls the function; second two parameters: who the function refers to; last parameters: other data
	void (*m_onReceiveFtn)(const char[], unsigned int);
	void (*m_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int);
	void (*m_onAcceptFtn)(const char[], unsigned int, const char[], unsigned int);
	void (*m_onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int);
};

#endif
