/*
Author: Franco Di Pietro, Arren Glover
 */

#include <yarp/cv/Cv.h>
#include <yarp/os/all.h>
#include <yarp/sig/Image.h>
#include <event-driven/core.h>
#include <event-driven/algs.h>
#include <hpe-core/utility.h>
#include <hpe-core/motion_estimation.h>
#include <hpe-core/fusion.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "vojext_msgs/NC_humanPose.h"
#include <yarp/rosmsg/sensor_msgs/Image.h>

using namespace yarp::os;
using namespace yarp::sig;
using std::vector;

class erosplus
{
private:

    cv::Mat surface;
    cv::Mat true_surface;
    int k_size{7};
    double absorb{0.05};
    double inject{0.01};
    double balance{100.0};

public:
    void init(int width, int height, int kernel_size = 7, double absorb = 0.05, double inject = 0.003)
    {
        k_size = kernel_size % 2 == 0 ? kernel_size + 1 : kernel_size;
        this->absorb = absorb;
        this->inject = inject;
        surface = cv::Mat(height + k_size-1, width + k_size-1, CV_64F, cv::Scalar(10));
        true_surface = surface({k_size/2, k_size/2, width, height});
    }

    void update(int u, int v)
    {
        static int half_kernel = k_size / 2;
        static cv::Rect region = {0, 0, k_size, k_size};
        static cv::Mat region_mat;
        static double nabsorb = 1.0 - absorb;
        
        region.x = u; region.y = v; region_mat = surface(region);
        double& c = surface.at<double>(v+half_kernel, u+half_kernel);
        if(c > balance*2.0) {
            region_mat *= 0.5;
        } else {
            double ca = 0.0;
            for(int x = 0; x < k_size; x++) {
                for(int y = 0; y < k_size; y++) {
                    double& cc = region_mat.at<double>(y, x);
                    ca += cc * absorb;
                    cc *= nabsorb;
                }
            }
            c += ca + balance*inject;
        }
    }

    cv::Mat& getSurface()
    {
        static cv::Mat ret;
        true_surface.convertTo(ret, CV_8U);
        return ret;
    }

};

class externalDetector
{
private:
    double period{0.1}, tic{0.0};
    bool waiting{false};

    BufferedPort<ImageOf<PixelMono>> output_port;
    BufferedPort<Bottle> input_port;

public:
    bool init(std::string output_name, std::string input_name, double rate)
    {
        if (!output_port.open(output_name))
            return false;

        if (!input_port.open(input_name))
            return false;

        period = 1.0 / rate;
        return true;
    }
    void close()
    {
        output_port.close();
        input_port.close();
    }

    bool update(cv::Mat latest_image, double latest_ts, hpecore::stampedPose &previous_skeleton)
    {
        // send an update if the timer has elapsed
        if ((!waiting && latest_ts - tic > period) || (latest_ts - tic > 2.0))
        {
            static cv::Mat cv_image;
            output_port.prepare().copy(yarp::cv::fromCvMat<PixelMono>(latest_image));
            output_port.write();
            tic = latest_ts;
            waiting = true;
        }

        // read a ready data
        Bottle *mn_container = input_port.read(false);
        if (mn_container)
        {
            previous_skeleton.pose = hpecore::extractSkeletonFromYARP<Bottle>(*mn_container);
            previous_skeleton.timestamp = tic;
            previous_skeleton.delay = latest_ts - tic;
            waiting = false;
        }

        return mn_container != nullptr;
    }
};


class VOJEXT_HPE : public RFModule
{

private:
    // event reading
    std::thread thread_events;
    std::thread thread_detection;
    ev::window<ev::AE> input_events;

    // detection handlers
    externalDetector mn_handler;
    std::string killline;

    // velocity and fusion
    hpecore::multiJointLatComp state;
    erosplus eros_handler;

    // internal data structures
    hpecore::skeleton13 skeleton_detection{0};

    cv::Size image_size{640, 480};
    cv::Mat vis_image;
    cv::Mat edpr_logo;

    // parameters
    double th_period{0.01}, thF{100.0};
    double tnow{0.0};

    // ros 
    yarp::os::Node* ros_node{nullptr};
    yarp::os::Publisher<yarp::rosmsg::NC_humanPose> ros_publisher;
    yarp::rosmsg::NC_humanPose ros_output;
    typedef yarp::os::Publisher<yarp::rosmsg::sensor_msgs::Image> ImageTopicType;
    ImageTopicType publisherPort_eros, publisherPort_evs;

public:
    bool configure(yarp::os::ResourceFinder &rf) override
    {
        // =====SET UP YARP=====
        if (!yarp::os::Network::checkNetwork(2.0))
        {
            std::cout << "Could not connect to YARP" << std::endl;
            return false;
        }

        // set the module name used to name ports
        setName((rf.check("name", Value("/edpr_vojext")).asString()).c_str());

        if (!input_events.open(getName("/AE:i")))
        {
            yError() << "Could not open events input port";
            return false;
        }

        eros_handler.init(image_size.width, image_size.height);

        // =====READ PARAMETERS=====
        double procU = rf.check("pu", Value(10)).asFloat64();
        double measUD = rf.check("muD", Value(1)).asFloat64();
        double measUV = rf.check("muV", Value(0)).asFloat64();
        
        thF = rf.check("thF", Value(100.0)).asFloat64();
        th_period = 1/thF;

        // run python code for movenet
        int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py &");
        while (!yarp::os::NetworkBase::exists("/movenet/sklt:o"))
            sleep(1);
        yInfo() << "MoveEnet started correctly";

        FILE* pipe = popen("pgrep -f movenet_online", "r");
        killline = "kill "; killline.resize(32);
        char * k = fgets(killline.data()+5, killline.size()-5, pipe);
        yInfo() << killline;
        

        if (!mn_handler.init(getName("/eros:o"), getName("/movenet:i"), 30))
        {
            yError() << "Could not open movenet ports";
            return false;
        }

        // ===== SET UP INTERNAL VARIABLE/DATA STRUCTURES =====

        // shared images
        vis_image = cv::Mat(image_size, CV_8UC3, cv::Vec3b(0, 0, 0));
        edpr_logo = cv::imread("/usr/local/src/EDPR-APRIL/edpr_logo.png");

        // fusion
        if (!state.initialise({procU, measUD, measUV, -1.0}))
        {
            yError() << "Not KF initialized";
            return false;
        }

        // ===== TRY DEFAULT CONNECTIONS =====
        Network::connect("/file/ch0dvs:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/atis3/AE:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/file/ch2GT50Hzskeleton:o", getName("/gt:i"), "fast_tcp");
        Network::connect("/movenet/sklt:o", getName("/movenet:i"), "fast_tcp");
        Network::connect("/zynqGrabber/AE:o", getName("/AE:i"), "fast_tcp");
        Network::connect(getName("/eros:o"), "/movenet/img:i", "fast_tcp");
        Network::connect("/file/atis/AE:o", getName("/AE:i"), "fast_tcp");
    

        cv::namedWindow("edpr-vojext", cv::WINDOW_NORMAL);
        cv::resizeWindow("edpr-vojext", image_size);

        // set-up ROS interface

        // ros_node = new yarp::os::Node("/VOJEXT");
        // if (!ros_publisher.topic("/pem/neuromorphic_camera/data"))
        // {
        //     yError() << "Could not open ROS pose output publisher";
        //     return false;
        // }
        // else
        //     yInfo() << "ROS pose output publisher: OK";

        // if (!publisherPort_eros.topic("/pem/neuromorphic_camera/eros"))
        // {
        //     yError() << "Could not open ROS EROS output publisher";
        //     return false;
        // }
        // else
        //     yInfo() << "ROS EROS output publisher: OK";

        // if (!publisherPort_evs.topic("/pem/neuromorphic_camera/evs"))
        // {
        //     yError() << "Could not open ROS EVS output publisher";
        //     return false;
        // }
        // else
        //     yInfo() << "ROS EVS output publisher: OK";
        


        thread_events = std::thread([this]{ this->run_camera_interface(); });
        thread_detection = std::thread([this]{ this->run_detection(); });

        return true;
    }

    double getPeriod() override
    {
        // run the module as fast as possible. Only as fast as new images are
        // available and then limited by how fast OpenPose takes to run
        return th_period;
    }

    bool interruptModule() override
    {
        // if the module is asked to stop ask the asynchronous thread to stop
        input_events.stop();
        mn_handler.close();
        thread_events.join();
        thread_detection.join();


        int r = system(killline.c_str());
        
        return true;
    }

    bool close() override
    {
        // when the asynchronous thread is asked to stop, close ports and do other clean up
        return true;
    }

    void drawEROS(cv::Mat img)
    {
        cv::Mat blurred_eros;
        //cv::GaussianBlur(eros_handler.getSurface(), blurred_eros, {1, 1}, -1);
        cv::cvtColor(eros_handler.getSurface(), img, CV_GRAY2BGR);

    }

    // synchronous thread
    bool updateModule() override
    {

        if (cv::getWindowProperty("edpr-vojext", cv::WND_PROP_ASPECT_RATIO) < 0) {
            stopModule();
            return false;
        }
            

        static cv::Mat canvas = cv::Mat(image_size, CV_8UC3);
        canvas.setTo(cv::Vec3b(0, 0, 0));

        drawEROS(canvas);

        // plot skeletons
        hpecore::drawSkeleton(canvas, skeleton_detection, {0, 0, 255}, 3);

        if (!edpr_logo.empty())
        {
            static cv::Mat mask;
            cv::cvtColor(edpr_logo, mask, CV_BGR2GRAY);
            edpr_logo.copyTo(canvas, mask);
        }

        cv::imshow("edpr-vojext", canvas);
        if(cv::waitKey(1) == '\e')
            cv::destroyWindow("edpr-vojext");
        return true;
    }

    void run_camera_interface()
    {
        while (!isStopping())
        {
            ev::info stats = input_events.readAll(true);
            tnow = stats.timestamp;
            for(auto &v : input_events) {
                eros_handler.update(v.x, v.y);

            }
        }
    }

    void run_detection()
    {
        while (!isStopping())
        {
        hpecore::stampedPose detected_pose;
        bool was_detected = mn_handler.update(eros_handler.getSurface(), yarp::os::Time::now(), detected_pose);

        if(was_detected)
            if(!state.poseIsInitialised())
                state.set(detected_pose.pose, detected_pose.timestamp);
            else
                state.updateFromPosition(detected_pose.pose, detected_pose.timestamp);
            skeleton_detection = state.query();
        }
    }

};

int main(int argc, char *argv[])
{
    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.configure(argc, argv);

    /* create the module */
    VOJEXT_HPE instance;
    return instance.runModule(rf);
}
