#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/nr-module.h"
#include "ns3/udp-client-server-helper.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // Log de informações
    LogComponentEnable("AnimationInterface", LOG_LEVEL_INFO);

    // Número de veículos
    uint32_t numVehicles = 10;

    // Frequência central e largura de banda para NR
    double frequency = 28e9;      // 28 GHz (mmWave)
    double bandwidth = 100e6;     // 100 MHz de largura de banda
    BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::UMa;

    // Criação dos nós representando veículos
    NodeContainer vehicles;
    vehicles.Create(numVehicles);

    // Configuração do modelo de mobilidade aleatória para os veículos
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(0, 500, 0, 500)),
                              "Speed", StringValue("ns3::UniformRandomVariable[Min=10.0|Max=20.0]"));
    mobility.Install(vehicles);

    // Instalação da pilha de protocolos de Internet nos veículos
    InternetStackHelper internet;
    internet.Install(vehicles);

    // Configuração do NR (New Radio) para comunicação
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    CcBwpCreator::SimpleOperationBandConf bandConf(frequency,
                                                   bandwidth,
                                                   numCcPerBand,
                                                   scenario);

    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    nrHelper->InitializeOperationBand(&band);
    allBwps = CcBwpCreator::GetAllBwps({band});

    // Instalando dispositivos NR (New Radio) nos veículos
    NetDeviceContainer devices = nrHelper->InstallUeDevice(vehicles, allBwps);

    // Atribuindo endereços IP aos veículos
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Configuração de comunicação entre veículos usando UDP
    uint16_t port = 8080;
    UdpServerHelper udpServer(port);
    ApplicationContainer serverApps = udpServer.Install(vehicles.Get(0)); // Servidor no veículo 0
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(30.0));
    

    UdpClientHelper udpClient(interfaces.GetAddress(0), port); // Cliente enviando pacotes para o veículo 0
    udpClient.SetAttribute("MaxPackets", UintegerValue(100));
    udpClient.SetAttribute("Interval", TimeValue(Seconds(0.5))); // Envia pacotes a cada 0,5 segundos
    udpClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = udpClient.Install(vehicles.Get(1)); // Cliente no veículo 1
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(30.0));

    // Configuração do NetAnim para visualização da simulação
    AnimationInterface anim("vehicle-simulation.xml");

    // Executando a simulação
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
