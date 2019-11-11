/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3asy.h"
#include "genericApp.h"
#include "defaultCallbacks.h"
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
//   |    10.1.1.1    |    |    10.1.1.2    |    |    10.1.1.3    |
//   +----------------+    +----------------+    +----------------+
//   | simple channel |    | simple channel |    | simple channel |
//   +----------------+    +----------------+    +----------------+
//           |                     |                     |
//           +---------------------+---------------------+
//
// ===========================================================================

void (*a_onReceiveFtn)(const char[], unsigned int) = &PacketReceived;
void (*a_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketRead;
void (*a_onAcceptFtn)(const char[], unsigned int, const char[], unsigned int) = &ConnectionAccepted;
void (*a_onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketSent;

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

	unsigned int nodesCount = 3;

	NodeContainer nodes;
	nodes.Create(nodesCount);

	SimpleNetDeviceHelper sndh;

	NetDeviceContainer devices;
	devices = sndh.Install(nodes);

//	//It is possible to set an error model for a certain device which determines
//	//how its reception of data from the network is affected by error
//	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
//	em->SetAttribute("ErrorRate", DoubleValue(0.00001));
//	devices.Get(2)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	InternetStackHelper stack;
	stack.Install(nodes);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces = address.Assign(devices);

	vector<Ptr<GenericApp>> apps;

	for (unsigned int i = 0; i < nodesCount; i++) {
		Ptr<Socket> serverSocket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());
		Ptr<Socket> sendSocket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());
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

	unsigned int recipient[nodesCount] = {1, 2, 0};

	for (unsigned int i = 0; i < nodesCount; i++) {
		Simulator::Schedule(Seconds(2.0 + 1e-9), &GenericApp::ConnectToPeerAndSendPackets, apps.at(i),
				InetSocketAddress(recipient[i] == i ? Ipv4Address::GetLoopback() :
				interfaces.GetAddress(recipient[i]), 8080), 1040, 100, DataRate("1Mbps"));
	}

	Simulator::Stop(Seconds(20));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}

