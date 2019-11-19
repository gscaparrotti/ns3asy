/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NS3ASY_TOPOLOGY
#define NS3ASY_TOPOLOGY

#include "ns3/core-module.h"

using namespace ns3;
using namespace std;

class Topology: public Object {
public:
	Topology(unsigned int nodesCount);
	//we use the default destructor
	void AddReceiver(unsigned int senderIndex, unsigned int receiverIndex);
	vector<unsigned int> GetReceivers(unsigned int senderIndex);
	unsigned int GetNodesCount();
private:
	vector<vector<unsigned int>> topology;
};

#endif
