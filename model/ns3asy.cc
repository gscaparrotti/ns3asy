/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3asy.h"
#include "genericApp.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include <unistd.h>
#include <iostream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3asy");

// ===========================================================================
//
//         node 0                node 1                node 2
//   +----------------+    +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |    |    10.1.1.2    |
//   +----------------+    +----------------+    +----------------+
//   |      csma      |    |      csma      |    |      csma      |
//   +----------------+    +----------------+    +----------------+
//           |                     |                     |
//           +---------------------+---------------------+
//                            5 Mbps, 2 ms
// ===========================================================================


static void ConnectionAccepted(const char receiverIp[], unsigned int receiverPort, const char senderIp[], unsigned int senderPort) {
	NS_LOG_DEBUG("A connection has been accepted by the socket " << receiverIp << ":"
			<< receiverPort << "; it was requested by " << senderIp << ":" << senderPort);
}

static void PacketReceived(const char ip[], unsigned int port) {
	NS_LOG_DEBUG("A packet has been received by the socket with ip=" << ip << " and port=" << port);
}

static void PacketRead(const char receiverIp[], unsigned int receiverPort, const char senderIp[], unsigned int senderPort, const unsigned char payload[], unsigned int payloadLength) {
	std::ostringstream outputDebug;
	std::ostringstream outputInfo;
	outputDebug << "A packet has been read by the socket " << receiverIp << ":"
			<< receiverPort << "; it was sent by " << senderIp << ":" << senderPort;
	outputInfo << "The read content is: ";
	for (unsigned int i = 0; i < payloadLength; i++) {
		outputInfo << payload[i];
	}
	NS_LOG_DEBUG(outputDebug.str().c_str());
	NS_LOG_INFO(outputInfo.str().c_str());
}

static void PacketSent(const char senderIp[], unsigned int senderPort, const char receiverIp[], unsigned int receiverPort, const unsigned char payload[], unsigned int payloadLength) {
	std::ostringstream outputDebug;
	std::ostringstream outputInfo;
	outputDebug << "A packet has been sent by the socket " << senderIp << ":"
			<< senderPort << "; it was sent to " << receiverIp << ":" << receiverPort;
	outputInfo << "The sent content is: ";
	for (unsigned int i = 0; i < payloadLength; i++) {
		outputInfo << payload[i];
	}
	NS_LOG_DEBUG(outputDebug.str().c_str());
	NS_LOG_INFO(outputInfo.str().c_str());
}

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
	NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}

static void RxDrop(Ptr<const Packet> p) {
	NS_LOG_UNCOND("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void (*congestionWindowCallback)(unsigned int, unsigned int) = &CwndChange;
void (*a_onReceiveFtn)(const char[], unsigned int) = &PacketReceived;
void (*a_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketRead;
void (*a_onAcceptFtn)(const char[], unsigned int, const char[], unsigned int) = &ConnectionAccepted;
void (*a_onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketSent;

void SetTcpCongestionWindowCallback(void (*callback)(unsigned int, unsigned int)) {
	congestionWindowCallback = callback;
}

void SetOnReceiveFtn(void (*ftn)(const char[], unsigned int)) {
	a_onReceiveFtn = ftn;
}

void SetOnPacketReadFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int)) {
	a_onPacketReadFtn = ftn;
}

void SetOnAcceptFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int)) {
	a_onAcceptFtn = ftn;
}

void SetOnSendFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int)) {
	a_onSendFtn = ftn;
}

int RunSimulation() {
	Ptr<DefaultSimulatorImpl> s = CreateObject<DefaultSimulatorImpl>();
	Simulator::SetImplementation(s);

	int nodesCount = 3;

	//Vengono creati due nodi. Uno (il nodo 0) avrà come Application MyApp, l'altro avrà
	//invece PacketSinkApplication (il nodo 1), che è un' Application che riceve pacchetti e
	//genera eventi senza trasmettere dati
	NodeContainer nodes;
	nodes.Create(nodesCount);

	SimpleNetDeviceHelper sndh;

	NetDeviceContainer devices;
	devices = sndh.Install(nodes);

//	//Il canale di comunicazione non è perfetto, ma può generare degli errori
//	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
//	em->SetAttribute("ErrorRate", DoubleValue(0.00001));
//	devices.Get(2)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	InternetStackHelper stack;
	stack.Install(nodes);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces = address.Assign(devices);

	//CongestionWindow e PhyRxDrop (sotto) sono due trace sources, ossia sorgenti di informazioni
	//che producono dati al verificarsi di certi eventi.
	//TraceConnectWithoutContext serve ad agganciare una callback ad una certa trace source.
	//CongestionWindow è una trace source di TcpSocketBase, perciò TraceConnectWithoutContext
	//va chiamato sulla socket, mentre PhyRxDrop è una trace source di SimpleNetDevice,
	//perciò TraceConnectWithoutContext va chiamato sul device.
	//Il metodo TraceConnectWithoutContext è dichiarato in ObjectBase, perciò tutti le classi
	//che ereditano da essa ce l'hanno.

	//viene creata la socket per MyApp e gli si aggancia la callback per l'evento CongestionWindow

	vector<Ptr<GenericApp>> apps;

	for (unsigned int i = 0; i < nodes.GetN(); i++) {
		Ptr<Socket> serverSocket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());
		Ptr<Socket> sendSocket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());
		//socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(congestionWindowCallback));
		Ptr<GenericApp> app = CreateObject<GenericApp>();
		app->SetOnReceiveFunction(a_onReceiveFtn);
		app->SetOnPacketReadFunction(a_onPacketReadFtn);
		app->SetOnAcceptFunction(a_onAcceptFtn);
		app->SetOnSendFunction(a_onSendFtn);
		app->Setup(serverSocket, sendSocket);
		nodes.Get(1)->AddApplication(app);
		app->SetStartTime(Seconds(1.));
		app->SetStopTime(Seconds(50.));
		apps.push_back(app);
	}

	int recipient[nodesCount] = {1, 2, 0};

	//the last node does not send any packet, but is the one who receives them from all the other nodes
	for (unsigned int i = 0; i < apps.size(); i++) {
		Simulator::Schedule(Seconds(2.0 + 1e-9), &GenericApp::ConnectToPeerAndSendPackets, apps.at(i),
				InetSocketAddress(interfaces.GetAddress(recipient[i]), 8080), 1040, 100, DataRate("1Mbps"));
	}

	//viene agganciata la callback per PhyRxDrop alla PacketSinkApplication
	devices.Get(2)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));

	Simulator::Stop(Seconds(20));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}

