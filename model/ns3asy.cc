/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3asy.h"
#include "genericApp.h"
#include "defaultCallbacks.h"
#include "topology.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/string.h"
#include "ns3/config.h"
#include "ns3/flow-monitor-module.h"
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

static int scheduledEventsCount = 0;
static vector<Ptr<GenericApp>> apps;
static Ptr<Topology> topology = CreateObject<Topology>(0);
static Ipv4InterfaceContainer interfaces;

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

static void SetSockets(unsigned int nodesCount, NodeContainer nodes) {
	for (unsigned int i = 0; i < nodesCount; i++) {
		Ptr<Socket> serverSocket = Socket::CreateSocket(nodes.Get(i), UdpSocketFactory::GetTypeId());
		vector<Ptr<Socket>> sendSockets;
		for (unsigned int k = 0; k < topology->GetReceivers(i).size(); k++) {
			sendSockets.push_back(Socket::CreateSocket(nodes.Get(i), UdpSocketFactory::GetTypeId()));
		}
		Ptr<GenericApp> app = CreateObject<GenericApp>();
		app->SetOnReceiveFunction(a_onReceiveFtn);
		app->SetOnPacketReadFunction(a_onPacketReadFtn);
		app->SetOnAcceptFunction(a_onAcceptFtn);
		app->SetOnSendFunction(a_onSendFtn);
		app->Setup(serverSocket, sendSockets, interfaces.GetAddress(i));
		nodes.Get(i)->AddApplication(app);
		app->SetStartTime(Seconds(0.));
		apps.push_back(app);
	}

	for (unsigned int i = 0; i < nodesCount; i++) {
		vector<unsigned int> receiversForNode = topology->GetReceivers(i);
		for(unsigned int k = 0; k < receiversForNode.size(); k++) {
			unsigned int kthReceiver = receiversForNode.at(k);
			Simulator::Schedule(Seconds(1e-9), &GenericApp::ConnectToPeer, apps.at(i),
					InetSocketAddress(kthReceiver == i ? Ipv4Address::GetLoopback() :
					interfaces.GetAddress(kthReceiver), 8080), k);
		}
	}
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
	interfaces = address.Assign(devices);

	SetSockets(nodesCount, nodes);

	return 0;
}

int FinalizeWithWifiPhy() {
	//PHY == physical layer

	std::string phyRate = "HtMcs7";
	WifiMacHelper wifiMac;
	WifiHelper wifiHelper;
	wifiHelper.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);

	/* Set up Legacy Channel */
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

	/* Setup Physical Layer */
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());
	wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
	wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager",
			"DataMode", StringValue(phyRate), "ControlMode", StringValue("HtMcs0"));

	unsigned int nodesCount = topology->GetNodesCount();

	NodeContainer networkNodes;
	//ad one node for the ap
	networkNodes.Create(nodesCount + 1);
	Ptr<Node> apWifiNode = networkNodes.Get(nodesCount);

	/* Configure AP */
	Ssid ssid = Ssid("network");
	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

	NetDeviceContainer apDevice;
	apDevice = wifiHelper.Install(wifiPhy, wifiMac, apWifiNode);

	/* Configure STA */
	/* a station (abbreviated as STA) is a device that has the capability to use the 802.11 protocol */
	wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

	NetDeviceContainer staDevices;
	staDevices = wifiHelper.Install(wifiPhy, wifiMac, networkNodes);

	/* Mobility model */
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
	positionAlloc->Add(Vector(0.0, 0.0, 0.0));
	positionAlloc->Add(Vector(174.0, 1.0, 0.0));

	mobility.SetPositionAllocator(positionAlloc);
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(apWifiNode);
	mobility.Install(networkNodes);

	/* Internet stack */
	InternetStackHelper stack;
	stack.Install(networkNodes);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer apInterface;
	apInterface = address.Assign(apDevice);
	interfaces = address.Assign(staDevices);

	/* Populate routing table */
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	SetSockets(nodesCount, networkNodes);

    wifiPhy.EnablePcap ("AccessPoint", apDevice);
    wifiPhy.EnablePcap ("Station", staDevices);

	return 0;
}

void SchedulePacketsSending(unsigned int senderIndex, unsigned int nPackets, const char* payload, int length) {
	Simulator::Schedule(MicroSeconds(++scheduledEventsCount), &GenericApp::SendPackets,
			apps.at(senderIndex), 1040, nPackets, DataRate("1Mbps"), payload, length);
}

void ResumeSimulation(double delay) {
	scheduledEventsCount = 0;
	if (delay >= 0) {
		Simulator::Stop(Seconds(Simulator::Now().GetSeconds() + delay));
	}
	Simulator::Run();
}

void StopSimulation() {
	Simulator::Stop();
	Simulator::Destroy();
	apps.clear();
}

//ALWAYS REMEMBER TO FREE THE RETURNED POINTER!!!
char* getIpAddressFromIndex(unsigned int index) {
	char* IpAsString = static_cast<char*>(malloc(16 * sizeof(char)));
	strcpy(IpAsString, "none");
	if (index >= 0 && index < interfaces.GetN()) {
		//a ip address can have at most 26 characters (including the trailing /0)
		std::ostringstream ipStream;
		interfaces.GetAddress(index).Print(ipStream);
		strcpy(IpAsString, ipStream.str().c_str());
	}
	return IpAsString;
}

int getIndexFromIpAddress(const char* ip) {
	for (unsigned int i = 0; i != interfaces.GetN(); i++) {
	    std::pair<Ptr<Ipv4>, uint32_t> pair = interfaces.Get(i);
	    std::ostringstream ipStream;
	    pair.first->GetAddress(1, 0).GetLocal().Print(ipStream);
	    if (strcmp(ip, ipStream.str().c_str()) == 0) {
	    	return i;
	    }
	}
	return -1;
}
