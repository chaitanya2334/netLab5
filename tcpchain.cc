
#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/bridge-module.h"
#include <fstream>
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SixthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  //NodeContainer nodes;
  //nodes.Create (2);
  /* Build nodes. */
  NodeContainer term_0;
  term_0.Create (1);
  NodeContainer term_1;
  term_1.Create (1);
  NodeContainer term_2;
  term_2.Create (1);
  NodeContainer term_3;
  term_3.Create (1);

  PointToPointHelper pointToPoint_1, pointToPoint_2, pointToPoint_3;
  pointToPoint_1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint_1.SetChannelAttribute ("Delay", StringValue ("2ms"));

  pointToPoint_2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint_2.SetChannelAttribute ("Delay", StringValue ("2ms"));

  pointToPoint_3.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint_3.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //NetDeviceContainer devices;
  //devices = pointToPoint.Install (nodes);
  /* Build link net device container. */
  NodeContainer all_hub_3;
  all_hub_3.Add (term_0);
  all_hub_3.Add (term_1);
  NetDeviceContainer ndc_hub_3 = pointToPoint_1.Install (all_hub_3);
  NodeContainer all_hub_4;
  all_hub_4.Add (term_1);
  all_hub_4.Add (term_2);
  NetDeviceContainer ndc_hub_4 = pointToPoint_2.Install (all_hub_4);
  NodeContainer all_hub_5;
  all_hub_5.Add (term_2);
  all_hub_5.Add (term_3);
  NetDeviceContainer ndc_hub_5 = pointToPoint_3.Install (all_hub_5);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  ndc_hub_3.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  //InternetStackHelper stack;
  //stack.Install (nodes);

  InternetStackHelper internetStackH;
  internetStackH.Install (term_0);
  internetStackH.Install (term_1);
  internetStackH.Install (term_2);
  internetStackH.Install (term_3);

  //Ipv4AddressHelper address;
  //address.SetBase ("10.1.1.0", "255.255.255.252");
  //Ipv4InterfaceContainer interfaces = address.Assign (devices);
  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_3 = ipv4.Assign (ndc_hub_3);
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_4 = ipv4.Assign (ndc_hub_4);
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_5 = ipv4.Assign (ndc_hub_5);

   /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Generate Application. */
  uint16_t port_tcp_0 = 1090;
  Address sinkAddress (InetSocketAddress (iface_ndc_hub_3.GetAddress (1), port_tcp_0));
  Address sinkLocalAddress_tcp_0 (InetSocketAddress (Ipv4Address::GetAny (), port_tcp_0));
  PacketSinkHelper sinkHelper_tcp_0 ("ns3::TcpSocketFactory", sinkLocalAddress_tcp_0);
  ApplicationContainer sinkApp_tcp_0 = sinkHelper_tcp_0.Install (term_3);
  sinkApp_tcp_0.Start (Seconds (0.0));
  sinkApp_tcp_0.Stop (Seconds (20.0));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (term_3.Get(0), TcpSocketFactory::GetTypeId ()); //TODO find out which node to put here instead of term_0

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  term_3.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (0.));
  app->SetStopTime (Seconds (20.));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("sixth.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);
  ndc_hub_3.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  //OnOffHelper clientHelper_tcp_0 ("ns3::TcpSocketFactory", Address ());
  //clientHelper_tcp_0.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1)));
  //clientHelper_tcp_0.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
  //ApplicationContainer clientApps_tcp_0;
  //AddressValue remoteAddress_tcp_0 (InetSocketAddress (iface_ndc_hub_5.GetAddress (1), port_tcp_0));
  //clientHelper_tcp_0.SetAttribute ("Remote", remoteAddress_tcp_0);
  //clientApps_tcp_0.Add (clientHelper_tcp_0.Install (term_0));
  //clientApps_tcp_0.Start (Seconds (0.0));
  //clientApps_tcp_0.Stop (Seconds (20.0));

  //uint16_t sinkPort = 8080;
  //Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
  //PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  //ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  //sinkApps.Start (Seconds (0.));
  //sinkApps.Stop (Seconds (20.));

  //Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  //Ptr<MyApp> app = CreateObject<MyApp> ();
  //app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  //nodes.Get (0)->AddApplication (app);
  //app->SetStartTime (Seconds (1.));
  //app->SetStopTime (Seconds (20.));

  //AsciiTraceHelper asciiTraceHelper;
  //Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("sixth.cwnd");
  //ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  //PcapHelper pcapHelper;
  //Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);
  //devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  

  Simulator::Stop (Seconds (20));
  AnimationInterface anim ("animation.xml"); 
  anim.SetConstantPosition(term_0.Get(0), 1.0, 2.0);
  anim.SetConstantPosition(term_1.Get(0), 11.0, 2.0);
  anim.SetConstantPosition(term_2.Get(0), 21.0, 2.0);
  anim.SetConstantPosition(term_3.Get(0), 31.0, 2.0);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

