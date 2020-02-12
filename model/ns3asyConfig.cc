/*
 * ns3asyConfig.cpp
 *
 *  Created on: 11 feb 2020
 *      Author: giacomo
 */

#include "ns3asyConfig.h"

ns3asyConfig::ns3asyConfig(bool isUdp, int packetLength, const char* dataRate) {
	m_transportProtocol = isUdp ? UdpSocketFactory::GetTypeId() : TcpSocketFactory::GetTypeId();
	m_packetLength = packetLength != 0 ? packetLength : 1040;
	m_dataRate = DataRate(dataRate);
}

TypeId ns3asyConfig::getTransportProtocol() {
	return m_transportProtocol;
}

int ns3asyConfig::getPacketLength() {
	return m_packetLength;
}

DataRate ns3asyConfig::getDataRate() {
	return m_dataRate;
}
