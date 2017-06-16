#include "structures_molecule.hpp"
#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "parser.hpp"
#include <math.h>
#define SIZE_POCKET 5 
#define NAME_DIMENSION 13

const float PI = 3.14159265;

using namespace std;
namespace po = boost::program_options;
using namespace boost::algorithm;

void createPocket(Atom pocket[],float distance){
	
	struct Vertex2D{
		
		float latitude;
		float longitude;
		
	}vertex2D[SIZE_POCKET*SIZE_POCKET];
	
	float latAlfa,latBeta,lonAlfa,lonBeta,phi;
	
	//Creates a mesh of equidistant 2d points 
	
	for(int i=0;i<SIZE_POCKET;i++){
		for(int j=0;j<SIZE_POCKET;j++){
		
			vertex2D[i*SIZE_POCKET+j].latitude=i;
			vertex2D[i*SIZE_POCKET+j].longitude=j;
			
		}
	}
	
	   latAlfa= ((vertex2D[2*SIZE_POCKET].latitude)*PI)/180;
	   latBeta= ((vertex2D[2*SIZE_POCKET+1].latitude)*PI)/180;
	   lonAlfa= ((vertex2D[2*SIZE_POCKET].longitude)*PI)/180;
	   lonBeta= ((vertex2D[2*SIZE_POCKET+1].longitude)*PI)/180;
	   phi=fabs(lonAlfa-lonBeta); 
	   
	   float radius= distance/(acos(sin(latBeta)*sin(latAlfa)+cos(latBeta)*cos(latAlfa)*cos(phi)));
	   
	   //Transforms the bidimensional points of a mesh into coordinates of equidistant atoms in the sphere
	   
	   for(int i=0;i<SIZE_POCKET;i++){
			for(int j=0;j<SIZE_POCKET;j++){
				
				pocket[i*SIZE_POCKET+j].x= radius*(sin(PI * vertex2D[i*SIZE_POCKET+j].latitude / SIZE_POCKET) *cos(2*PI * vertex2D[i*SIZE_POCKET+j].longitude / SIZE_POCKET));
				pocket[i*SIZE_POCKET+j].y= radius*(sin(PI * vertex2D[i*SIZE_POCKET+j].latitude / SIZE_POCKET) *sin(2*PI * vertex2D[i*SIZE_POCKET+j].longitude / SIZE_POCKET));
				pocket[i*SIZE_POCKET+j].z= radius*(cos(PI * vertex2D[i*SIZE_POCKET+j].latitude / SIZE_POCKET));
	  
			}
		}	  
}


int main( int argc, char** argv ) {

    int N_ELEMENTS;
    unsigned int platform_id=0, device_id=0;

	Atom* pocket = new Atom[SIZE_POCKET*SIZE_POCKET];
    createPocket(pocket,0.2);
	float* score = new float[1];
	int* bestMolecule = new int[1];

    string file_name = "NULL";
    int n = 0;
    string device = "NULL";
    
    po::options_description desc;
    
    desc.add_options()
    ("help, h", "Shows description of the options")
    ("file_name, f", po::value<string>(&file_name)->default_value("ace_ligands.mol2"), "Set file name")
    ("number, n", po::value<int>(&n)->default_value(1), "Set the number of the elements to be read")
    ("device, d", po::value<string>(&device)->default_value("cpu"), "Set the type of device you want use. Available option: <gpu> or <cpu>");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if(vm.count("help"))
        return 0;

    if(!vm.count("device"))
    {
        cout << "Device not setted\n";
        return 0;
    }
    
    N_ELEMENTS = n;
	//std::unique_ptr<Molecule[]> A(new Molecule[N_ELEMENTS]);
	Molecule* molecules = new Molecule[N_ELEMENTS];
    //molecules[0] = m1;
    
    molecules = parseFile(file_name, n);

	// Query for platforms
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	// Get a list of devices on this platform
	std::vector<cl::Device> devices;
    
    // Select the platform.
    to_lower(device);
    if(device.compare("gpu") == 0)
        platforms[platform_id].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    else if(device.compare("cpu") == 0)
        platforms[platform_id].getDevices(CL_DEVICE_TYPE_CPU, &devices);
    else
    {
        cout << "Unable to select the platform";
        return 0;
    }

	// Create a context
	cl::Context context(devices);

	// Create a command queue
	cl::CommandQueue queue = cl::CommandQueue( context, devices[device_id] );   // Select the device.
	// Create the memory buffers
	cl::Buffer bufferMolecules=cl::Buffer(context, CL_MEM_READ_ONLY, N_ELEMENTS * sizeof(Molecule));
	cl::Buffer bufferPocket=cl::Buffer(context, CL_MEM_READ_ONLY, SIZE_POCKET*SIZE_POCKET*sizeof(Atom));
	cl::Buffer bufferBestMolecule=cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int));
	cl::Buffer bufferBestScore=cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(score));

	// Copy the input data to the input buffers using the command queue.
	queue.enqueueWriteBuffer( bufferMolecules, CL_FALSE, 0, N_ELEMENTS * sizeof(Molecule), molecules);
	queue.enqueueWriteBuffer( bufferPocket, CL_FALSE, 0, SIZE_POCKET*SIZE_POCKET*sizeof(Atom), pocket);
	queue.enqueueWriteBuffer( bufferBestMolecule, CL_FALSE, 0, sizeof(int), bestMolecule);
	queue.enqueueWriteBuffer( bufferBestScore, CL_FALSE, 0, sizeof(float), score);

	// Read the program source
	std::ifstream sourceFile("kernel.cl");
	std::string sourceCode( std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
	cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()));

	// Make program from the source code
	cl::Program program=cl::Program(context, source);

	// Build the program for the devices
	program.build(devices);

	// Make kernel
	cl::Kernel doRotation_kernel(program, "doAllRotation");

	// Set the kernel arguments
	doRotation_kernel.setArg(0, bufferMolecules);
	doRotation_kernel.setArg(1, bufferPocket);
	doRotation_kernel.setArg(2, bufferBestMolecule);
	doRotation_kernel.setArg(3, bufferBestScore);

	// Execute the kernel
	cl::NDRange global( N_ELEMENTS );
	cl::NDRange local( 1 );
	queue.enqueueNDRangeKernel( doRotation_kernel, cl::NullRange, global, local );
	queue.enqueueReadBuffer( bufferBestScore, CL_TRUE, 0, sizeof(float), score);
	queue.enqueueReadBuffer( bufferBestMolecule, CL_TRUE, 0, sizeof(int), bestMolecule);

	std::cout << "\nThe best score is:  " << std::to_string(score[0]);
	//string name = str(molecules[*bestMolecule].name);
	std::cout << "\nThe best Molecule is:  " << molecules[*bestMolecule].name;
	return( EXIT_SUCCESS );
}