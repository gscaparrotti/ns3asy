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
#include "ns3/csma-module.h"
#include <unistd.h>
#include <iostream>
#include <vector>
#include "ns3asyConfig.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3asy");

static int scheduledEventsCount = 0;
static Ptr<ns3asyConfig> config;
static vector<Ptr<GenericApp>> apps;
static Ptr<Topology> topology = CreateObject<Topology>(0);
static Ipv4InterfaceContainer interfaces;

static void (*a_onReceiveFtn)(const char[], unsigned int, double) = &PacketReceived;
static void (*a_onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double) = &PacketRead;
static void (*a_onAcceptFtn)(const char[], unsigned int, const char[], unsigned int, double) = &ConnectionAccepted;
static void (*a_onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double) = &PacketSent;

void SetOnReceiveFtn(void (*ftn)(const char[], unsigned int, double)) {
	a_onReceiveFtn = ftn;
}

void SetOnPacketReadFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double)) {
	a_onPacketReadFtn = ftn;
}

void SetOnAcceptFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, double)) {
	a_onAcceptFtn = ftn;
}

void SetOnSendFtn(void (*ftn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int, double)) {
	a_onSendFtn = ftn;
}

void SetNodesCount(unsigned int nodesCount) {
	topology = CreateObject<Topology>(nodesCount);
}

void AddLink(unsigned int sourceIndex, unsigned int destinationIndex) {
	topology->AddReceiver(sourceIndex, destinationIndex);
}

bool isUdp() {
	return config->getTransportProtocol() == UdpSocketFactory::GetTypeId();
}

static void SetSockets(unsigned int nodesCount, NodeContainer nodes) {
	TypeId transportProtocol = config->getTransportProtocol();
	for (unsigned int i = 0; i < nodesCount; i++) {
		Ptr<Socket> serverSocket = Socket::CreateSocket(nodes.Get(i), transportProtocol);
		vector<Ptr<Socket>> sendSockets;
		for (unsigned int k = 0; k < topology->GetReceivers(i).size(); k++) {
			sendSockets.push_back(Socket::CreateSocket(nodes.Get(i), transportProtocol));
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

int FinalizeSimulationSetup(bool isUdp, int packetLength, double errorRate, const char* dataRate) {

	Ptr<DefaultSimulatorImpl> s = CreateObject<DefaultSimulatorImpl>();
	Simulator::SetImplementation(s);

	config = CreateObject<ns3asyConfig>(isUdp, packetLength, dataRate);

	unsigned int nodesCount = topology->GetNodesCount();

	NodeContainer nodes;
	nodes.Create(nodesCount);

	CsmaHelper csma;
	if (config->getDataRate().GetBitRate() != 0) {
		csma.SetChannelAttribute("DataRate", DataRateValue(config->getDataRate()));
	}

	NetDeviceContainer devices;
	devices = csma.Install(nodes);

	//It is possible to set an error model for a certain device which determines
	//how its reception of data from the network is affected by error
	if (errorRate > 0) {
		Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
		em->SetRate(errorRate);
		for(unsigned int i = 0; i < devices.GetN(); i++) {
			devices.Get(i)->SetAttribute("ReceiveErrorModel", PointerValue(em));
		}
	}

	InternetStackHelper stack;
	stack.Install(nodes);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	interfaces = address.Assign(devices);

	SetSockets(nodesCount, nodes);

	return 0;
}

int FinalizeWithWifiPhy(bool isUdp, int packetLength, double errorRate, const char* dataRate,
		const char* propagationDelay, const char* propagationLoss, double xPos[], double yPos[], double zPos[]) {
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
	Simulator::Schedule(MicroSeconds(++scheduledEventsCount * 100), &GenericApp::SendPackets,
			apps.at(senderIndex), config->getPacketLength(), nPackets, config->getDataRate(), payload, length);
}

void ResumeSimulation(double delay) {
	scheduledEventsCount = 0;
	if (delay >= 0) {
		Simulator::Stop(Seconds(delay));
	}
	Simulator::Run();
}

void StopSimulation() {
	for (unsigned int i = 0; i < apps.size(); i++) {
		apps.at(i)->StopApplication();
	}
	Simulator::Run();
	Simulator::Stop();
	Simulator::Destroy();
	apps.clear();
}

int getNodesCount() {
	return topology->GetNodesCount();
}

int getReceiversN(unsigned int sender) {
	return topology->GetReceivers(sender).size();
}

int getReceiverAt(unsigned int sender, unsigned int receiverIndex) {
	return topology->GetReceivers(sender).at(receiverIndex);
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
