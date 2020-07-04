// create two separate LANs, connect the two with P2P connection, then have one device on LAN1 ping a device on LAN2
//   (LAN1)         (P2P)        (LAN2)
// n1----n2----n3========n1-----n2-----n3
// |                                   |
// echo                              echo
// server                           client
// 
// desgin strategy:
// start by creating nodes for each LAN 
// from each LAN take one node and add them to a new node container, these will be the P2P connection
// create device containers for LANs and P2P networks and add their nodes to them
// install internet stack on all nodes, remember that our P2P nodes already exist as nodes in the LANs so we dont have to install them twice
// each LAN will have its own IP range, and the P2P network will also have its own IP
// 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P2P");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // creating first LAN nodes
  NodeContainer nodes;
  nodes.Create (3);

  // creating second LAN nodes
  NodeContainer nodes2;
  nodes2.Create (3);

  // adding nodes from LAN1 and LAN2 to the P2P channel
  NodeContainer p2pNodes;
  p2pNodes.Add (nodes.Get (2));
  p2pNodes.Add (nodes2.Get (0));

  // specs of both LANs
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  // specs of P2P channel
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // adding lAN1 nodes to device container
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (nodes);

  // adding lAN2 nodes to device container
  NetDeviceContainer csmaDevices2;
  csmaDevices2 = csma.Install (nodes2);

  // adding p2p nodes to device container
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  // install internet protocols on our nodes
  InternetStackHelper stack;
  stack.Install (nodes);
  stack.Install (nodes2);

  Ipv4AddressHelper address;
  // first LAN's IP
  address.SetBase ("10.1.1.0", "255.255.255.0");
  // assign above IP to devices
  Ipv4InterfaceContainer LAN1_interfaces;
  LAN1_interfaces = address.Assign (csmaDevices);

  // second LAN's IP
  address.SetBase ("10.1.2.0", "255.255.255.0");
  // assign above IP to devices
  Ipv4InterfaceContainer LAN2_interfaces;
  LAN2_interfaces = address.Assign (csmaDevices2);

  // p2p ip
  address.SetBase ("192.168.200.0", "255.255.255.0");
  // assign above IP to devices
  Ipv4InterfaceContainer p2p_interface;
  p2p_interface = address.Assign (p2pDevices);

// pick a certain port number for the udp server
  UdpEchoServerHelper echoServer (9);

// install udp server in LAN1 node(0)
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (10.0));

// configure udp client with ip and port number of server
  UdpEchoClientHelper echoClient (LAN1_interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

// install udp application on lan2 node(2)
  ApplicationContainer clientApps = echoClient.Install (nodes2.Get (2));
  clientApps.Start (Seconds (1));
  clientApps.Stop (Seconds (10.0));

  // this command will do the routing for us
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
