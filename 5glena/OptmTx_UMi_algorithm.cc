
#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/nr-phy.h"
#include "ns3/nr-ue-phy.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/nr-gnb-net-device.h"
#include "ns3/log.h"
#include "ns3/trace-helper.h"
#include "ns3/ideal-beamforming-algorithm.h"
#include <cmath>
#include <iostream>     
#include <iomanip>      
#include <fstream>      
#include <sstream>      
#include "ns3/vector.h"  
#include "ns3/node.h"    
#include <algorithm> 
#include <vector>    
#include <limits>
#include <chrono>
#include <filesystem>
#include <unistd.h> 
#include <random>
#include "ns3/log.h"
#include <dlfcn.h>
#include <cstdlib>
#include <signal.h>     
#include <thread>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CttcNrDemo");

// Function to read positions from the CSV file
bool ReadPositionsFromCSV(const std::string &filename, std::vector<Vector> &positions)
{
    std::ifstream file(filename);
    std::string line;
        
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    std::getline(file, line);
   
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string nodeStr, xStr, yStr;        
        
        std::getline(ss, nodeStr, ',');  
        std::getline(ss, xStr, ',');
        std::getline(ss, yStr, ',');
        
        double x = std::stod(xStr);
        double y = std::stod(yStr);

        positions.push_back(Vector(x, y, 0.0)); // Add to positions vector
    }

    file.close();
    return true;
}
    
    std::ofstream openResultsFile(uint32_t seed) {

    uint32_t currentSeed = seed;  
    uint32_t lastSeed = currentSeed;
    std::string simTag = "";    
    std::string folderPath = "simulation_results/" + std::to_string(seed);
    //std::string folderPath = "simulation_results/seed" + std::to_string(seed);
        
    if (!std::filesystem::exists(folderPath)) {
        if (!std::filesystem::create_directories(folderPath)) {
            std::cerr << "Error: Could not create directory " << folderPath << std::endl;
            exit(1);
        }
    }
   
    std::string outputFilePath = folderPath + "/" + simTag + std::to_string(seed) + ".csv";
       
    std::ofstream outFile;
    if (seed != lastSeed) {
        lastSeed = seed;  
        outFile.open(outputFilePath, std::ios::out);  
    } else {
        outFile.open(outputFilePath, std::ios::app);  
    }
    
    if (!outFile.is_open()) {
        std::cerr << "Can't open file " << outputFilePath << std::endl;
        exit(1); 
    }
    
    outFile.setf(std::ios_base::fixed, std::ios_base::floatfield);
    outFile.precision(6);

    // Write CSV header if creating a new file
    if (outFile.tellp() == 0) { 
        outFile << "ueid,txpackets,rxpackets,packetLoss,deliveryratio,throughput,latency,meanjitter,txenergy,distance,txpower\n";
    }

    return outFile;
}

double calculateSignalPower(double txPower, double distance, double frequency, double hBs, double hUt) {    
    if (distance < 1.0) {
        distance = 1.0;
    }    
    double losPathLoss = 32.4 + 21 * std::log10(distance/1) + 20 * std::log10(frequency/1e9) + 4.0;     
    double nlosPathLoss = 22.4 + 35.3 * std::log10(distance/1) + 21.3 * std::log10(frequency/1e9) - 0.3 * (hUt - 1.5) + 7.82;
    double pathLoss = std::max(losPathLoss, nlosPathLoss);

    double signalPower = txPower - pathLoss;
    
    //std::cout << "UEs Sig: " << signalPower << std::endl;

    return signalPower; // dBm
}


// Function to calculate the minimum TxPower for a UE achieving DR >= 99%
double CalculateMinimumTxPowerForUE(double minTxPower, double maxTxPower, double frequency, double distance, 
                                    double bandwidth, double hBs, double hUt, double targetSnr, double margin, 
                                    double targetDr = 0.99) {

    if (distance < 1.0) {
        distance = 1.0;
    }

    const double k = 1.38e-23;  // Boltzmann constant (J/K)
    const double T = 290.0;     // Temperature in Kelvin
    double noiseFigure = 5.0;   // Noise figure in dB
    double noisePower = 10 * std::log10(k * T * bandwidth) + noiseFigure;
    double receiverSensitivity = noisePower + targetSnr;

    // Path Loss Models (LoS and NLoS)
    double losPathLoss = 32.4 + 21 * std::log10(distance / 1.0) + 20 * std::log10(frequency / 1e9) + 4.0;
    double nlosPathLoss = 22.4 + 35.3 * std::log10(distance / 1.0) + 21.3 * std::log10(frequency / 1e9) - 
                          0.3 * (hUt - 1.5) + 7.82;
    double pathLoss = std::max(losPathLoss, nlosPathLoss);

    // Initial TxPower Calculation
    double minimumTxPowerForUE = receiverSensitivity + pathLoss + margin;
    //minimumTxPowerForUE = std::clamp(minimumTxPowerForUE, minTxPower, maxTxPower);
    
    double currentTxPower = std::abs(minimumTxPowerForUE);
    double snr, ber, per, dr;
    double stepSize = 3.0;  // Step size for faster convergence
    double feedbackGain = 3.0; // Increase feedback gain to adjust power
    int maxIterations = 50;  // Limit iterations for faster convergence
    int iteration = 0;

    // Dynamic margin for each UE based on conditions
    double dynamicMargin = margin;
    double prevDr = 0.0;
    while (currentTxPower > minTxPower && iteration < maxIterations) {
        // Calculate DR for the current TxPower
        double receivedPower = currentTxPower - pathLoss;
        snr = receivedPower - noisePower;
        double snrScalar = pow(10, snr / 10.0);

        // BER Calculation (QPSK)
        //ber = 0.5 * std::erfc(std::sqrt(std::pow(10, snr / 10.0)));
        
        // QPSK BER formula: 2 * erfc(sqrt(SNR/2)) - (erfc(sqrt(SNR/2)))^2
        ber = 2 * std::erfc(std::sqrt(snrScalar / 2)) - std::pow(std::erfc(std::sqrt(snrScalar / 2)), 2);

        // PER Calculation
        double packetSizeBits = 1500 * 8; // Packet size in bits
        per = 1 - std::pow((1 - ber), packetSizeBits);
        dr = 1 - per;

        // Early termination if DR improvement is small
        if (iteration > 0 && std::abs(prevDr - dr) < 0.0001) {
            std::cout << "Termination due to small DR improvement." << std::endl;
            break;
        }
        prevDr = dr;

        // Adjust power based on feedback
        if (dr < targetDr) {
            currentTxPower += stepSize * feedbackGain;  // Increase power if DR is below the target
        } else {
            currentTxPower -= stepSize * feedbackGain;  // Reduce power if DR is above the target
        }

        // Ensure power stays within the min and max limits
        currentTxPower = std::clamp(currentTxPower, minTxPower, maxTxPower);

        // Gradually reduce step size once we're close to the target DR
        if (std::abs(targetDr - dr) < 0.01) {
            stepSize *= 0.5;  // Reduce the step size once we're close to the target DR
        }

        // Log the current state for debugging
        std::cout << "Iteration: " << iteration << ", CurrentTxPower: " << currentTxPower << ", DR: " << dr << std::endl;

        // Increment iteration
        iteration++;

        // Check for convergence: if DR is above target, break the loop
        if (dr >= targetDr) {
            std::cout << "Converged to target DR." << std::endl;
            break;
        }

        // Adjust dynamic margin based on DR and SINR to fine-tune convergence
        if (dr < targetDr) {
            dynamicMargin += 0.1;  // Increase margin slightly if DR is below target
        } else if (dr > targetDr + 0.05) {
            dynamicMargin -= 0.1;  // Reduce margin if DR is well above the target
        }
    }

    if (iteration >= maxIterations) {
        std::cout << "Warning: Maximum iterations reached, DR might not be fully optimized." << std::endl;
    }

    return currentTxPower;
}

// Structure to store UE, closest gNB, and distance
struct UeToGnbInfo {
    uint32_t ueId;
    uint32_t gnbId;
    double distance;
};

std::vector<UeToGnbInfo> closestGnbResults;

// Vector to store distances for all nodes
std::vector<double> distances;

void runSimulation(uint16_t gNbNum, uint16_t ueNumPergNb, double maxTxPower, double minTxPower, double maxDistance, double seed, int numRun, double frequency, int argc)
{    
  bool logging = false;

  // Traffic parameters:
  uint32_t udpPacketSize = 1500;
  uint32_t lambdaHT = 20000;

  // Simulation parameters:
  Time simTime = MilliSeconds (1000);
  Time udpAppStartTime = MilliSeconds (400);

  // NR parameters:
  uint16_t numerologyBwp1 = 3;
  double centralFrequencyBand1 = 6.8e9; // GHz
  double bandwidthBand1 = 400e6; // MHz
  double hUt = 1.5; // m
  double hBs = 10.0; // m  
  double targetSnr = 20; // dB
  double margin = 5.0; // dB
 
  if (logging)
  {
    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    LogComponentEnable ("LtePdcp", LOG_LEVEL_INFO);
  }

  // Set default configuration for RLC
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (999999999));

  int64_t randomStream = 1;
  GridScenarioHelper gridScenario;
  gridScenario.SetRows (1);
  gridScenario.SetColumns (gNbNum);
  gridScenario.SetHorizontalBsDistance (10.0);
  gridScenario.SetVerticalBsDistance (0.0);
  gridScenario.SetBsHeight (hBs);
  gridScenario.SetUtHeight (hUt);
  gridScenario.SetSectorization (GridScenarioHelper::SINGLE);
  gridScenario.SetBsNumber (gNbNum);
  gridScenario.SetUtNumber (ueNumPergNb * gNbNum);
  gridScenario.SetScenarioHeight (3);
  gridScenario.SetScenarioLength (3);
  randomStream += gridScenario.AssignStreams (randomStream);
  gridScenario.CreateScenario ();

  // Create container for all UEs
  NodeContainer ueContainer;
  for (uint32_t i = 0; i < gNbNum * ueNumPergNb; ++i)
  {
    Ptr<Node> ue = gridScenario.GetUserTerminals ().Get (i);
    ueContainer.Add (ue);
  }

//std::cout << "\nNumber of gNBs: " << gridScenario.GetBaseStations().GetN() << std::endl;

// Iterate over each gNB node and print its position
for (uint32_t i = 0; i < gridScenario.GetBaseStations().GetN(); ++i)
{
    Ptr<Node> gndbNode = gridScenario.GetBaseStations().Get(i); // Get the gNB node
    Ptr<MobilityModel> mobilitygnb = gndbNode->GetObject<MobilityModel>(); // Get the mobility model

    // Ensure the mobility model is available
    if (mobilitygnb)
    {
        // Get the position of the gNB
        //Vector position = mobilitygnb->GetPosition();
      /*  
        // Print the position (x, y, z)
        std::cout << "gNB " << i << " Position: (" 
                  << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;  */
    }
    else
    {
        std::cerr << "Error: gNB " << i << " does not have a mobility model!" << std::endl;
    }
}

// The reference position
Vector gNBPosition(0.0, 0.0, hBs);
//std::cout << "\nNumber of UEs: " << ueContainer.GetN() << std::endl;

// Position UEs at fixed positions read from a CSV file
MobilityHelper mobility;
Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

// Vector to hold the positions from the CSV file
std::vector<Vector> uePositions;

// Read the positions from the CSV file ("positions20ue200m.csv")
std::string csvFile = "positions20ue200m.csv";
if (!ReadPositionsFromCSV(csvFile, uePositions))
{
    std::cerr << "Error reading positions from file!" << std::endl;
    exit(1);
}

// Ensure we have enough positions for all UEs
if (uePositions.size() < ueNumPergNb * gNbNum)
{
    std::cerr << "Not enough positions in the file for all UEs!" << std::endl;
    exit(1);
}

// Assign the positions to the position allocator
uint32_t positionIndex = 0;
for (uint32_t i = 0; i < ueNumPergNb * gNbNum; ++i)
{
    // Use the fixed position from the CSV for each UE
    positionAlloc->Add(uePositions[positionIndex]);
 /*
    // Print the assigned position for debugging
    std::cout << "UE " << i << " Position: (" 
              << uePositions[positionIndex].x << ", " 
              << uePositions[positionIndex].y << ", 0.0)" << std::endl;
   */           
    // Calculate the distance from the gNB for this UE
    double dx = uePositions[positionIndex].x - gNBPosition.x;
    double dy = uePositions[positionIndex].y - gNBPosition.y;
    double dz = uePositions[positionIndex].z - gNBPosition.z;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Store the distance in the vector
    distances.push_back(distance);

    // Move to the next position in the list (loop if we run out of positions)
    positionIndex = (positionIndex + 1) % uePositions.size();
}

// Assign the generated positions to the MobilityHelper for UEs
mobility.SetPositionAllocator(positionAlloc);
mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
mobility.Install(ueContainer);
  
  Ptr<Node> selectedUe = ueContainer.Get (numRun); // Selecting the first UE from the container.  
  
  // Print the node being simulated (UE)
  std::cout << "\nUE " << selectedUe->GetId() << " Running on network... " << std::endl; // Print Node ID  

  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  nrHelper->SetBeamformingHelper (idealBeamformingHelper);
  nrHelper->SetEpcHelper (epcHelper);

  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1;  

  CcBwpCreator::SimpleOperationBandConf bandConf1 (centralFrequencyBand1, bandwidthBand1, numCcPerBand, BandwidthPartInfo::UMi_StreetCanyon);
  OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc (bandConf1);

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (0)));
  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

// Precompute distances between UEs and gNBs
std::vector<std::vector<double>> ueToGnbDistances(gridScenario.GetUserTerminals().GetN(),
                                                  std::vector<double>(gridScenario.GetBaseStations().GetN(), 0.0));

// Precompute distances between UEs
std::vector<std::vector<double>> ueToUeDistances(gridScenario.GetUserTerminals().GetN(),
                                                 std::vector<double>(gridScenario.GetUserTerminals().GetN(), 0.0));

// Precompute closest gNB for each UE
std::vector<std::tuple<uint32_t, uint32_t, double>> closestGnbResults;

// Calculate distances once
for (uint32_t ueIndex = 0; ueIndex < gridScenario.GetUserTerminals().GetN(); ++ueIndex) {
    Ptr<Node> ueNode = gridScenario.GetUserTerminals().Get(ueIndex);
    Vector uePosition = ueNode->GetObject<MobilityModel>()->GetPosition();

    Ptr<Node> closestGnb = nullptr;
    double minDistanceSquared = std::numeric_limits<double>::max();

    // Calculate UE to gNB distances
    for (uint32_t gnbIndex = 0; gnbIndex < gridScenario.GetBaseStations().GetN(); ++gnbIndex) {
        Ptr<Node> gnbNode = gridScenario.GetBaseStations().Get(gnbIndex);
        Vector gnbPosition = gnbNode->GetObject<MobilityModel>()->GetPosition();

        double distanceSquared = std::pow(uePosition.x - gnbPosition.x, 2) +
                                 std::pow(uePosition.y - gnbPosition.y, 2)
                                 /*std::pow(uePosition.z - gnbPosition.z, 2)*/;

        double distance = std::sqrt(distanceSquared);
        ueToGnbDistances[ueIndex][gnbIndex] = distance;

        if (distanceSquared < minDistanceSquared) {
            minDistanceSquared = distanceSquared;
            closestGnb = gnbNode;
        }
    }

    double minDistance = std::sqrt(minDistanceSquared);
    closestGnbResults.push_back({ueIndex, closestGnb->GetId(), minDistance});

    // Calculate UE to UE distances
    for (uint32_t otherUeIndex = 0; otherUeIndex < gridScenario.GetUserTerminals().GetN(); ++otherUeIndex) {
        if (otherUeIndex != ueIndex) {
            Ptr<Node> otherUeNode = gridScenario.GetUserTerminals().Get(otherUeIndex);
            Vector otherUePosition = otherUeNode->GetObject<MobilityModel>()->GetPosition();

            double distanceSquared = std::pow(uePosition.x - otherUePosition.x, 2) +
                                     std::pow(uePosition.y - otherUePosition.y, 2)
                                     /*std::pow(uePosition.z - otherUePosition.z, 2)*/;

            ueToUeDistances[ueIndex][otherUeIndex] = std::sqrt(distanceSquared);           
            
        }
    }
}

double interferencePower = 0.0;
// Process UEs in the low-latency container (ueLowLatContainer)
for (uint32_t j = 0; j < ueContainer.GetN(); ++j) {
    Ptr<Node> ue = ueContainer.Get(j);
    auto [ueId, servingGnbId, distance] = closestGnbResults[j];
    Ptr<Node> servingGnb = gridScenario.GetBaseStations().Get(servingGnbId);

    // Calculate initial txPower based on distance to gNB (log-distance model)
    //double txPower = maxTxPower;
    
    // Calculate signal power from the serving gNB
    	//double signalPower = calculateSignalPower(txPower, distance, frequency, hBs, hUt);
    //double signalPower_W = pow(10, (signalPower / 10)) / 1000;  // Convert signal power to Watts

    // Calculate interference from other UEs (3gpp UMi-street canyon model)
    for (uint32_t k = 0; k < ueContainer.GetN(); ++k) {
        if (k != j) {  // Skip the current UE itself
            double interferingDistance = ueToUeDistances[j][k];
            if(interferingDistance < 100){
                interferencePower += calculateSignalPower(maxTxPower, interferingDistance, frequency, hBs, hUt);
            }
        }
    }
    
    //std::cout << "Serving gNB: " << servingGnbId << std::endl;           
}
//std::cout << "Total interference: " << interferencePower << std::endl;

double totalBandwidth = bandwidthBand1;
auto [ueId, servingGnbId, distance] = closestGnbResults[numRun];
double reqTxPowerDbm = CalculateMinimumTxPowerForUE(minTxPower, maxTxPower, frequency, distance, totalBandwidth, hBs, hUt, targetSnr, margin);
double reqTxPowerMw = pow(10, reqTxPowerDbm / 10);
double reqTxPowerW = pow(10, reqTxPowerDbm / 10)/1000;
 
  nrHelper->InitializeOperationBand (&band1);

  allBwps = CcBwpCreator::GetAllBwps ({band1});

  Packet::EnableChecking ();
  Packet::EnablePrinting ();

  idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  uint32_t bwpIdForHighThroughput = 0;
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("NGBR_VIDEO_TCP_PREMIUM", UintegerValue (bwpIdForHighThroughput));
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("NGBR_VIDEO_TCP_PREMIUM", UintegerValue (bwpIdForHighThroughput));

  // Install the device for the selected UE
  NodeContainer selectedUeContainer;
  selectedUeContainer.Add (selectedUe);

  NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (gridScenario.GetBaseStations (), allBwps);
  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (selectedUeContainer, allBwps);

  randomStream += nrHelper->AssignStreams (enbNetDev, randomStream);
  randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);
  
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerologyBwp1));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("TxPower", DoubleValue (10 * log10 ((bandwidthBand1 / totalBandwidth) * reqTxPowerMw)));
  
  for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
  {
    DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
  }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
  {
    DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
  }

  // Create the internet and install the IP stack on the UE
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (gridScenario.GetUserTerminals ());

  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  for (uint32_t j = 0; j < 1; ++j)
  {
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (selectedUe->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  nrHelper->AttachToClosestGnb (ueNetDev, enbNetDev);

  uint16_t dlPortHighThroughput = 1236;

  ApplicationContainer serverApps;
  UdpServerHelper dlPacketSinkHighThroughput (dlPortHighThroughput);
  serverApps.Add (dlPacketSinkHighThroughput.Install (selectedUe));  // Install only for the selected UE

  UdpClientHelper dlClientHighThroughput;
  dlClientHighThroughput.SetAttribute ("RemotePort", UintegerValue (dlPortHighThroughput));
  dlClientHighThroughput.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));
  dlClientHighThroughput.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));
  dlClientHighThroughput.SetAttribute ("Interval", TimeValue (Seconds (1.0 / lambdaHT)));

  ApplicationContainer clientApps;
  dlClientHighThroughput.SetAttribute ("RemoteAddress", AddressValue (ueIpIface.GetAddress (0)));  // Point to the selected UE
  clientApps.Add (dlClientHighThroughput.Install (remoteHost));

  serverApps.Start (udpAppStartTime);
  clientApps.Start (udpAppStartTime);
  serverApps.Stop (simTime);
  clientApps.Stop (simTime);

  FlowMonitorHelper flowmonHelper;
  NodeContainer endpointNodes;
  endpointNodes.Add (remoteHost);
  endpointNodes.Add (selectedUe);

  Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install (endpointNodes);
  monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

  Simulator::Stop (simTime);
  Simulator::Run ();

  // Print per-flow statistics for selected UE
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  
  // Map to store fixed mapping between UE IDs and Flow IDs
    std::map<uint32_t, FlowId> ueToFlowMap;
    
  // Prepare data for CSV    
    uint32_t flowId = 1;    
    for (auto& ueInfo : closestGnbResults) {
        ueToFlowMap[std::get<0>(ueInfo)] = flowId++;  // Access the ueId using std::get<0>()
    }
        for (const auto& entry : ueToFlowMap) {
            if (entry.second == flowId) {
                ueId = entry.first;
                break;
            }
        }
	  
  // Using openResultsFile function to manage file opening and appending
    std::ofstream outFile = openResultsFile(seed);

    // Simulation duration and flow stats
    double flowDuration = (simTime - udpAppStartTime).GetSeconds();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6) protoStream.str("TCP");
        if (t.protocol == 17) protoStream.str("UDP");

        // Calculate metrics
        double packetLoss = (double)(i->second.txPackets - i->second.rxPackets) / i->second.txPackets * 100;
        double deliveryRatio = (double)i->second.rxPackets / i->second.txPackets * 100;
        double throughput = (i->second.rxPackets > 0) ? i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000 : 0;
        double latency = (i->second.rxPackets > 0) ? 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets : 0;
        double meanJitter = (i->second.rxPackets > 0) ? 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets : 0;
        double txEnergy = std::abs(reqTxPowerW * (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()));

        // Print statistics to console
        std::cout /* << "Flow " << i->first << " */ << "Results UE " << selectedUe->GetId() << ": "
                  << "(" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort
                  << ") proto " << protoStream.str() << "\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Packet Loss: " << packetLoss << " %\n";
        std::cout << "  Delivery Ratio: " << deliveryRatio << " %\n";
        std::cout << "  Throughput: " << throughput << " Mbps\n";
        std::cout << "  Latency: " << latency << " ms\n";
        std::cout << "  Mean jitter: " << meanJitter << " ms\n";
        std::cout << "  Tx Energy: " << txEnergy << " J\n";
        std::cout << "  Dist from gNB: " << distances[selectedUe->GetId()] << " m \n";
        std::cout << "  Tx Power: " << reqTxPowerDbm << " dBm \n\n";

        // Write statistics to CSV file
        outFile << selectedUe->GetId() << ","
                << i->second.txPackets << "," 
                << i->second.rxPackets << "," 
                << packetLoss << ","
                << deliveryRatio << "," 
                << throughput << "," 
                << latency << "," 
                << meanJitter << ","
                << txEnergy << "," 
                << distances[selectedUe->GetId()] << ","
                << reqTxPowerDbm << "\n";
    }

    // Close the CSV file
    outFile.close();

    // End of simulation cleanup
    Simulator::Destroy();
    
    }   

int main(int argc, char* argv[])
{
    // Set fixed parameters
    double maxTxPower = 30.0; // dBm
    double minTxPower = 0.0;  // dBm
    double maxDistance = 200.0; // meters   
     
    std::vector<int> seeds = {13, 89, 97, 229, 233, 241, 439, 491, 829, 857, 859, 881, 883, 887, 1021};
    std::vector<int> numRuns = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
        
    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 20;
    double frequency = 6.8e9;  //GHz    
           
    CommandLine cmd(__FILE__);

    cmd.AddValue("gNbNum", "The number of gNbs in multiple-ue topology", gNbNum);
    cmd.AddValue("ueNumPergNb", "The number of UE per gNb in multiple-ue topology", ueNumPergNb);
    cmd.AddValue("maxTxPower", "Maximum txPower to be used", maxTxPower);
    cmd.AddValue("minTxPower", "Minimum txPower to be used", minTxPower);
    cmd.AddValue("maxDistance", "Semente para aleatoriedade", maxDistance);
    //cmd.AddValue("seed", "Maximum Coverage distance from gNB", seed);
    
    // Parse the command line
    cmd.Parse(argc, argv); 
    
    std::string command0 = "/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/shared_read_cmd.so";
    //std::cout << "Running command0: " << command0 << std::endl;
    int ret0 = std::system(command0.c_str());
    if(ret0 != 0){
        //std::cout << "Command0 returned: " << ret0 << std::endl;
        std::cerr << "Failed to find shared_read_cmd writer executable file" << std::endl;
        exit(1);
    }
/*    
    std::string activecmd = "/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/shared_write_activecmd.so";
    //std::cout << "Running activecmd: " << activecmd << std::endl;
    int activeret = std::system(activecmd.c_str());
    if(activeret != 0){
        //std::cout << "activecmd returned: " << activeret << std::endl;
        std::cerr << "Failed to find shared_write_activecmd writer executable file" << std::endl;
        exit(1);
    }    
*/     
    for (int seed : seeds) {
    	RngSeedManager::SetSeed(seed);
    	//RngSeedManager::SetRun(run);    // set run times of each seed 
    	for(int numRun : numRuns){    	      
    	    runSimulation(gNbNum, ueNumPergNb, maxTxPower, minTxPower, maxDistance, seed, numRun, frequency, argc);
    	} 
    }
    
    std::string fetch_sort = "/home/ufg/anaconda3/envs/fa_5glena/myproject/net_fetch_sort.so";
    //std::cout << "Running fetch_sort: " << fetch_sort << std::endl;
    int sort_ret = std::system(fetch_sort.c_str());
    if(sort_ret != 0){
        //std::cout << "fetch_sort returned: " << sort_ret << std::endl;
        std::cerr << "Failed to find net_fetch_sort executable file" << std::endl;
        exit(1);
    } 
    
    std::string select_stats = "/home/ufg/anaconda3/envs/fa_5glena/myproject/net_select_stats.so";
    //std::cout << "Running select_stats: " << select_stats << std::endl;
    int select_ret = std::system(select_stats.c_str());
    if(select_ret != 0){
        //std::cout << "select_stats returned: " << select_ret << std::endl;
        std::cerr << "Failed to find net_select_stats executable file" << std::endl;
        exit(1);
    }     
    
    std::string command = "/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/shared_write_dr.so";
    //std::cout << "Running command: " << command << std::endl;
    int ret = std::system(command.c_str());
    if(ret != 0){
        //std::cout << "Command1 returned: " << ret << std::endl;
        std::cerr << "Failed to find shared_write_dr executable file" << std::endl;
        exit(1);
    }
    
    std::string command2 = "/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/shared_write_cmd.so";
    //std::cout << "Running command2: " << command2 << std::endl;
    int ret2 = std::system(command2.c_str());
    if(ret2 != 0){
        //std::cout << "Command2 returned: " << ret2 << std::endl;
        std::cerr << "Failed to find shared_write_cmd writer executable file" << std::endl;
        exit(1);
    } 
    
return 0;
}

