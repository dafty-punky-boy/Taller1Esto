#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HierarchicalManet");

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);
    
    NodeContainer cluster1, cluster2, clusterHeads, secondLevelCluster;
    cluster1.Create(4); // 4 nodos en clúster 1
    cluster2.Create(4); // 4 nodos en clúster 2
    clusterHeads.Create(2); // 2 cluster heads para primer nivel
    secondLevelCluster.Create(1); // 1 cluster head de segundo nivel

    NodeContainer allNodes = NodeContainer(cluster1, cluster2, clusterHeads, secondLevelCluster);
    
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(10.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(5),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                              "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"));
    mobility.Install(allNodes);
    
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    
    WifiMacHelper mac;
    Ssid ssid = Ssid("MANET-Hierarchical");
    mac.SetType("ns3::AdhocWifiMac");
    
    NetDeviceContainer devices = wifi.Install(phy, mac, allNodes);
    
    InternetStackHelper stack;
    OlsrHelper olsr;
    stack.SetRoutingHelper(olsr);
    stack.Install(allNodes);
    
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    
    // Aplicaciones para prueba de conectividad
    uint16_t port = 5000;
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApp = echoServer.Install(secondLevelCluster.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(20.0));
    
    UdpEchoClientHelper echoClient(interfaces.GetAddress(8), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    
    ApplicationContainer clientApp = echoClient.Install(cluster1.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(20.0));
    
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();
    
    return 0;
}
