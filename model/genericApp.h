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


/**
 * \brief A class used to carry the fundamental info about a connection
 *
 * This class can be useful when there is the need to share the addresses and the ports
 * used by a connection without sharing the actual socket.
 * Everything that is inside this class is not a reference to the original socket,
 * but it is a mere copy which is then safe to share.
 */
class ConnectionInfo: public Object {
public:
	ConnectionInfo();
	//we use the default destructor
	void SetSenderAddress(Address sender);
	void SetReceiverAddress(Address receiver);
	ConnectionInfoData Get();
	static Address NameFromSocket(Ptr<Socket> socket);
	static Address PeerNameFromSocket(Ptr<Socket> socket);
	static string IpAsStringFromAddress(Address address);
	static unsigned int PortFromAddress(Address address);
private:
	ConnectionInfoData cid;
};

class GenericApp: public Application {
public:
	GenericApp();
	//this is the destructor. It's declared as virtual so that subclasses can specify their own destructor
	//and have it actually called at runtime
	virtual ~GenericApp();
	void Setup(Ptr<Socket> serverSocket, vector<Ptr<Socket>> sendSockets);
	void ConnectToPeer(Address address);
	void SendPackets(uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
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

	Ptr<Socket> m_serverSocket;
	vector<Ptr<Socket>> m_sendSockets;
	unsigned int m_nextSocket;
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
