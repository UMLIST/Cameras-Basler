// ----------------------------HEADERS------------------------------------------------------

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h> 
#include <stdio.h>
#include <stdlib.h>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

// -----------------------------BEGIN MAIN -------------------------------------------------
int main( int argc, char** argv)
{
    //--------------------------ASSIGN ARGUMENTS TO VARIABLES---------------------------
    
    double FPS = atof(argv[1]);         // Frames per second, pulled from first input argument
    int NumFrames = atof(argv[2]);      // Number of frames (FPS * Runtime(s)), pulled from second input argument

    cout << "FPS: " << FPS << ", No. of Frames: " << NumFrames << "\n"; // print input values to the Command line


    //------------------------INIT PYLON--------------------------------------------------
    // Exit code of sample application
    int exitCode = 0;

    // Before using pylon methods, initialize pylon runtime
    PylonInitialize();


    // -----------------------EXIT PYLON---------------------------------------------------
    cerr <<endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');
    
    // Release pylon resources
    PylonTerminate();

    return exitCode;

}