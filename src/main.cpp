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
#include <chrono>
#include <ctime>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

// Flag to indicate if Ctrl+C signal is received
volatile sig_atomic_t stopFlag = 0;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    stopFlag = 1;
}

// Get the current system time (to microsecond precision)
std::tm get_PC_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto now_time_t = std::chrono::high_resolution_clock::to_time_t(now);
    return *std::localtime(&now_time_t);
}

// Print PC time (only)
void print_PC_time(const std::tm &local_time) {
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;

    std::cout << "Current time: " 
            << std::put_time(&local_time, "%a %b %e %Y %H:%M:%S")
            << "." << std::setw(6) << std::setfill('0') << micros << std::endl;
}

void print_timestamps(const std::tm &local_time, int64_t camera_timestamp, int framecount) {
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    // Print the PC time
    std::cout << "Current time: " 
            << std::put_time(&local_time, "%a %b %e %Y %H:%M:%S")
            << "." << std::setw(6) << std::setfill('0') << micros % 1000000 << ", ";

    // Print the camera timestamp
    std::cout << "Frame: " << framecount << ", Cam Timestamp: " << camera_timestamp / 1E9 << std::endl; // divide by 1E9 to convert ticks to sec
}

// ----------------------------------------------------------------------------------
// --BEGIN MAIN----------------------------------------------------------------------
// ----------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // ------------------------------------------------------------------------------
    // --ASSIGN ARGUMENTS TO VARIABLES-----------------------------------------------
    // ------------------------------------------------------------------------------
    
    double FPS = -1; // Default value for FPS
    int NumFrames = -1; // Default value for NumFrames
    string autoExposureMode = "Off"; // Default value for autoExposureMode
    double exposureTime = -1; // Default value for exposureTime
    bool crop1080 = false; // Default value for crop1080
    bool crop720 = false; // Default value for crop720
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-fps" && i + 1 < argc) {
            FPS = atof(argv[i + 1]);
        } else if (arg == "-frames" && i + 1 < argc) {
            NumFrames = atoi(argv[i + 1]) + 1;
        } else if (arg == "-autoexposure" && i + 1 < argc) {
            autoExposureMode = argv[i + 1];
        } else if (arg == "-exposuretime" && i + 1 < argc) {
            exposureTime = atof(argv[i + 1]);
        } else if  (arg == "-crop1080") {
            crop1080 = true;
        } else if  (arg == "-crop720") {
            crop720 = true;
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

        // Set the pixel format to Bayer RG 8 ("Raw", note: faster than RGB 8)
        camera.PixelFormat.SetValue(PixelFormat_BayerRG8);
    
        // Set acquisition mode to continuous if NumFrames not provided
        if (NumFrames == -1) {
            camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
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

        // Set FPS
        camera.AcquisitionFrameRateEnable.SetValue(true);
        camera.AcquisitionFrameRate.SetValue(FPS);
        double FPS_target = camera.AcquisitionFrameRate.GetValue();
        // Get the resulting frame rate
        double FPS_set = camera.ResultingFrameRate.GetValue();
        // cout << "Resulting Frame Rate: " << d << endl;
        if (FPS_target != FPS_set) {
            cerr << "Could not set FPS to " << FPS_target << ". Using " << FPS_set << " instead." << endl;
        }



        // ------------------------------------------------------------------------------
        // --VIDEO MANAGEMENT------------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Create a video writer object.
        CVideoWriter videoWriter;
        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 10;

        // Enable Chunks
        //camera.ChunkModeActive.SetValue( true );
        //camera.ChunkSelector.SetValue( ChunkSelector_Timestamp );
        //camera.ChunkEnable.SetValue( true );

        // Optional: Depending on your camera or computer, you may not be able to save
        // a video without losing frames.

        // Get the required camera settings.
        CIntegerParameter width( camera.GetNodeMap(), "Width" );
        CIntegerParameter height( camera.GetNodeMap(), "Height" );
        CEnumParameter pixelFormat( camera.GetNodeMap(), "PixelFormat" );
        
        if (crop1080) {
            // CROPPED 1080p
            width.TrySetValue( 1920, IntegerValueCorrection_Nearest );
            height.TrySetValue( 1080, IntegerValueCorrection_Nearest );
            // Center the image ROI
            camera.BslCenterX.Execute();
            camera.BslCenterY.Execute();
        } else if (crop720) {
            // CROPPED 720p
            width.TrySetValue( 1280, IntegerValueCorrection_Nearest );
            height.TrySetValue( 720, IntegerValueCorrection_Nearest );
            // Center the image ROI
            camera.BslCenterX.Execute();
            camera.BslCenterY.Execute();
        }
        else {
            /* Max Resolution */
            width.TrySetValue( 3860, IntegerValueCorrection_Nearest );
            height.TrySetValue( 2178, IntegerValueCorrection_Nearest );
        }



        // Map the pixelType
        CPixelTypeMapper pixelTypeMapper( &pixelFormat );
        EPixelType pixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue( pixelFormat.GetIntValue() );

        // Set parameters before opening the video writer.
        videoWriter.SetParameter(
        (uint32_t) width.GetValue(),
                (uint32_t) height.GetValue(),
                pixelType,
                FPS_set,
                90 );

        // ------------------------------------------------------------------------------
        // --PRINTER HEADER INFO TO CLI--------------------------------------------------
        // ------------------------------------------------------------------------------
        cout << "Recording Parameters:" << endl;
        cout << "FPS: " << FPS_set << endl;
        cout << "No. of Frames: " << NumFrames-1 << endl; 
        cout << "Camera Model: " << camera.GetDeviceInfo().GetModelName() << endl; // get the cameras model name and print to terminal
        
        // camera.Close(); // NOTE: commented out, causing errors

        // ------------------------------------------------------------------------------
        // --START CAM / VIDEO ----------------------------------------------------------
        // ------------------------------------------------------------------------------
        // Open the video writer.
        videoWriter.Open( "test.mp4" );

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        camera.StartGrabbing(NumFrames);

        CGrabResultPtr ptrGrabResult;

        // Set frame counter to zero
        int64_t framecount = 0;
        // camera.TimestampReset.Execute(); // **This timer reset function does not work?        // Start the grabbing of NumFrames images if specified, otherwise, grab continuously

        // // Get the current system time
        // auto now = std::chrono::high_resolution_clock::now();
        // auto now_time_t = std::chrono::high_resolution_clock::to_time_t(now);
        // std::tm *local_time = std::localtime(&now_time_t);
        // auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000; // Extract microseconds
        // // Print the time
        // std::cout << "Current time: " 
        //         << std::put_time(local_time, "%a %b %e %Y %H:%M:%S")
        //         << "." << std::setw(6) << std::setfill('0') << micros << std::endl;

        // Get the PC time
        std::tm local_time = get_PC_time();
        // print_PC_time(local_time);

        // Get starting camera timestamp
        camera.TimestampLatch.Execute();
        int64_t ts_START = camera.TimestampLatchValue.GetValue();
        // cout << "Frame: " << framecount << ", Start Cam Timestamp: " << ts_START / 1E9 << endl; // divide by 1E9 to convert ticks to sec

        print_timestamps(local_time, ts_START, framecount);
    
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

                    /* Get CAMERA Time //
                    Timestamp Tick Frequency for daA3840-45uc:
                     1 GHz (= 1 000 000 000 ticks per second, 1 tick = 1 ns) */
                    camera.TimestampLatch.Execute();
                    int64_t ts = camera.TimestampLatchValue.GetValue();
                    
                    // Get PC time
                    std::tm local_time = get_PC_time();

                    // Print the PC time and camera timestamp
                    print_timestamps(local_time, ts, framecount);

                    /* Print timestamps separately... */
                    // cout << "Frame: " << framecount << ", Cam Timestamp: " << ts / 1E9 << endl; // divide by 1E9 to convert ticks to sec
                    // std::tm local_time = get_PC_time();
                    // print_PC_time(local_time);

            }
            
            else
            {
                cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }

            videoWriter.Add( ptrGrabResult );

            // If images are skipped, writing video frames takes too much processing time.
            // cout << "Images Skipped = " << ptrGrabResult->GetNumberOfSkippedImages() << boolalpha
            // << "; Image has been converted = " << !videoWriter.CanAddWithoutConversion( ptrGrabResult )
            // << endl;

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