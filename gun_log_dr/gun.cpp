
#include <HepMC3/GenEvent.h>
#include <HepMC3/WriterAscii.h>
#include <HepMC3/GenParticle.h>
#include <HepMC3/FourVector.h>
#include <HepMC3/GenVertex.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <random>
#include <cmath>

using namespace HepMC3;


// Add these includes at the top of your file
#include <random>
#include <cmath>
#include "G4Electron.hh"
#include "QBBC.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"


double get_mass_G4(int pid) {
    // Based on Example showing how to retrieve Geant4 particle properties
    // Alvaro Tolosa-Delgado, May 2024, CERN
    std::map<int, std::string> myMap;
    myMap[211] = "pi+";
    myMap[-211] = "pi-";
    myMap[2212] = "proton";
    myMap[-2212] = "proton";
    myMap[2112] = "neutron";
    myMap[111] = "pi0";
    myMap[130] = "kaon0L";
    myMap[-15]= "tau-";
    myMap[15]= "tau+";
    myMap[-13]= "mu-";
    myMap[13]= "mu+";
    myMap[22]= "gamma";
    myMap[-11]= "e-";
    myMap[11]= "e+";
    myMap[213]= "rho+";
    myMap[-213]= "rho-";
    myMap[310]= "kaon0S";
    myMap[321]= "kaon+";
    myMap[-321]= "kaon-";
    myMap[3112]= "sigma+";
    myMap[-3112]= "sigma-";
    myMap[3122]= "lambda";
    myMap[3312]= "xi-";

    std::string  particle_name = myMap[pid];
    // Initialize a builtin physics list, any would work in this case
    G4VModularPhysicsList* physicsList = new QBBC; // from QBBC.hh
    // Initialize the definition of all the particles known to Geant4
    physicsList->ConstructParticle();
    
    // now the Particle table is ready to be used
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle(particle_name);
    G4double particle_mass = particle->GetPDGMass();
    particle_mass = particle_mass / 1e3;
    // std::cout << "particle_mass: " << particle_mass << std::endl;
    return particle_mass;
}



void generate_event(WriterAscii& writer, const std::vector<int>& pid_list, 
                    const std::vector<int>& npart_range, const std::vector<float>& eta_range, 
                    const std::vector<float>& mom_range, const std::vector<float>& dr_range) {
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate random eta
    std::uniform_real_distribution<> eta_dist(eta_range[0], eta_range[1]);
    float eta = eta_dist(gen);

    // Generate random phi
    std::uniform_real_distribution<> phi_dist(-M_PI, M_PI);
    float phi = phi_dist(gen);

    // Generate random number of particles
    std::uniform_int_distribution<> npart_dist(npart_range[0], npart_range[1]);
    int k = npart_dist(gen);

    // Create a new event
    GenEvent evt(Units::GEV, Units::MM);

    // Create a vertex at the origin
    GenVertexPtr v = std::make_shared<GenVertex>();

    GenParticlePtr p1 = make_shared<GenParticle>(FourVector (0.0,    0.0,   125.0,  125.0  ),11,  3);
    GenParticlePtr p2 = make_shared<GenParticle>(FourVector (0.0,    0.0,   -125.0,  125.0  ),11,  3);

    v->add_particle_in(p1);
    v->add_particle_in(p2);

    std::uniform_real_distribution<> log_dr_dist(log(dr_range[0]), log(dr_range[1]));
    float drmax = exp(log_dr_dist(gen));
    std::cout<<" --- new event ----------------\n";
    std::cout<<k<<","<<eta<<","<<phi<<","<<drmax<<","<<dr_range[0]<<","<<dr_range[1]<<"\n";
    // Loop over each particle
    for (int i = 0; i < k; i++) {
        // Generate random PID
        std::uniform_int_distribution<> pid_dist(0, pid_list.size() - 1);
        int pid = pid_list[pid_dist(gen)];

        // Generate random direction within drmax
        
        float etap, phip;
        do {
            std::uniform_real_distribution<> deta_dist(-drmax, drmax);
            std::uniform_real_distribution<> dphi_dist(-drmax, drmax);
            etap = eta + deta_dist(gen);
            phip = phi + dphi_dist(gen);
        } while (sqrt(pow(etap - eta, 2) + pow(phip - phi, 2)) > drmax);

        // Create the particle

        // Get the particle mass
        double mass = get_mass_G4(pid);

        // Convert eta to theta
        float theta = 2.0 * atan(exp(-etap));

        // Generate random momentum
        std::uniform_real_distribution<> log_mom_dist(log(mom_range[0]), log(mom_range[1]));
        float momp = exp(log_mom_dist(gen));

         //std::cout<<"  "<<i<<","<<pid<<","<<momp<<","<<etap<<","<<phip<<"\n";
        // Compute px, py, pz, E
        float px = momp * sin(theta) * cos(phip);
        float py = momp * sin(theta) * sin(phip);
        float pz = momp * cos(theta);
        float e = sqrt(px*px + py*py + pz*pz + mass*mass);

        // Create the particle
        GenParticlePtr particle = make_shared<GenParticle>(FourVector(px, py, pz, e), pid, 1);

        // add the particle to the vertex
        //v->add_particle_in(particle);
        v->add_particle_out(particle);

    }

    evt.add_vertex(v);


    // Write the event to the file
    writer.write_event(evt);

}


int main(int argc, char** argv) {
    // Check that the correct number of parameters were passed
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>\n";
        return 1;
    }

    // Open the configuration file
    std::ifstream configFile(argv[1]);
    if (!configFile) {
        std::cerr << "Could not open configuration file " << argv[1] << "\n";
        return 1;
    }

    // Parse the configuration file
    std::map<std::string, std::string> config;
    std::string line;
    while (std::getline(configFile, line)) {
        if (line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, value;
        if (!(iss >> key >> value)) {
            break;  // error
        }
        config[key] = value;
    }

    // Convert nevents from string to int
    float nevents = std::stof(config["nevents"]);

    // Convert npart_range from string to vector<int>
    std::vector<int> npart_range;
    std::istringstream npart_range_stream(config["npart_range"]);
    std::string npart;
    while (std::getline(npart_range_stream, npart, ',')) {
        npart_range.push_back(std::stoi(npart));
    }

    // Convert eta_range from string to vector<float>
    std::vector<float> eta_range;
    std::istringstream eta_range_stream(config["eta_range"]);
    std::string eta;
    while (std::getline(eta_range_stream, eta, ',')) {
        eta_range.push_back(std::stof(eta));
    }

    // Convert mom_range from string to vector<float>
    std::vector<float> mom_range;
    std::istringstream mom_range_stream(config["mom_range"]);
    std::string mom;
    while (std::getline(mom_range_stream, mom, ',')) {
        mom_range.push_back(std::stof(mom));
    }

    // // Convert drmax from string to float
    // float drmax = std::stof(config["drmax"]);

    // Convert mom_range from string to vector<float>
    std::vector<float> dr_range;
    std::istringstream dr_range_stream(config["dr_range"]);
    std::string dr;
    while (std::getline(dr_range_stream, dr, ',')) {
        dr_range.push_back(std::stof(dr));
    }

    // Convert pid_list from string to vector<int>
    std::vector<int> pid_list;
    std::istringstream pid_list_stream(config["pid_list"]);
    std::string pid;
    while (std::getline(pid_list_stream, pid, ',')) {
        pid_list.push_back(std::stoi(pid));
    }

    // Print the parsed values for debugging
    std::cout << "nevents: " << nevents << "\n";

    std::cout << "npart_range: ";
    for (int npart : npart_range) {
        std::cout << npart << " ";
    }
    std::cout << "\n";

    std::cout << "eta_range: ";
    for (float eta : eta_range) {
        std::cout << eta << " ";
    }
    std::cout << "\n";

    std::cout << "mom_range: ";
    for (float mom : mom_range) {
        std::cout << mom << " ";
    }
    std::cout << "\n";

    std::cout << "dr_range: ";
    for (float dr : dr_range) {
        std::cout << dr << " ";
    }
    std::cout << "\n";

    std::cout << "pid_list: ";
    for (int pid : pid_list) {
        std::cout << pid << " ";
    }
    std::cout << "\n";

    // Open the output file
    WriterAscii writer("events.hepmc");

    // Generate and write n events
    for (int i = 0; i < nevents; i++) {
        generate_event(writer, pid_list, npart_range, eta_range, mom_range, dr_range);
    }

    writer.close();  // This will add the "HepMC::Asciiv3-END_EVENT_LISTING" line
    return 0;
}

