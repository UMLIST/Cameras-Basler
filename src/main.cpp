// ----------------------------------------------------------------------------------
// --HEADERS-------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h> 
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <csignal>
#include <chrono>
#include <ctime>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

// Object for log file
std::ofstream logFile;

// Flag to indicate if Ctrl+C signal is received
volatile sig_atomic_t stopFlag = 0;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    stopFlag = 1;
}

std::pair<std::tm, long long> get_PC_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto now_time_t = std::chrono::high_resolution_clock::to_time_t(now);
    std::tm time = *std::localtime(&now_time_t);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
    return {time, microseconds};
}

void print_PC_time(const std::pair<std::tm, long long>& timePair) {
    const auto& time = timePair.first;
    std::cout << "Timestamp: "
              << time.tm_year + 1900 << " "
              << time.tm_mon + 1 << " "
              << time.tm_mday << " "
              << time.tm_hour << ":"
              << time.tm_min << ":"
              << time.tm_sec << "."
              << timePair.second
              << std::endl;
}

// Function to print the both PC time and camera timestamp with frame number
void print_timestamps(const std::pair<std::tm, long long>& timePair, int64_t camera_timestamp, int framecount){
    const auto& PCtime = timePair.first;
    std::cout << "PC Time: " << PCtime.tm_year + 1900 << " " 
        << PCtime.tm_mon + 1 << " " 
        << PCtime.tm_mday << " "
        << PCtime.tm_hour << ":"
        << PCtime.tm_min << ":"
        << PCtime.tm_sec << "."
        // << timePair.second << ", ";
        << std::setw(6) << std::setfill('0') << timePair.second << ", ";
    std::cout << "Frame: " << framecount
    << ", Cam Timestamp: " << std::fixed << std::setprecision(6) << camera_timestamp / 1E9 << std::endl;
}


// Function to open the log file and write the header
void openLogFile(const std::string& filename, double FPS_target, double FPS_set, const std::string& autoExposureMode, double exposureTime, int widthValue, int heightValue, const std::string& camera_name) {

    logFile.open(filename);
    if (logFile.is_open()) {
        // Write header containing camera parameters
        logFile << "Camera Parameters\n";
        logFile << "Camera Model," << camera_name << "\n";
        logFile << "FPS (Target)," << FPS_target << "\n";
        logFile << "FPS (ACTUAL)," << FPS_set << "\n";
        logFile << "Auto Exposure Mode," << autoExposureMode << "\n";
        logFile << "Exposure Time," << exposureTime << "\n";
        logFile << "Resolution," << widthValue << "x" << heightValue << "\n\n";

        // Write column headers for timestamps
        logFile << "Frame,PC Timestamp,Camera Time\n";
    } else {
        cerr << "Error opening log file: " << filename << endl;
    }
}


// Function to log timestamps
void logTimestamps(const std::pair<std::tm, long long>& timePair, int64_t camera_timestamp, int framecount){
    const auto& PCtime = timePair.first;
    // Log PC timestamp and camera timestamp
    logFile << framecount << ","
    << std::put_time(&PCtime, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(6) << std::setfill('0') << timePair.second
        << "," << std::fixed << std::setprecision(6) << camera_timestamp / 1E9 << "\n";
}


// Function to close the log file
void closeLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
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
        // if (FPS_target != FPS_set) {
        if (std::abs(FPS_target - FPS_set) > .01) {
            cerr << "WARNING: Could not set FPS to " << FPS_target << ". Using " << FPS_set << " instead." << endl;
        }
        else {
            cout << "FPS set to " << FPS_set << endl;
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

    

        // Set frame counter to zero
        int64_t framecount = 0;
        // camera.TimestampReset.Execute(); // **This timer reset function does not work?        // Start the grabbing of NumFrames images if specified, otherwise, grab continuously
        // INodeMap& nodemap = camera.GetNodeMap();
        // Take a "snapshot" of the camera's current timestamp value
        // CCommandParameter(&camera.GetNodeMap(), "TimestampReset").Execute();

        // Get the PC time
        auto local_time = get_PC_time();
        print_PC_time(local_time);

        // Get starting camera timestamp
        // "There is an unspecified and variable delay between sending the TimestampLatch command and it becoming effective."
        camera.TimestampLatch.Execute();
        int64_t ts_START = camera.TimestampLatchValue.GetValue();


        print_timestamps(local_time, ts_START, framecount);

        // Open log file
        std::ostringstream filename;
        const auto& time = local_time.first;
        filename << "videolog_" << time.tm_year + 1900 << time.tm_mon + 1 << time.tm_mday << time.tm_hour << time.tm_min << time.tm_sec << ".csv";
        cout << "Log file: " << filename.str() << endl;
        std::string modelName = std::string(camera.GetDeviceInfo().GetModelName().c_str());
        openLogFile(filename.str(), FPS_target, FPS_set, autoExposureMode, exposureTime, (int)width.GetValue(), (int)height.GetValue(), modelName);


        // videoWriter.Open( "test.mp4" );
        // Name video with timestamp

        std::ostringstream videoFilename;
        videoFilename << "video_" << time.tm_year + 1900 << time.tm_mon + 1 << time.tm_mday << time.tm_hour << time.tm_min << time.tm_sec << ".mp4";
        Pylon::String_t videoFileNamePylon = videoFilename.str().c_str();
        videoWriter.Open(videoFileNamePylon);

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        camera.StartGrabbing(NumFrames);

        CGrabResultPtr ptrGrabResult;
    
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
                    auto local_time = get_PC_time();
                    print_timestamps(local_time, ts, framecount);
                    // Write to log file
                    logTimestamps(local_time, ts, framecount);

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

    closeLogFile();

    // ------------------------------------------------------------------------------
    // --EXIT PYLON------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    cerr << "Press enter to exit." << endl;
    while (cin.get() != '\n');


    return exitCode;

} //AutoInitTerm's descructor calls PylonTerminate() now