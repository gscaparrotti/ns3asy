/*
 * ns3asyConfig.h
 *
 *  Created on: 11 feb 2020
 *      Author: giacomo
 */

#ifndef SRC_NS3ASY_MODEL_NS3ASYCONFIG_H_
#define SRC_NS3ASY_MODEL_NS3ASYCONFIG_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

class ns3asyConfig: public Object {
public:
	ns3asyConfig(bool isUdp, int packetLength, const char* dataRate);
	TypeId getTransportProtocol();
	int getPacketLength();
	DataRate getDataRate();
private:
	TypeId m_transportProtocol;
	int m_packetLength;
	DataRate m_dataRate;
};

#endif /* SRC_NS3ASY_MODEL_NS3ASYCONFIG_H_ */
