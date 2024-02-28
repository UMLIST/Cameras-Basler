// ----------------------------------------------------------------------------------
// --HEADERS-------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h> 
#include <stdio.h>
#include <stdlib.h>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

// ----------------------------------------------------------------------------------
// --BEGIN MAIN----------------------------------------------------------------------
// ----------------------------------------------------------------------------------
int main( int argc, char** argv)
{
    // ------------------------------------------------------------------------------
    // --ASSIGN ARGUMENTS TO VARIABLES-----------------------------------------------
    // ------------------------------------------------------------------------------
    
    double FPS = atof(argv[1]); // Frames per second, pulled from first input argument
    int NumFrames = atof(argv[2]); // Number of frames (FPS * Runtime(s))

    
    // ------------------------------------------------------------------------------
    // --INIT PYLON------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    // Exit code of sample application
    int exitCode = 0;

    // Before using pylon methods, initialize pylon runtime
    Pylon::PylonAutoInitTerm autoInitTerm; //PylonInitialize() will be called now
    
    try
    {
        // create first camera device
        // Use CBaslerUniversalInstantCamera or else it throws error
        CBaslerUniversalInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice() );

        // ------------------------------------------------------------------------------
        // --SETTINGS MANAGEMENT---------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Must open camera before modifying settings
        camera.Open();
    
        // open camera for settings parameters
        camera.AcquisitionFrameRate.SetValue(FPS);
        double FPS_out = camera.AcquisitionFrameRate.GetValue();
    
        // set acquisition mode to continuous
        camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
    
        // set trigger to frame start
        camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
        camera.TriggerMode.SetValue(TriggerMode_Off);
    
        // ------------------------------------------------------------------------------
        // --VIDEO MANAGEMENT------------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Create a video writer object.
        CVideoWriter videoWriter;
        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 10;

        // Get the required camera settings.
        CIntegerParameter width( camera.GetNodeMap(), "Width" );
        CIntegerParameter height( camera.GetNodeMap(), "Height" );
        CEnumParameter pixelFormat( camera.GetNodeMap(), "PixelFormat" );

        // Optional: Depending on your camera or computer, you may not be able to save
        // a video without losing frames. Therefore, we limit the resolution:
        width.TrySetValue( 1920, IntegerValueCorrection_Nearest );
        height.TrySetValue( 1080, IntegerValueCorrection_Nearest );

        // Map the pixelType
        CPixelTypeMapper pixelTypeMapper( &pixelFormat );
        EPixelType pixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue( pixelFormat.GetIntValue() );

        // Set parameters before opening the video writer.
        videoWriter.SetParameter(
        (uint32_t) width.GetValue(),
                (uint32_t) height.GetValue(),
                pixelType,
                FPS_out,
                90 );

        // ------------------------------------------------------------------------------
        // --PRINTER HEADER INFO TO CLI--------------------------------------------------
        // ------------------------------------------------------------------------------
        cout << "FPS: " << FPS_out << endl;
        cout << "No. of Frames: " << NumFrames << endl; 
        cout << "Camera Model: " << camera.GetDeviceInfo().GetModelName() << endl; // get the cameras model name and print to terminal

        camera.Close();

        // ------------------------------------------------------------------------------
        // --START CAM / VIDEO ----------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Open the video writer.
        videoWriter.Open( "_TestVideo.mp4" );

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        camera.StartGrabbing( NumFrames );

        CGrabResultPtr ptrGrabResult;

        while (camera.IsGrabbing())
            {
                // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
                camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException );

                // Image grabbed successfully?
                if (ptrGrabResult->GrabSucceeded())
                {
                    // Access the image data.
                    cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                    cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                    const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                    cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
                }

                else
                {
                    cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
                }

            videoWriter.Add( ptrGrabResult );

                // If images are skipped, writing video frames takes too much processing time.
                cout << "Images Skipped = " << ptrGrabResult->GetNumberOfSkippedImages() << boolalpha
                << "; Image has been converted = " << !videoWriter.CanAddWithoutConversion( ptrGrabResult )
                << endl;
            }

    }
    catch(const GenericException& e)
    {
        cerr << "an exception has occured" << endl 
        << e.GetDescription() << endl;
        exitCode = 1;
    }
    

    // ------------------------------------------------------------------------------
    // --EXIT PYLON------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    cerr <<endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');
    

    return exitCode;

} //AutoInitTerm's descructor calls PylonTerminate() now