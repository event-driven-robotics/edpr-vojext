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
            cv::GaussianBlur(latest_image, cv_image, cv::Size(5, 5), 0, 0);
            output_port.prepare().copy(yarp::cv::fromCvMat<PixelMono>(cv_image));
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

    // velocity and fusion
    hpecore::multiJointLatComp state;
    ev::EROS eros_handler;

    // internal data structures
    hpecore::skeleton13 skeleton_detection{0};

    cv::Size image_size;
    cv::Mat vis_image;
    cv::Mat edpr_logo;

    // parameters
    bool gpu{false};
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

        eros_handler.init(640, 480, 7, 0.3);

        // =====READ PARAMETERS=====
        gpu = rf.check("gpu") && rf.check("gpu", Value(true)).asBool();
        double procU = rf.check("pu", Value(1e-1)).asFloat64();
        double measUD = rf.check("muD", Value(1e-4)).asFloat64();
        double measUV = rf.check("muV", Value(0)).asFloat64();
        
        thF = rf.check("thF", Value(100.0)).asFloat64();
        th_period = 1/thF;

        // run python code for movenet
        if (gpu)
            int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py --gpu &");
        else
            int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py &");
        while (!yarp::os::NetworkBase::exists("/movenet/sklt:o"))
            sleep(1);
        yInfo() << "MoveEnet started correctly";
        if (!mn_handler.init(getName("/eros:o"), getName("/movenet:i"), 80))
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

        ros_node = new yarp::os::Node("/APRIL");
        if (!ros_publisher.topic("/pem/neuromorphic_camera/data"))
        {
            yError() << "Could not open ROS pose output publisher";
            return false;
        }
        else
            yInfo() << "ROS pose output publisher: OK";

        if (!publisherPort_eros.topic("/pem/neuromorphic_camera/eros"))
        {
            yError() << "Could not open ROS EROS output publisher";
            return false;
        }
        else
            yInfo() << "ROS EROS output publisher: OK";

        if (!publisherPort_evs.topic("/pem/neuromorphic_camera/evs"))
        {
            yError() << "Could not open ROS EVS output publisher";
            return false;
        }
        else
            yInfo() << "ROS EVS output publisher: OK";
        


        thread_events = std::thread([this]{ this->run_camera_interface(); });
        //thread_detection = std::thread([this]{ this->run_detection(); });

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
        cv::GaussianBlur(eros_handler.getSurface(), blurred_eros, {5, 5}, -1);
        cv::cvtColor(blurred_eros, img, CV_GRAY2BGR);
    }


    // synchronous thread
    bool updateModule() override
    {
        static cv::Mat canvas = cv::Mat(image_size, CV_8UC3);
        canvas.setTo(cv::Vec3b(0, 0, 0));

        drawEROS(canvas);

        // plot skeletons
        hpecore::drawSkeleton(canvas, state.query(), {0, 0, 255}, 3);

        if (!edpr_logo.empty())
        {
            static cv::Mat mask;
            cv::cvtColor(edpr_logo, mask, CV_BGR2GRAY);
            edpr_logo.copyTo(canvas, mask);
        }

        cv::imshow("edpr-vojext", canvas);
        return true;
    }

    // void run_detection()
    // {
    //     double latency = 0.0;
    //     hpecore::stampedPose detected_pose;
    //     double t0 = Time::now();
    //     std:vector<double> sklt_out; 

    //     while (!isStopping())
    //     {
    //         tnow = Time::now() - t0;

    //         // ---------- DETECTIONS ----------
    //         bool was_detected = false;
    //         if (use_gt)
    //         {
    //             was_detected = gt_handler.update(tnow, detected_pose);
    //         }
    //         else if (movenet)
    //         {
    //             static cv::Mat eros8;
    //             // pw_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
    //             if(vpx || noVE) pw_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
    //             if(vsf || ver || vcr) sf_velocity.queryEROS().copyTo(eros8);
    //             if(vtr) trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
    //             if(pxt) pw_trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
    //             was_detected = mn_handler.update(eros8, tnow, detected_pose);
    //         }

    //         if (was_detected && hpecore::poseNonZero(detected_pose.pose))
    //         {
    //             skeleton_detection = detected_pose.pose;
    //             latency = detected_pose.delay;
    //             if (state.poseIsInitialised())
    //                 state.updateFromPosition(skeleton_detection, detected_pose.timestamp);
    //             else
    //                 state.set(skeleton_detection, tnow);
    //         }
    //     }
    // }

    // void updateSAE()
    // {

    // }

    void run_camera_interface()
    {
        while (!isStopping())
        {
            ev::info stats = input_events.readAll(true);
            for(auto &v : input_events)
                eros_handler.update(v.x, v.y);
        }
    }

    // void run_opixels()
    // {
    //     hpecore::skeleton13_vel jv;
    //     hpecore::skeleton13_vel skel_vel = {0};
    //     ev::info event_stats = {0};
    //     double latency = 0.0;
    //     hpecore::stampedPose detected_pose;
    //     input_events.readPacket(true);
    //     double t0 = Time::now();
    //     std:vector<double> sklt_out, vel_out;
    //     double t1, t2, dt;
    //     double ts0;

    //     while (!isStopping())
    //     {
    //         double tnow = Time::now() - t0;

    //         // ---------- VELOCITY ----------
    //         // read events
    //         event_stats = input_events.readAll(false);
    //         if (event_stats.count == 0)
    //             continue;

    //         if(!started)
    //         {
    //             started = true;
    //             ts0 = event_stats.timestamp;
    //         } 

    //         // update images
    //         for (auto &v : input_events)
    //         {
    //             // if (v.p)
    //             //     vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(64, 150, 90);
    //             // else
    //             //     vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(32, 82, 50);
    //             vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(255, 255, 255);
    //         }

    //         t1 = Time::now();
    //         if(vpx || vtr || noVE) pw_velocity.update(input_events.begin(), input_events.end());
            

    //         // only update velocity if the pose is initialised
    //         if (!state.poseIsInitialised())
    //             continue;

    //         if(vpx) skel_vel = pw_velocity.query(state.query(), roiSize, 2, state.queryVelocity());
    //         if(vsf) skel_vel = sf_velocity.update(input_events.begin(), input_events.end(), state.query(), tnow);
    //         if(ver)
    //         {
    //             sf_velocity.errorToVel(input_events.begin(), input_events.end(), state.query(), state.queryVelocity(), state.queryError(), tnow);
    //             skel_vel = sf_velocity.updateOnError(state.queryVelocity(), state.queryError());
    //         }
    //         if(vcr)
    //         {
    //             sf_velocity.errorToCircle(input_events.begin(), input_events.end(), state.query(), state.queryVelocity(), state.queryError(), tnow);
    //             skel_vel = sf_velocity.updateOnError(state.queryVelocity(), state.queryError());
    //         }
    //         if(vqu)
    //         {
    //             skel_vel = q_velocity.update(input_events.begin(), input_events.end(), state.query(), event_stats.timestamp);
    //         }
    //         if(vtr) skel_vel = trip_velocity.update(input_events.begin(), input_events.end(), state.query(), event_stats.timestamp);

    //         if(pxt)
    //         {
    //             pw_trip_velocity.updateSAE(input_events.begin(), input_events.end(), event_stats.timestamp-ts0); 
    //             skel_vel = pw_trip_velocity.query(state.query(), event_stats.timestamp-ts0, roiSize, 1);
    //         }
            
    //         // yInfo() << event_stats.timestamp;
    //         // hpecore::print_skeleton(skel_vel);
    //         // this scaler was thought to be from timestamp misconversion.
    //         // instead we aren't sure why it is needed.
    //         for (int j = 0; j < 13; j++) // (F) overload * to skeleton13
    //             skel_vel[j] = skel_vel[j] * scaler;
    //         state.setVelocity(skel_vel);
    //         // if(vpx) state.updateFromVelocity(skel_vel, event_stats.timestamp);
    //         // else state.updateFromVelocity(skel_vel, event_stats.timestamp);
    //         state.updateFromVelocity(skel_vel, event_stats.timestamp);

    //         t2 = Time::now();
    //         dt = (t2 - t1) * 1e3;
    //         if(poseWr) skelwriter.write({event_stats.timestamp, dt, state.query()});
    //         if(velWr) velwriter.write({event_stats.timestamp, dt, skel_vel});

    //         // skelwriter.write({event_stats.timestamp, latency, state.query()});
    //         // velwriter.write({event_stats.timestamp, latency, skel_vel});

    //         if (ros)
    //         {
    //             // format skeleton to ros output
    //             sklt_out.clear();
    //             vel_out.clear();
    //             for (int j = 0; j < 13; j++)
    //             {
    //                 sklt_out.push_back(skeleton_detection[j].u);
    //                 sklt_out.push_back(skeleton_detection[j].v);
    //                 vel_out.push_back(skel_vel[j].u);
    //                 vel_out.push_back(skel_vel[j].v);
    //             }
    //             ros_output.timestamp = tnow;
    //             ros_output.pose = sklt_out;
    //             ros_output.velocity = vel_out;
    //             // publish data
    //             ros_publisher.prepare() = ros_output;
    //             ros_publisher.write();
    //         }
    //     }
    // }
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
