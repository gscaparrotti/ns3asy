/*
 * ns3asyConfig.cpp
 *
 *  Created on: 11 feb 2020
 *      Author: giacomo
 */

#include "ns3asyConfig.h"

ns3asyConfig::ns3asyConfig(bool isUdp, int packetLength, const char* dataRate) {
	m_transportProtocol = isUdp ? UdpSocketFactory::GetTypeId() : TcpSocketFactory::GetTypeId();
	m_packetLength = packetLength;
	m_dataRate = DataRate(strcmp(dataRate, "Default") == 0 ? "0Mbps" : dataRate);
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
