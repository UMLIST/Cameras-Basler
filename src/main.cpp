// ----------------------------------------------------------------------------------
// --HEADERS-------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h> 
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <csignal>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

// Flag to indicate if Ctrl+C signal is received
volatile sig_atomic_t stopFlag = 0;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    stopFlag = 1;
}

// ----------------------------------------------------------------------------------
// --BEGIN MAIN----------------------------------------------------------------------
// ----------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // ------------------------------------------------------------------------------
    // --ASSIGN ARGUMENTS TO VARIABLES-----------------------------------------------
    // ------------------------------------------------------------------------------
    
    double FPS = 25; // Default value for FPS
    int NumFrames = -1; // Default value for NumFrames
    string autoExposureMode = "Off"; // Default value for autoExposureMode
    double exposureTime = -1; // Default value for exposureTime
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-fps" && i + 1 < argc) {
            FPS = atof(argv[i + 1]);
        } else if (arg == "-frames" && i + 1 < argc) {
            NumFrames = atoi(argv[i + 1]);
        } else if (arg == "-autoexposure" && i + 1 < argc) {
            autoExposureMode = argv[i + 1];
        } else if (arg == "-exposuretime" && i + 1 < argc) {
            exposureTime = atof(argv[i + 1]);
        }
    }
    
    // Check if mandatory argument FPS is provided
    if (FPS == -1) {
        cerr << "FPS is a mandatory argument." << endl;
        return 1;
    }

    // ------------------------------------------------------------------------------
    // --INIT PYLON------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    // Exit code of sample application
    int exitCode = 0;

    // Before using pylon methods, initialize pylon runtime
    Pylon::PylonAutoInitTerm autoInitTerm; //PylonInitialize() will be called now
    
    try
    {
        // Install signal handler for Ctrl+C
        signal(SIGINT, signalHandler);

        // create first camera device
        // Use CBaslerUniversalInstantCamera or else it throws error
        CBaslerUniversalInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice() );

        // ------------------------------------------------------------------------------
        // --SETTINGS MANAGEMENT---------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Must open camera before modifying settings
        camera.Open();

        // Set FPS
        camera.AcquisitionFrameRateEnable.SetValue(true);
        camera.AcquisitionFrameRate.SetValue(FPS);
        double FPS_out = camera.AcquisitionFrameRate.GetValue();
    
        // Set acquisition mode to continuous if NumFrames not provided
        if (NumFrames == -1) {
            camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
        }
        else {
            NumFrames = NumFrames + 1;
        }

        // Set auto exposure mode
        if (autoExposureMode == "Continuous") {
            camera.ExposureAuto.SetValue(ExposureAuto_Continuous);
        } else if (autoExposureMode == "Once") {
            camera.ExposureAuto.SetValue(ExposureAuto_Once);
        } else {
            camera.ExposureAuto.SetValue(ExposureAuto_Off);
        }

        // Set exposure time if provided
        if (exposureTime > 0) {
            camera.ExposureAuto.SetValue(ExposureAuto_Off);
            camera.ExposureTime.SetValue(exposureTime);
        }
        
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

        // Enable Chunks
        //camera.ChunkModeActive.SetValue( true );
        //camera.ChunkSelector.SetValue( ChunkSelector_Timestamp );
        //camera.ChunkEnable.SetValue( true );

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
        
        // camera.Close(); // NOTE: commented out, causing errors

        // ------------------------------------------------------------------------------
        // --START CAM / VIDEO ----------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Open the video writer.
        videoWriter.Open( "_TestVideo.mp4" );

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        camera.StartGrabbing(NumFrames);

        CGrabResultPtr ptrGrabResult;

        // Set frame counter to zero
        int64_t framecount = 0;
        //camera.TimestampReset.Execute(); // **This timer reset function does not work?        // Start the grabbing of NumFrames images if specified, otherwise, grab continuously

        while (!stopFlag && (NumFrames == -1 || framecount < NumFrames))
        {
            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException );

            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                    // Access the image data.
                    //cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                    //cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                    //const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                    //cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;

                    camera.TimestampLatch.Execute();
                    int64_t ts = camera.TimestampLatchValue.GetValue();
                    cout << "Frame: " << framecount << ", Timestamp: " << ts / 1E9 << endl; // 1 tick equals 1 GHz, divide by 1E9 to convert to sec
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

            framecount++;

        }

    }
    catch (const GenericException& e) 
    {
        cerr << "An exception has occurred" << endl 
             << e.GetDescription() << endl;
        exitCode = 1;
    }


    // ------------------------------------------------------------------------------
    // --EXIT PYLON------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    cerr << "Press enter to exit." << endl;
    while (cin.get() != '\n');


    return exitCode;

} //AutoInitTerm's descructor calls PylonTerminate() now