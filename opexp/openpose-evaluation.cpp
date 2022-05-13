#include <hpe-core/utility.h>
#include <hpe-core/openpose_detector.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

using std::vector;
using std::string;

// class openpose-evaluation  {

// private:

//     vReadPort<vector<AE> > input_events;
//     BufferedPort<Bottle> input_gt;
//     BufferedPort<ImageOf<PixelMono>> input_grey;

//     hpecore::skeleton13 skeleton_gt{0,0,0,0,0,0,0,0,0,0,0,0,0};
//     hpecore::skeleton13 skeleton_detection{0,0,0,0,0,0,0,0,0,0,0,0,0};

//     hpecore::jointName my_joint;
//     hpecore::queuedVelocity velocity_estimator;
//     hpecore::kfEstimator state;
//     hpecore::PIM pim;
//     std::mutex m;

//     cv::Size image_size;
//     cv::Mat vis_image;
//     cv::Mat grey_frame;

//     hpecore::writer skelwriter;

//     hpecore::OpenPoseDetector detop;

// public:

//     bool configure(yarp::os::ResourceFinder& rf) override
//     {
//         if (!yarp::os::Network::checkNetwork(2.0)) {
//             std::cout << "Could not connect to YARP" << std::endl;
//             return false;
//         }
//         //set the module name used to name ports
//         setName((rf.check("name", Value("/isaac-hpe")).asString()).c_str());

//         //open io ports
//         if(!input_events.open(getName("/AE:i"))) {
//             yError() << "Could not open events input port";
//             return false;
//         }

//         if(!input_gt.open(getName("/gt:i"))) {
//             yError() << "Could not open input port";
//             return false;
//         }

//         if (!input_grey.open(getName("/grey:i"))) {
//             yError() << "Could not open input port";
//             return false;
//         }

//         Network::connect("/file/ch0dvs:o", getName("/AE:i"), "fast_tcp");
//         Network::connect("/file/ch2GT10Hzskeleton:o", getName("/gt:i"), "fast_tcp");
//         Network::connect("/file/ch3frames:o", getName("/grey:i"), "fast_tcp");

//         image_size = cv::Size(rf.check("width" , Value(640)).asInt32(),
//                               rf.check("height", Value(480)).asInt32());
//         vis_image = cv::Mat(image_size, CV_8UC3, cv::Vec3b(0, 0, 0));
//         grey_frame = cv::Mat(image_size, CV_8UC1, cv::Vec3b(0, 0, 0));
//         cv::namedWindow("isaac-hpe", cv::WINDOW_NORMAL);

//         my_joint = hpecore::str2enum("handL");
//         velocity_estimator.setParameters(40, 3, 5, 1000);

//         if(rf.check("filepath")) {
//             std::string filepath = rf.find("filepath").asString();
//             if(skelwriter.open(filepath))
//                 yInfo() << "saving data to:" << filepath;
//         }

//         state.initialise({1.0, 1.0});
//         pim.init(image_size.width, image_size.height);

//         std::string models_path = rf.check("models_path", Value("/openpose/models")).asString();
//         std::string pose_model = rf.check("pose_model", Value("COCO")).asString();

//         if(!detop.init(models_path, pose_model))
//             return false;
//         yError() << "OpenPose initialised";

//         return Thread::start();
//     }

//     double getPeriod() override
//     {
//         //run the module as fast as possible. Only as fast as new images are
//         //available and then limited by how fast OpenPose takes to run
//         return 0.05; 
//     }

//     bool interruptModule() override
//     {
//         //if the module is asked to stop ask the asynchronous thread to stop
//         return Thread::stop();
//     }

//     void onStop() override
//     {
//         input_events.close();
//         input_grey.close();
//         input_gt.close();
//         skelwriter.close();
//     }

//     bool close() override
//     {
//         //when the asynchronous thread is asked to stop, close ports and do other clean up
//         return true;
//     }

//     //synchronous thread
//     virtual bool updateModule() 
//     {
//         cv::Size compiled_size(std::max(grey_frame.size().width, vis_image.size().width),
//                             std::max(grey_frame.size().height, vis_image.size().height));
//         cv::Mat compiled(compiled_size, CV_8UC3, cv::Vec3b(0, 0, 0));
//         cv::Mat rgb; cv::cvtColor(grey_frame, rgb, cv::COLOR_GRAY2BGR);
//         rgb.copyTo(compiled(cv::Rect(0, 0, rgb.cols, rgb.rows)));
//         vis_image.copyTo(compiled, vis_image);
//         hpecore::drawSkeleton(compiled, skeleton_gt, {0, 0, 255});
//         //hpecore::drawSkeleton(compiled, state.query(), {128, 128, 0});
//         //cv::rectangle(compiled, cv::Rect(state.query(my_joint).u-20, state.query(my_joint).v-20, 40, 40), CV_RGB(255, 0, 0));
//         cv::imshow("isaac-hpe", compiled);
//         //cv::waitKey(1);
//         vis_image.setTo(cv::Vec3b(0, 0, 0));

//         cv::Mat floater;
//         pim.getSurface().copyTo(floater);

//         //cv::GaussianBlur(floater, floater, cv::Size(5, 5), 0);
//         double min_val, max_val;
//         cv::minMaxLoc(floater, &min_val, &max_val);
//         max_val = std::max(fabs(max_val), fabs(min_val));
//         floater /= (2*max_val);
//         floater += 0.5;

//         cv::Mat pimmer, pimmer3;
//         floater.convertTo(pimmer, CV_8U, 255, 0);
//         pimmer = grey_frame;
//         cv::cvtColor(pimmer, pimmer3, cv::COLOR_GRAY2BGR);
    
//         hpecore::skeleton13 pose = detop.detect(pimmer3);
//         hpecore::drawSkeleton(pimmer3, pose);

        
//         //cv::normalize(floater, floater, 0, 1, cv::NORM_MINMAX);
//         //cv::namedWindow("Representation Visualisation", cv::WINDOW_NORMAL);
//         cv::imshow("Representation Visualisation", pimmer3);
//         cv::waitKey(1);
        
//         return true;
//     }

//     //asynchronous thread
//     void run() override
//     {
//         Stamp ystamp;
//         while(!Thread::isStopping()) {

//             //TODO - function for vis
//             //TODO - update on packet? or something more robust?
//             //TODO - remove vtsscaler and use packet timestamps?

//             ImageOf<PixelMono> *grey = input_grey.read(false);
//             if(grey) 
//             {
//                 yarp::cv::toCvMat(*grey).copyTo(grey_frame);
//             }

//             Bottle *gt_container = input_gt.read(false);
//             bool gt_available = (gt_container != nullptr);
//             if(gt_available) 
//             {
//                 skeleton_gt = hpecore::extractSkeletonFromYARP<Bottle>(*gt_container);
//             }

//             hpecore::jDot jv;
//             int nqs = input_events.queryunprocessed();
//             //if(nqs == 0) nqs = 1;
//             for(auto i = 0; i < nqs; i++) {
//                 const vector<AE> *q = input_events.read(ystamp);
//                 if(!q) return;

//                 for(auto &v : *q) {
//                     vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(128, 255, 0);
//                     pim.update(v.x, v.y, 0.0, v.polarity);
//                 }

//                 jv = velocity_estimator.update<vector<AE>>(*q, state.query(my_joint));
//                 jv = jv * vtsHelper::vtsscaler;
//             }
//             pim.temporalDecay(Time::now(), 1.0);
//             // pim.spatialDecay(5);

//             if(!state.poseIsInitialised() && gt_available)
//                 state.set(skeleton_gt);
//             else if(
            
//             if (state.poseIsInitialised())
//                 state.updateFromVelocity(my_joint, jv, ev::vtsHelper::deltaS(q->back().stamp, q->front().stamp));

//             if (state.poseIsInitialised())
//                 state.updateFromPosition(skeleton_gt, 0.001);
//             else
                

//             if(state.poseIsInitialised())
//                 skelwriter.write({ystamp.getTime(), input_events.queryDelayT(), state.query()});
//         }
//     }
// };

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

    cv::VideoCapture input_video;
    if(!input_video.open(video_path)) 
    {
        std::cerr << "Could not open video at: " << video_path << std::endl;
        return -1;
    }

    hpecore::writer csv_writer;
    if(!csv_writer.open(csv_path))
    {
        std::cerr << "Could not open csv at: " << csv_path << std::endl;
        return -1;
    }

    cv::Mat frame;
    //process the video
    while(true) 
    {
        input_video >> frame;
        if(frame.empty()) break;

        cv::imshow("Frame", frame);
        if(cv::waitKey() == 27) break;

        //skelwriter.write({input_video.get(cv::CAP_PROP_POS_MSEC)*1000.0, 0.0, detop.detect(frame)});

    }

    cv::destroyAllWindows();
    input_video.release();
    csv_writer.close();
    return 0;
}
