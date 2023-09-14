/*
Author: Franco Di Pietro, Arren Glover
 */

#include <yarp/cv/Cv.h>
#include <yarp/os/all.h>
#include <yarp/sig/Image.h>
#include <event-driven/core.h>
#include <event-driven/algs.h>
#include <hpe-core/utility.h>
#include <hpe-core/motion.h>
#include <hpe-core/fusion.h>
#include <hpe-core/representations.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "sim_sem_msgs/NChumanPose.h"
#include <yarp/rosmsg/sensor_msgs/Image.h>

using namespace yarp::os;
using namespace yarp::sig;
using std::vector;


class externalDetector
{
private:
    double period{0.1}, tic{0.0};
    bool waiting{false};

    BufferedPort<ImageOf<PixelMono>> output_port;
    BufferedPort<Bottle> input_port;

public:
    bool init(std::string output_name, std::string input_name, double period)
    {
        if (!output_port.open(output_name))
            return false;

        if (!input_port.open(input_name))
            return false;

        this->period = period;
        return true;
    }
    void close()
    {
        output_port.close();
        input_port.close();
    }

    bool update(const cv::Mat &latest_image, double latest_ts, hpecore::stampedPose &previous_skeleton)
    {
        // send an update if the timer has elapsed
        if ((!waiting && latest_ts - tic > period) || (latest_ts - tic > 2.0))
        {
            cv::Mat temp;
            latest_image.convertTo(temp, CV_8U);
            cv::GaussianBlur(temp, temp, {5, 5}, -1);
            output_port.prepare().copy(yarp::cv::fromCvMat<PixelMono>(temp));
            output_port.write();
            tic = latest_ts;
            waiting = true;
        }

        // read a ready data
        Bottle *mn_container = input_port.read(false);
        if (mn_container)
        {
            previous_skeleton.pose = hpecore::extractSkeletonFromYARP<Bottle>(*mn_container);
            previous_skeleton.conf = hpecore::extractConfidenceFromYARP<Bottle>(*mn_container);
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
    std::thread thread_events, thread_detection, thread_rosimg;
    ev::window<ev::AE> input_events;

    // detection handlers
    externalDetector mn_handler;
    std::string killline;

    // velocity and fusion
    hpecore::multiJointLatComp state;
    hpecore::EROS eros_handler;
    hpecore::SAE sae_handler;
    hpecore::BIN binary_handler;

    // internal data structures
    hpecore::skeleton13 skeleton_detection{0};
    hpecore::pwtripletvelocity velocitizer;

    cv::Size image_size{640, 480};
    cv::Mat edpr_logo;

    // parameters
    double p_vis{0.033}, p_img{0.2}, p_det{0.2}, p_vel{0.02};
    double tnow{0.0};
    bool vis{false};
    bool high_confidence{true};

    // ros 
    yarp::os::Node* ros_node{nullptr};
    yarp::os::Publisher<yarp::rosmsg::sim_sem_msgs::NChumanPose> ros_publisher;
    typedef yarp::os::Publisher<yarp::rosmsg::sensor_msgs::Image> ImageTopicType;
    ImageTopicType publisherPort_eros, publisherPort_evs;

public:
    bool configure(yarp::os::ResourceFinder &rf) override
    {
        // =====SET UP YARP=====
        if(rf.check("help")) {
            yInfo() << " EDPR VOJEXT HPE ";
            yInfo() << "--name <string> : name of module for YARP ports";
            yInfo() << "--vis <bool>    : open visualisation";
            yInfo() << "--f_vis <float> : visualisation rate [20]";
            yInfo() << "--f_det <float> : HPE detection rate [5]";
            yInfo() << "--f_vel <float> : HPE velocity estimation rate [50]";
            yInfo() << "--f_img <float> : ROS image output rate [3]";
            yInfo() << "--pu <float>    : KF process uncertainty [10.0]";
            yInfo() << "--muD <float>   : KF measurement uncertainty [1.0]";
            return false;
        }
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

        eros_handler.init(image_size.width, image_size.height, 7, 0.3);
        binary_handler.init(image_size.width, image_size.height);
        sae_handler.init(image_size.width, image_size.height);

        // =====READ PARAMETERS=====
        double procU = rf.check("pu", Value(10)).asFloat64();
        double measUD = rf.check("muD", Value(1)).asFloat64();
        
        vis = rf.check("vis", Value(false)).asBool();
        p_vis = 1.0/std::max(rf.check("f_vis", Value(20.0)).asFloat64(), 5.0);
        p_det = 1.0/std::max(rf.check("f_det", Value(5.0)).asFloat64(), 1.0);
        p_vel = 1.0/std::max(rf.check("f_vel", Value(50)).asFloat64(), 1.0/p_det);
        if(rf.check("f_img", Value(3.0)).asFloat64() > 0)
            p_img = 1.0/rf.check("f_img", Value(3.0)).asFloat64();
        else
            p_img = 0.0;
        

        // run python code for movenet
        int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py &");
        while (!yarp::os::NetworkBase::exists("/movenet/sklt:o"))
            sleep(1);
        yInfo() << "MoveEnet started correctly";

        FILE* pipe = popen("pgrep -f movenet_online", "r");
        killline = "kill "; killline.resize(32);
        char * k = fgets(killline.data()+5, killline.size()-5, pipe);
        yInfo() << killline;
        

        if (!mn_handler.init(getName("/eros:o"), getName("/movenet:i"), p_det))
        {
            yError() << "Could not open movenet ports";
            return false;
        }

        // ===== SET UP INTERNAL VARIABLE/DATA STRUCTURES =====

        // shared images
        edpr_logo = cv::imread("/usr/local/src/edpr-vojext/edpr_logo.png");

        // fusion
        if (!state.initialise({procU, measUD, 0.0, -1.0}))
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

        if(vis) {
            cv::namedWindow("edpr-vojext", cv::WINDOW_NORMAL);
            cv::resizeWindow("edpr-vojext", image_size);
        }

        // set-up ROS interface

        ros_node = new yarp::os::Node("/edpr_vojext");
        if (!ros_publisher.topic("/pem/neuromorphic_camera/data"))
        {
            yError() << "Could not open ROS pose output publisher";
            return false;
        }
        else
            yInfo() << "ROS pose output publisher: OK";

        if (!publisherPort_eros.topic("/isim/neuromorphic_camera/eros"))
        {
            yError() << "Could not open ROS EROS output publisher";
            return false;
        }
        else
            yInfo() << "ROS EROS output publisher: OK";

        if (!publisherPort_evs.topic("/isim/neuromorphic_camera/evs"))
        {
            yError() << "Could not open ROS EVS output publisher";
            return false;
        }
        else
            yInfo() << "ROS EVS output publisher: OK";
        


        thread_events = std::thread([this]{ this->run_camera_interface(); });
        thread_detection = std::thread([this]{ this->run_detection(); });
        if(p_img > 0.0)
            thread_rosimg = std::thread([this]{ this->run_rosimg(); });


        return true;
    }

    double getPeriod() override
    {
        // run the module as fast as possible. Only as fast as new images are
        // available and then limited by how fast OpenPose takes to run
        return p_vis;
    }

    bool interruptModule() override
    {
        // if the module is asked to stop ask the asynchronous thread to stop
        input_events.stop();
        mn_handler.close();
        thread_events.join();
        thread_detection.join();
        if(p_img > 0.0)
            thread_rosimg.join();

        int r = system(killline.c_str());
        
        return true;
    }

    bool close() override
    {
        // when the asynchronous thread is asked to stop, close ports and do other clean up
        return true;
    }

    void drawEROS_MONO(cv::Mat &img)
    {
        eros_handler.getSurface().convertTo(img, CV_8U);
        cv::GaussianBlur(img, img, {5, 5}, -1);
    }

    void drawEROS_RGB(cv::Mat img)
    {
        cv::Mat blurred_eros;
        drawEROS_MONO(blurred_eros);
        cv::cvtColor(blurred_eros, img, CV_GRAY2BGR);
    }

    void drawEVENTS_MONO(cv::Mat &img)
    {
        binary_handler.getSurface().convertTo(img, CV_8U);
    }

    void drawEVENTS_RGB(cv::Mat &img)
    {
        cv::Mat eventsmono;
        drawEVENTS_MONO(eventsmono);
        cv::cvtColor(eventsmono, img, CV_GRAY2BGR);
    }

    // synchronous thread
    bool updateModule() override
    {
        
        if(vis) {
            if (cv::getWindowProperty("edpr-vojext", cv::WND_PROP_ASPECT_RATIO) < 0) {
                stopModule();
                return false;
            }

            static bool show_eros = true;
            static cv::Mat canvas = cv::Mat(image_size, CV_8UC3, cv::Vec3b(0, 0, 0));

            if(show_eros)
                drawEROS_RGB(canvas);
            else
                drawEVENTS_RGB(canvas);

            // plot skeletons
            if (high_confidence)
                hpecore::drawSkeleton(canvas, state.query(), {0, 0, 255}, 3);

            if (!edpr_logo.empty())
            {
                static cv::Mat mask;
                cv::cvtColor(edpr_logo, mask, CV_BGR2GRAY);
                edpr_logo.copyTo(canvas, mask);
            }

            cv::imshow("edpr-vojext", canvas);
            char key = cv::waitKey(1);
            if(key == '\e')
                cv::destroyWindow("edpr-vojext");
            else if(key == 'e')
                show_eros = !show_eros; 
        }
        binary_handler.getSurface().setTo(0);

        if(!high_confidence) 
        {
            yarp::rosmsg::sim_sem_msgs::NChumanPose& ros_output = ros_publisher.prepare();
            ros_output.pose.clear();
            ros_output.pose.resize(26, -1.0);
            ros_output.velocity.clear();
            ros_output.velocity.resize(26, -1.0);
            ros_output.timestamp = tnow;
            ros_publisher.write();
        }

        return true;
    }

    void run_rosimg()
    {
        while(!isStopping())
        {
            static yarp::os::Stamp ystamp;
            ystamp.update();
            cv::Mat temp;

            // EROS
            drawEROS_MONO(temp);
            auto yarpEROS = yarp::cv::fromCvMat<yarp::sig::PixelMono>(temp);
            yarp::rosmsg::sensor_msgs::Image& rosEROS = publisherPort_eros.prepare();
            rosEROS.data.resize(yarpEROS.getRawImageSize());
            rosEROS.width = yarpEROS.width();
            rosEROS.height = yarpEROS.height();
            rosEROS.encoding = "8UC1";
            rosEROS.step = yarpEROS.getRowSize();
            rosEROS.is_bigendian = 0;
            rosEROS.header.frame_id = "eros";
            rosEROS.header.seq = ystamp.getCount();
            rosEROS.header.stamp = ystamp.getTime();
            memcpy(rosEROS.data.data(), yarpEROS.getRawImage(), yarpEROS.getRawImageSize());
            publisherPort_eros.setEnvelope(ystamp);
            publisherPort_eros.write();

            // EV image
            drawEVENTS_MONO(temp);
            auto yarpEVS = yarp::cv::fromCvMat<yarp::sig::PixelMono>(temp);
            yarp::rosmsg::sensor_msgs::Image& rosEVS = publisherPort_evs.prepare();
            rosEVS.data.resize(yarpEVS.getRawImageSize());
            rosEVS.width = yarpEVS.width();
            rosEVS.height = yarpEVS.height();
            rosEVS.encoding = "8UC1";
            rosEVS.step = yarpEVS.getRowSize();
            rosEVS.is_bigendian = 0;
            rosEVS.header.frame_id = "eventimage";
            rosEVS.header.seq = ystamp.getCount();
            rosEVS.header.stamp = ystamp.getTime();
            memcpy(rosEVS.data.data(), yarpEVS.getRawImage(), yarpEVS.getRawImageSize());
            publisherPort_evs.setEnvelope(ystamp);
            publisherPort_evs.write();

            Time::delay(p_img);
        }
    }

    void run_camera_interface()
    {
        while (!isStopping())
        {
            ev::info stats = input_events.readAll(true);
            tnow = stats.timestamp;
            for(auto &v : input_events) {
                eros_handler.update(v.x, v.y);
                binary_handler.update(v.x, v.y);
                sae_handler.update(v.x, v.y, tnow);
            }
        }
    }

    void run_detection()
    {
        while (!isStopping())
        {
            Time::delay(p_vel);
            hpecore::stampedPose detected_pose;
            bool was_detected = mn_handler.update(eros_handler.getSurface(), tnow, detected_pose);
            if(was_detected)
            {
                high_confidence = hpecore::testConfidence(detected_pose);
                if(!state.poseIsInitialised())
                    state.set(detected_pose.pose, detected_pose.timestamp);
                else
                    state.updateFromPosition(detected_pose.pose, detected_pose.timestamp);
            } 
            if(!state.poseIsInitialised() || !high_confidence) continue;

            auto jvs = velocitizer.multi_area_velocity(sae_handler.getSurface(), tnow, state.query(), 20);
            state.updateFromVelocity(jvs, tnow);

            { //ROS OUTPUT
                // format skeleton to ros output
                yarp::rosmsg::sim_sem_msgs::NChumanPose& ros_output = ros_publisher.prepare();
                hpecore::skeleton13 pose = state.query();
                hpecore::skeleton13 vel = state.queryVelocity();
                ros_output.pose.resize(26);
                ros_output.velocity.resize(26, 0.0);
                for (int j = 0; j < 13; j++)
                {
                    ros_output.pose[j*2] = pose[j].u;
                    ros_output.pose[j*2+1] = pose[j].v;
                    ros_output.velocity[j * 2] = vel[j].u;
                    ros_output.velocity[j * 2 + 1] = vel[j].v;
                }
                ros_output.timestamp = tnow;
                ros_publisher.write();
            }
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
