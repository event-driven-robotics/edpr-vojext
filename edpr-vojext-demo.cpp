/*
Author: Franco Di Pietro, Arren Glover
 */

#include <yarp/cv/Cv.h>
#include <yarp/os/all.h>
#include <yarp/sig/Image.h>
#include <event-driven/core.h>
#include <hpe-core/utility.h>
#include <hpe-core/motion_estimation.h>
#include <hpe-core/fusion.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "april_msgs/NC_humanPose.h"
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

class delayedGT
{
private:
    bool delay{false};
    double rate{10};
    BufferedPort<Bottle> input_port;
    hpecore::stampedPose internal{0.0, -1.0, {0}};

public:
    bool init(std::string input_name, double rate, bool delay)
    {
        if (!input_port.open(input_name))
            return false;

        this->delay = delay;
        this->rate = rate;
        return true;
    }
    void close()
    {
        input_port.close();
    }

    bool update(double latest_ts, hpecore::stampedPose &previous_skeleton)
    {
        Bottle *gt_container = input_port.read(false);
        if (gt_container && (rate * (latest_ts - internal.timestamp) > 1.0))
        {
            // if we have delay set the previous result to be returned
            if (delay)
                previous_skeleton = internal;

            // grab the new skeleton and set the timestamp to now
            internal.pose = hpecore::extractSkeletonFromYARP<Bottle>(*gt_container);
            internal.timestamp = latest_ts;

            // if we don't delay set the current result to be returned
            if (!delay)
                previous_skeleton = internal;

            // the delay is the difference between now and returned timestamp
            previous_skeleton.delay = latest_ts - previous_skeleton.timestamp;
            return true;
        }
        return false;
    }
};

class APRIL_HPE : public RFModule
{

private:
    // event reading
    std::thread asynch_thread;
    std::thread asynch_thread_detection;
    ev::window<ev::AE> input_events;

    // detection handlers
    externalDetector mn_handler;
    delayedGT gt_handler;

    // velocity and fusion
    hpecore::pwvelocity pw_velocity;
    hpecore::surfacedVelocity sf_velocity;
    hpecore::queuedVelocity q_velocity;
    hpecore::tripletVelocity trip_velocity;
    hpecore::pwTripletVelocity pw_trip_velocity;
    hpecore::multiJointLatComp state;
    // hpecore::stateEstimator state;

    // internal data structures
    hpecore::skeleton13 skeleton_gt{0};
    hpecore::skeleton13 skeleton_detection{0};

    cv::Size image_size;
    cv::Mat vis_image;
    cv::Mat edpr_logo;

    // recording results
    hpecore::writer skelwriter, velwriter;
    cv::VideoWriter output_video;

    // parameters
    bool movenet{false}, use_gt{false};
    int detF{10}, roiSize{20};
    double scaler{12.5};
    bool alt_view{false}, pltVel{false}, pltDet{false}, pltTra{false}, gpu{false}, ros{false}, visSAE{false};
    bool vpx{false}, vsf{false}, ver{false}, vcr{false}, vqu{false}, vtr{false}, pxt{false}, noVE{false};
    bool latency_compensation{true}, delay{false};
    double th_period{0.01}, thF{100.0};
    bool dhp19{false};
    bool pltRoi{false};
    cv::Scalar colors[13] = {{0, 0, 180}, {0, 180, 0}, {0, 0, 180},
                            {180, 180, 0}, {180, 0, 180}, {0, 180, 180},
                            {120, 0, 180}, {120, 180, 0}, {0, 120, 180},
                            {120, 120, 180}, {120, 180, 120}, {120, 120, 180}, {120, 120, 120}};
    bool started{false};
    double tnow;
    bool poseWr{false}, velWr{false};

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
        setName((rf.check("name", Value("/edpr_april")).asString()).c_str());

        if (!input_events.open(getName("/AE:i")))
        {
            yError() << "Could not open events input port";
            return false;
        }

        // =====READ PARAMETERS=====
        movenet = rf.check("movenet") && rf.check("movenet", Value(true)).asBool();
        use_gt = !movenet ? true : false;
        delay = rf.check("delay") && rf.check("delay", Value(true)).asBool();
        alt_view = rf.check("alt_view") && rf.check("alt_view", Value(true)).asBool();
        pltDet = rf.check("pltDet") && rf.check("pltDet", Value(true)).asBool();
        pltTra = rf.check("pltTra") && rf.check("pltTra", Value(true)).asBool();
        pltRoi = rf.check("pr") && rf.check("pr", Value(true)).asBool();
        gpu = rf.check("gpu") && rf.check("gpu", Value(true)).asBool();
        ros = rf.check("ros") && rf.check("ros", Value(true)).asBool();
        detF = rf.check("detF", Value(10)).asInt32();
        dhp19 = rf.check("dhp19") && rf.check("dhp19", Value(true)).asBool();
        image_size = cv::Size(rf.check("w", Value(640)).asInt32(),
                              rf.check("h", Value(480)).asInt32());
        roiSize = rf.check("roi", Value(20)).asInt32();
        double procU = rf.check("pu", Value(1e-1)).asFloat64();
        double measUD = rf.check("muD", Value(1e-4)).asFloat64();
        double measUV = rf.check("muV", Value(0)).asFloat64();
        latency_compensation = rf.check("use_lc") && rf.check("use_lc", Value(true)).asBool();
        double lc = latency_compensation ? 1.0 : 0.0;
        scaler = rf.check("sc", Value(12.5)).asFloat64();
        visSAE = rf.check("sae") && rf.check("sae", Value(true)).asBool();
        thF = rf.check("thF", Value(100.0)).asFloat64();
        th_period = 1/thF;

        // pltDet = true;
        pltTra = true;

        // ===== SELECT VELOCITY ESTIMATION METHOD =====
        std::string method = rf.check("ve", Value("")).asString();
        if(!method.compare("px"))
        {
            vpx = true;
            scaler = 8;
            yInfo() << "Velocity estimation method = Pixel-wise";
        }
        else if(!method.compare("surf"))
        {
            vsf = true;
            scaler = 2;
            yInfo() << "Velocity estimation method = Past surfaces";
        }
        else if(!method.compare("err"))
        {
            ver = true;
            scaler = 2;
            yInfo() << "Velocity estimation method = Error to previous velocity";
        }
        else if(!method.compare("circle"))
        {
            vcr = true;
            scaler = 1;
            yInfo() << "Velocity estimation method = Error to observation circle";
        }    
        else if(!method.compare("q"))
        {
            vqu = true;
            scaler = 30;
            yInfo() << "Velocity estimation method = Queues of events";
        }
        else if(!method.compare("trip"))
        {
            vtr = true;
            scaler = 1;
            // roiSize = 12;
            yInfo() << "Velocity estimation method = Triplet";
        }
        else if(!method.compare("pxt"))
        {
            pxt = true;
            scaler = 1.5;
            // roiSize = 32;
            yInfo() << "Velocity estimation method = Pixel-wise Triplet";
        }
        if(!method.length())
        {
            noVE = true;
            yInfo() << "Velocity estimation method = NONE";
        }

        if(dhp19)
        {
            image_size = cv::Size(346, 260);
            roiSize = 12;
        }

        // ===== SET UP DETECTOR METHOD =====
        if (use_gt)
        {
            if (!gt_handler.init(getName("/gt:i"), detF, delay))
            {
                yError() << "Could not open input port";
                return false;
            }
        }

        if (movenet)
        {
            // run python code for movenet
            if (gpu)
                int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py --gpu &");
            else
                int r = system("python3 /usr/local/src/hpe-core/example/movenet/movenet_online.py &");
            while (!yarp::os::NetworkBase::exists("/movenet/sklt:o"))
                sleep(1);
            yInfo() << "MoveEnet started correctly";
            if (!mn_handler.init(getName("/eros:o"), getName("/movenet:i"), detF))
            {
                yError() << "Could not open movenet ports";
                return false;
            }
        }

        // ===== SET UP INTERNAL VARIABLE/DATA STRUCTURES =====

        // shared images
        vis_image = cv::Mat(image_size, CV_8UC3, cv::Vec3b(0, 0, 0));
        edpr_logo = cv::imread("/usr/local/src/EDPR-APRIL/edpr_logo.png");
        
        //velocity estimation
        pw_velocity.setParameters(image_size, 7, 0.3, 0.01);
        sf_velocity.setParameters(roiSize, 2, 8, 1000, image_size);
        q_velocity.setParameters(roiSize, 2, 8, 1000);
        trip_velocity.setParameters(roiSize, 1, image_size);
        pw_trip_velocity.setParameters(roiSize, 1, image_size);

        // fusion
        if (!state.initialise({procU, measUD, measUV, lc}))
        {
            yError() << "Not KF initialized";
            return false;
        }

        // ===== SET UP RECORDING =====
        if (rf.check("filepath"))
        {
            std::string filepath = rf.find("filepath").asString();
            if (skelwriter.open(filepath))
            {
                yInfo() << "saving data to:" << filepath;
                poseWr = true;
            }
        }
        if (rf.check("velpath"))
        {
            std::string velpath = rf.find("velpath").asString();
            if (velwriter.open(velpath))
            {
                yInfo() << "saving velocity data to:" << velpath;
                velWr = true;
            }
        }

        if (rf.check("v"))
        {
            std::string videopath = rf.find("v").asString();
            if (!output_video.open(videopath,
                                   cv::VideoWriter::fourcc('H', '2', '6', '4'),
                                   (int)(thF),
                                   image_size,
                                   true))
            {
                yError() << "Could not open video writer!!";
                return false;
            }
        }

        // ===== TRY DEFAULT CONNECTIONS =====
        Network::connect("/file/ch0dvs:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/atis3/AE:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/file/ch2GT50Hzskeleton:o", getName("/gt:i"), "fast_tcp");
        Network::connect("/movenet/sklt:o", getName("/movenet:i"), "fast_tcp");
        Network::connect("/zynqGrabber/AE:o", getName("/AE:i"), "fast_tcp");
        Network::connect(getName("/eros:o"), "/movenet/img:i", "fast_tcp");
        Network::connect("/file/atis/AE:o", getName("/AE:i"), "fast_tcp");
        

        // * DPH19
        Network::connect("/file/ch3dvs:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/file/ch3GTskeleton:o", getName("/gt:i"), "fast_tcp");

        cv::namedWindow("edpr-april", cv::WINDOW_NORMAL);
        cv::resizeWindow("edpr-april", image_size);

        if(visSAE)
        {
            cv::namedWindow("SAE-positive", cv::WINDOW_NORMAL);
            cv::resizeWindow("SAE-positive", image_size);
            cv::namedWindow("SAE-negative", cv::WINDOW_NORMAL);
            cv::resizeWindow("SAE-negative", image_size);
        }

        // set-up ROS interface
        if (ros)
        {
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
        }


        asynch_thread = std::thread([this]{ this->run_opixels(); });
        asynch_thread_detection = std::thread([this]{ this->run_detection(); });

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
        skelwriter.close();
        velwriter.close();
        output_video.release();
        asynch_thread.join();
        asynch_thread_detection.join();
        return true;
    }

    bool close() override
    {
        // when the asynchronous thread is asked to stop, close ports and do other clean up
        return true;
    }

    void drawEROS(cv::Mat img)
    {
        cv::Mat eros8;
        if(vpx || noVE) pw_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
        // if(vsf || ver || vcr) sf_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
        if(vsf || ver || vcr) sf_velocity.queryEROS().copyTo(eros8);
        if(vtr) trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
        if(pxt) pw_trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
        cv::cvtColor(eros8, img, cv::COLOR_GRAY2BGR);
        cv::GaussianBlur(img, img, cv::Size(5, 5), 0, 0);
    }

    void drawSAE()
    {
        cv::Mat saep8, saen8;
        cv::Mat SAE_vis, SAEout;
        // positive
        pw_trip_velocity.querySAEP().copyTo(saep8);
        cv::normalize(saep8, SAE_vis, 0, 1, cv::NORM_MINMAX);
        SAE_vis.convertTo(SAEout, CV_32F, 1);
        cv::imshow("SAE-positive", SAEout);
        // negative
        pw_trip_velocity.querySAEN().copyTo(saen8);
        cv::normalize(saen8, SAE_vis, 0, 1, cv::NORM_MINMAX);
        SAE_vis.convertTo(SAEout, CV_32F, 1);
        cv::imshow("SAE-negative", SAEout);
    }

    void drawROI(cv::Mat img)
    {
        for(int i=0; i<13 ; i++)
        {
            float cx = state.query()[i].u;
            float cy = state.query()[i].v;
            cv::Point2d p1(cx-roiSize/2, cy-roiSize/2);
            cv::Point2d p2(cx+roiSize/2, cy+roiSize/2);
            cv::rectangle(img, cv::Rect(p1, p2), colors[i], 1);
            
        }
    }


    // synchronous thread
    bool updateModule() override
    {
        static cv::Mat canvas = cv::Mat(image_size, CV_8UC3);
        canvas.setTo(cv::Vec3b(0, 0, 0));

        if(started && visSAE) drawSAE();
        // plot the image
        // check if we plot events or alternative (PIM or EROS)
        if (alt_view)
            drawEROS(canvas);
            // drawSAE(canvas);
        else // events
            vis_image.copyTo(canvas);
        if(pltRoi)
            drawROI(canvas);

        if (ros)
        {
            // publish images using ROS
            // EROS
            static cv::Mat cvEROS = cv::Mat(image_size, CV_8UC3);
            cvEROS.setTo(cv::Vec3b(0, 0, 0));
            drawEROS(cvEROS);
            auto yarpEROS = yarp::cv::fromCvMat<yarp::sig::PixelRgb>(cvEROS);
            yarp::rosmsg::sensor_msgs::Image& rosEROS = publisherPort_eros.prepare();
            rosEROS.data.resize(yarpEROS.getRawImageSize());
            rosEROS.width = yarpEROS.width();
            rosEROS.height = yarpEROS.height();
            memcpy(rosEROS.data.data(), yarpEROS.getRawImage(), yarpEROS.getRawImageSize());
            publisherPort_eros.write();
            // EV image
            static cv::Mat cvEVS = cv::Mat(image_size, CV_8UC3);
            cvEVS.setTo(cv::Vec3b(0, 0, 0));
            vis_image.copyTo(cvEVS);
            auto yarpEVS = yarp::cv::fromCvMat<yarp::sig::PixelRgb>(cvEVS);
            yarp::rosmsg::sensor_msgs::Image& rosEVS = publisherPort_evs.prepare();
            rosEVS.data.resize(yarpEVS.getRawImageSize());
            rosEVS.width = yarpEVS.width();
            rosEVS.height = yarpEVS.height();
            memcpy(rosEVS.data.data(), yarpEVS.getRawImage(), yarpEVS.getRawImageSize());
            publisherPort_evs.write();
        }

        vis_image.setTo(cv::Vec3b(0, 0, 0));

        // plot skeletons
        if (pltDet)
            hpecore::drawSkeleton(canvas, skeleton_detection, {255, 0, 0}, 3);
        if (pltTra)
            hpecore::drawSkeleton(canvas, state.query(), {0, 0, 255}, 3);

        if (pltVel)
        {
            hpecore::skeleton13_vel jv = state.queryVelocity();
            for (int j = 0; j < 13; j++) // (F) overload * to skeleton13
                jv[j] = jv[j] * 0.1;

            hpecore::drawVel(canvas, state.query(), jv, {255, 255, 102}, 2);
        }

        if (!edpr_logo.empty())
        {
            static cv::Mat mask;
            cv::cvtColor(edpr_logo, mask, CV_BGR2GRAY);
            edpr_logo.copyTo(canvas, mask);
        }

        if (output_video.isOpened() && started)
            output_video << canvas;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << scaler;
        std::string mystring = ss.str();

        // cv::putText(canvas, //target image
        //     mystring, //text
        //     cv::Point(canvas.cols - 60, canvas.rows - 30), //top-left position
        //     cv::FONT_HERSHEY_DUPLEX,
        //     0.6,
        //     CV_RGB(150, 150, 150), //font color
        //     2);

        cv::imshow("edpr-april", canvas);
        char key_pressed = cv::waitKey(10);
        if (key_pressed > 0)
        {
            switch (key_pressed)
            {
            case 'v':
                pltVel = !pltVel;
                break;
            case 'd':
                pltDet = !pltDet;
                break;
            case 'e':
                alt_view = !alt_view;
                break;
            case 't':
                pltTra = !pltTra;
                break;
            case 'r':
                pltRoi = !pltRoi;
                break;
            case ',':
                if(roiSize>0)
                    roiSize--;
                sf_velocity.setParameters(roiSize, 2, 8, 1000, image_size);
                q_velocity.setParameters(roiSize, 2, 8, 1000);
                break;
            case '.':
                if(roiSize<image_size.height/2)
                    roiSize++;
                sf_velocity.setParameters(roiSize, 2, 8, 1000, image_size);
                q_velocity.setParameters(roiSize, 2, 8, 1000);
                break;
            case '[':
                if(scaler>0)
                    scaler-=0.5;
                break;
            case ']':
                if(scaler<30)
                    scaler+=0.5;
                break;
            case '\e':
                stopModule();
                break;
            }
        }
        return true;
    }

    void run_detection()
    {
        double latency = 0.0;
        hpecore::stampedPose detected_pose;
        double t0 = Time::now();
        std:vector<double> sklt_out; 

        while (!isStopping())
        {
            tnow = Time::now() - t0;

            // ---------- DETECTIONS ----------
            bool was_detected = false;
            if (use_gt)
            {
                was_detected = gt_handler.update(tnow, detected_pose);
            }
            else if (movenet)
            {
                static cv::Mat eros8;
                // pw_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
                if(vpx || noVE) pw_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
                if(vsf || ver || vcr) sf_velocity.queryEROS().copyTo(eros8);
                if(vtr) trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
                if(pxt) pw_trip_velocity.queryEROS().convertTo(eros8, CV_8U, 255);
                was_detected = mn_handler.update(eros8, tnow, detected_pose);
            }

            if (was_detected && hpecore::poseNonZero(detected_pose.pose))
            {
                skeleton_detection = detected_pose.pose;
                latency = detected_pose.delay;
                if (state.poseIsInitialised())
                    state.updateFromPosition(skeleton_detection, detected_pose.timestamp);
                else
                    state.set(skeleton_detection, tnow);
            }
        }
    }

    // void updateSAE()
    // {

    // }

    void run_opixels()
    {
        hpecore::skeleton13_vel jv;
        hpecore::skeleton13_vel skel_vel = {0};
        ev::info event_stats = {0};
        double latency = 0.0;
        hpecore::stampedPose detected_pose;
        input_events.readPacket(true);
        double t0 = Time::now();
        std:vector<double> sklt_out, vel_out;
        double t1, t2, dt;
        double ts0;

        while (!isStopping())
        {
            double tnow = Time::now() - t0;

            // ---------- VELOCITY ----------
            // read events
            event_stats = input_events.readAll(false);
            if (event_stats.count == 0)
                continue;

            if(!started)
            {
                started = true;
                ts0 = event_stats.timestamp;
            } 

            // update images
            for (auto &v : input_events)
            {
                // if (v.p)
                //     vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(64, 150, 90);
                // else
                //     vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(32, 82, 50);
                vis_image.at<cv::Vec3b>(v.y, v.x) = cv::Vec3b(255, 255, 255);
            }

            t1 = Time::now();
            if(vpx || vtr || noVE) pw_velocity.update(input_events.begin(), input_events.end());
            

            // only update velocity if the pose is initialised
            if (!state.poseIsInitialised())
                continue;

            if(vpx) skel_vel = pw_velocity.query(state.query(), roiSize, 2, state.queryVelocity());
            if(vsf) skel_vel = sf_velocity.update(input_events.begin(), input_events.end(), state.query(), tnow);
            if(ver)
            {
                sf_velocity.errorToVel(input_events.begin(), input_events.end(), state.query(), state.queryVelocity(), state.queryError(), tnow);
                skel_vel = sf_velocity.updateOnError(state.queryVelocity(), state.queryError());
            }
            if(vcr)
            {
                sf_velocity.errorToCircle(input_events.begin(), input_events.end(), state.query(), state.queryVelocity(), state.queryError(), tnow);
                skel_vel = sf_velocity.updateOnError(state.queryVelocity(), state.queryError());
            }
            if(vqu)
            {
                skel_vel = q_velocity.update(input_events.begin(), input_events.end(), state.query(), event_stats.timestamp);
            }
            if(vtr) skel_vel = trip_velocity.update(input_events.begin(), input_events.end(), state.query(), event_stats.timestamp);

            if(pxt)
            {
                pw_trip_velocity.updateSAE(input_events.begin(), input_events.end(), event_stats.timestamp-ts0); 
                skel_vel = pw_trip_velocity.query(state.query(), event_stats.timestamp-ts0, roiSize, 1);
            }
            
            // yInfo() << event_stats.timestamp;
            // hpecore::print_skeleton(skel_vel);
            // this scaler was thought to be from timestamp misconversion.
            // instead we aren't sure why it is needed.
            for (int j = 0; j < 13; j++) // (F) overload * to skeleton13
                skel_vel[j] = skel_vel[j] * scaler;
            state.setVelocity(skel_vel);
            // if(vpx) state.updateFromVelocity(skel_vel, event_stats.timestamp);
            // else state.updateFromVelocity(skel_vel, event_stats.timestamp);
            state.updateFromVelocity(skel_vel, event_stats.timestamp);

            t2 = Time::now();
            dt = (t2 - t1) * 1e3;
            if(poseWr) skelwriter.write({event_stats.timestamp, dt, state.query()});
            if(velWr) velwriter.write({event_stats.timestamp, dt, skel_vel});

            // skelwriter.write({event_stats.timestamp, latency, state.query()});
            // velwriter.write({event_stats.timestamp, latency, skel_vel});

            if (ros)
            {
                // format skeleton to ros output
                sklt_out.clear();
                vel_out.clear();
                for (int j = 0; j < 13; j++)
                {
                    sklt_out.push_back(skeleton_detection[j].u);
                    sklt_out.push_back(skeleton_detection[j].v);
                    vel_out.push_back(skel_vel[j].u);
                    vel_out.push_back(skel_vel[j].v);
                }
                ros_output.timestamp = tnow;
                ros_output.pose = sklt_out;
                ros_output.velocity = vel_out;
                // publish data
                ros_publisher.prepare() = ros_output;
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
    APRIL_HPE instance;
    return instance.runModule(rf);
}
