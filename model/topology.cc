/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "topology.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3asy-topology");

Topology::Topology(unsigned int nodesCount) {
	for (unsigned int i = 0; i < nodesCount; i++) {
		vector<unsigned int> v;
		Topology::topology.push_back(v);
	}
}

void Topology::AddReceiver(unsigned int senderIndex, unsigned int receiverIndex) {
	Topology::topology.at(senderIndex).push_back(receiverIndex);
}

vector<unsigned int> Topology::GetReceivers(unsigned int senderIndex) {
	return Topology::topology.at(senderIndex);
}

unsigned int Topology::GetNodesCount() {
	return Topology::topology.size();
}
