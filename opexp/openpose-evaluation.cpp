#include <hpe-core/utility.h>
#include <hpe-core/openpose_detector.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <unistd.h>

using std::vector;
using std::string;

using namespace hpecore;

int main(int argc, char * argv[])
{

    if(argc != 3) 
    {
        std::cout << "Usage: openpose-evaluation [video filename (.mp4)] [output path (.csv)]" << std::endl;
        return 0;
    }

    //open input and output files
    string video_path(argv[1]);
    string csv_path(argv[2]);
    string video_out_path = csv_path.substr(0, csv_path.size()-4) + ".mp4";

    cv::VideoCapture input_video;
    if(!input_video.open(video_path)) 
    {
        std::cerr << "Could not open video at: " << video_path << std::endl;
        return -1;
    }
    double total_frames = input_video.get(cv::CAP_PROP_FRAME_COUNT);
    int width = (int)input_video.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)input_video.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps = (int)input_video.get(cv::CAP_PROP_FPS);
    

    cv::VideoWriter output_video;
    if(!output_video.open(video_out_path, cv::VideoWriter::fourcc('h','2','6','4'), fps, cv::Size(width, height), true))
    {
        std::cerr << "Could not open output video at: " << video_out_path << std::endl;
        return -1;
    }

    hpecore::writer csv_writer;
    if(!csv_writer.open(csv_path))
    {
        std::cerr << "Could not open csv at: " << csv_path << std::endl;
        return -1;
    }

    hpecore::OpenPoseDetector openpose;
    if(!openpose.init("/openpose/models", "COCO", "256")) 
    {
        std::cerr << "Could not initialise openpose" << std::endl;
        return -1;
    }


    cv::Mat frame;


    cv::namedWindow("Result", cv::WINDOW_NORMAL);
    cv::resizeWindow("Result", cv::Size(640, 480));
    sleep(1);
    std::cout << "VIDEO: ["<<width<<","<<height<<"]"<<std::endl;
    std::cout << "Estimated time: "<<total_frames<<" @ 100ms = "
              << total_frames*0.1/60<<" minutes"<<std::endl;
    //process the video
    while(true) 
    {
        input_video >> frame;
        if(frame.empty()) break;

        double current_frame = input_video.get(cv::CAP_PROP_POS_FRAMES);
        csv_writer.write({input_video.get(cv::CAP_PROP_POS_MSEC)*0.001, 0.0, openpose.detect(frame)});
        //hpecore::drawProgressBar(frame, current_frame/total_frames);

        output_video << frame;

        cv::imshow("Result", frame);
        if(cv::waitKey(1) == 27) break;
        
    }

    cv::destroyAllWindows();
    output_video.release();
    input_video.release();
    csv_writer.close();
    return 0;
}
