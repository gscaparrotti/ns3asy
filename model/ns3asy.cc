/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3asy.h"
#include "genericApp.h"
#include "defaultCallbacks.h"
#include "topology.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
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

static vector<Ptr<GenericApp>> apps;
static Ptr<Topology> topology = CreateObject<Topology>(0);

static void (*a_onReceiveFtn)(const char[], unsigned int) = &PacketReceived;
static void (*a_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketRead;
static void (*a_onAcceptFtn)(const char[], unsigned int, const char[], unsigned int) = &ConnectionAccepted;
static void (*a_onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int) = &PacketSent;

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

void SetNodesCount(unsigned int nodesCount) {
	topology = CreateObject<Topology>(nodesCount);
}

void AddLink(unsigned int sourceIndex, unsigned int destinationIndex) {
	topology->AddReceiver(sourceIndex, destinationIndex);
}

int FinalizeSimulationSetup() {

	Ptr<DefaultSimulatorImpl> s = CreateObject<DefaultSimulatorImpl>();
	Simulator::SetImplementation(s);

	unsigned int nodesCount = topology->GetNodesCount();

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

	for (unsigned int i = 0; i < nodesCount; i++) {
		Ptr<Socket> serverSocket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());
		vector<Ptr<Socket>> sendSockets;
		for (unsigned int k = 0; k < topology->GetReceivers(i).size(); k++) {
			sendSockets.push_back(Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId()));
		}
		Ptr<GenericApp> app = CreateObject<GenericApp>();
		app->SetOnReceiveFunction(a_onReceiveFtn);
		app->SetOnPacketReadFunction(a_onPacketReadFtn);
		app->SetOnAcceptFunction(a_onAcceptFtn);
		app->SetOnSendFunction(a_onSendFtn);
		app->Setup(serverSocket, sendSockets);
		nodes.Get(i)->AddApplication(app);
		app->SetStartTime(Simulator::Now());
		//app->SetStopTime(Seconds(50.));
		apps.push_back(app);
	}

	for (unsigned int i = 0; i < nodesCount; i++) {
		vector<unsigned int> receiversForNode = topology->GetReceivers(i);
		for(unsigned int k = 0; k < receiversForNode.size(); k++) {
			unsigned int kthReceiver = receiversForNode.at(k);
			Simulator::Schedule(Seconds(Simulator::Now().GetSeconds() + 1e-9), &GenericApp::ConnectToPeer, apps.at(i),
					InetSocketAddress(kthReceiver == i ? Ipv4Address::GetLoopback() :
					interfaces.GetAddress(kthReceiver), 8080));
		}
	}

	return 0;
}

void SchedulePacketsSending(unsigned int senderIndex, unsigned int nPackets, const char* payload, int length) {
	Simulator::Schedule(Seconds(Simulator::Now().GetSeconds() + 1e-9), &GenericApp::SendPackets,
			apps.at(senderIndex), 1040, nPackets, DataRate("1Mbps"), payload, length);
}

void ResumeSimulation(double delay) {
	if (delay >= 0) {
		Simulator::Stop(Seconds(Simulator::Now().GetSeconds() + delay));
	}
	Simulator::Run();
}

void StopSimulation() {
	Simulator::Stop();
	Simulator::Destroy();
}

