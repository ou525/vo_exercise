#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <algorithm>
#include <boost/timer.hpp>

#include "myslam/config.h"
#include "myslam/visual_odometry.h"
#include "myslam/g2o_types.h"

namespace myslam
{
    VisualOdometry::VisualOdometry():
        state_(INITIALIZING), ref_(nullptr), curr_(nullptr), map_(new Map), num_lost_(0), num_inliers_(0)
        {
            num_of_features_    = Config::get<int>("number_of_features");
            scale_factor_       = Config::get<double>("scale_factor");
            level_pyramid_      = Config::get<int>("level_pyramid");
            match_ratio_        = Config::get<float>("match_ratio");
            max_num_lost_       = Config::get<float>("max_num_lost");
            min_inliers_        = Config::get<int>("min_inliers");
            key_frame_min_rot   = Config::get<double> ("keyframe_rotation");
            key_frame_min_trans = Config::get<double> ("keyframe_translation");
            orb_ = cv::ORB::create(num_of_features_, scale_factor_, level_pyramid_);
        }

    VisualOdometry::~VisualOdometry()
    {

    }

    bool VisualOdometry::addFrame ( Frame::Ptr frame )
    {
        int i=0;
        switch(state_)
        {
            case INITIALIZING:
            {
                state_ = OK;
                curr_ = ref_ = frame;
                // cout << "initializing " << endl;
                map_->insertKeyFrame(frame);
                // cout << "i am coming here0" << endl;
                // extract features from first frame 
                extractKeyPoints();
                // cout << "i am coming here1" << endl;
                computeDescriptors();
                // cout << "i am coming here2" << endl;
                // compute the 3d position of features in ref frame
                setRef3DPoints();
                // cout << "i am coming here3" << endl; 
                // cout << state_ << " i am coming here3" << endl;               
                break;                
            }

            case OK:
            {
                // cout << "jkdjfskkkkkkkjlsdfjlksddsjflk" << endl;
                curr_ = frame;
                extractKeyPoints();
                cout << "keypoints_curr_: " << keypoints_curr_.size() << endl; 
                computeDescriptors();
                featureMatching();
                poseEstimationPnP();
                if(checkEstimatedPose() == true) // a good estimation
                {
                    curr_->T_c_w_ = T_c_r_estimated_ * ref_->T_c_w_;  // T_c_w = T_c_r*T_r_w 
                    ref_ = curr_;
                    setRef3DPoints();
                    num_lost_ = 0;
                    if(checkKeyFrame() == true) //is a key-frame
                    {
                        addKeyFrame();
                    }
                }
                else    // bad estimation due to various reasons
                {
                    num_lost_++;
                    if(num_lost_ > max_num_lost_)
                    {
                        state_ = LOST;
                    }
                    return false;
                }
                break;
            }
            case LOST:
            {
                cout << " vo has lost " << endl;
                break;
            }
        }
        return true;
    }


    void VisualOdometry::extractKeyPoints()
    {
        // cout << "i am coming here00" << endl;
        // cv::imshow( "keypoints", curr_->color_ );
        orb_->detect( curr_->color_, keypoints_curr_);
        cout << "keypoints_curr_: " << keypoints_curr_.size() << endl;
        // cout << "i am coming here01" << endl;
    }  

    void VisualOdometry::computeDescriptors()
    {
        orb_->compute(curr_->color_, keypoints_curr_, descriptors_curr_);
    }

    void VisualOdometry::featureMatching()
    {
        // match desp_ref and desp_curr, use OpenCV's brute force match 
        vector<cv::DMatch> matches;
        cv::BFMatcher matcher (cv::NORM_HAMMING);
        matcher.match(descriptors_ref_, descriptors_curr_, matches);
        // select the best matches
        float min_dis = std::min_element(
            matches.begin(), matches.end(), [](const cv::DMatch& m1, const cv::DMatch& m2)
            {
                return m1.distance < m2.distance;
            }
        )->distance;

        feature_matches_.clear();
        for(cv::DMatch& m:matches)
        {
            if(m.distance < max<float>(min_dis*match_ratio_, 30.0))
            {
                feature_matches_.push_back(m);
            }
        }
        cout << "good matches: " << feature_matches_.size() << endl;
    }

    void VisualOdometry::setRef3DPoints()
    {
        // select the features with depth measurements
        pts_3d_ref_.clear();
        descriptors_ref_ = Mat();
        for(size_t i =0;i<keypoints_curr_.size();i++)
        {
            double d = ref_->findDepth(keypoints_curr_[i]);
            if(d > 0)
            {
                Vector3d p_cam = ref_->camera_->pixel2camera(
                    Vector2d(keypoints_curr_[i].pt.x, keypoints_curr_[i].pt.y), d
                );
                pts_3d_ref_.push_back(cv::Point3f(p_cam(0,0), p_cam(1,0), p_cam(2,0)));
                descriptors_ref_.push_back(descriptors_curr_.row(i));
            }
        }
    }

    // void VisualOdometry::poseEstimationPnP()
    // {
    //     // construct the 3d 2d observations
    //     vector<cv::Point3f> pts3d;
    //     vector<cv::Point2f> pts2d;

    //     for(cv::DMatch m:feature_matches_)
    //     {
    //         pts3d.push_back(pts_3d_ref_[m.queryIdx]);
    //         pts2d.push_back(keypoints_curr_[m.trainIdx].pt);
    //     }

    //     Mat K = ( cv::Mat_<double>(3,3)<<
    //         ref_->camera_->fx_, 0, ref_->camera_->cx_,
    //         0, ref_->camera_->fy_, ref_->camera_->cy_,
    //         0, 0, 1
    //     );
    //     Mat rvec, tvec, inliers;
    //     cv::solvePnPRansac(pts3d, pts2d, K, Mat(), rvec, tvec, false, 100, 4.0, 0.99, inliers);
    //     num_inliers_ = inliers.rows;
    //     cout << "pnp inliers: " << num_inliers_ << endl;
    //     T_c_r_estimated_ = SE3(
    //         SO3(rvec.at<double>(0,0), rvec.at<double>(1,0), rvec.at<double>(2,0)),
    //         Vector3d(tvec.at<double>(0,0), tvec.at<double>(1,0), tvec.at<double>(2,0))
    //     );
    // }

    void VisualOdometry::poseEstimationPnP()
    {
        // construct the 3d 2d observations
        vector<cv::Point3f> pts3d;
        vector<cv::Point2f> pts2d;

        for(cv::DMatch m:feature_matches_)
        {
            pts3d.push_back(pts_3d_ref_[m.queryIdx]);
            pts2d.push_back(keypoints_curr_[m.trainIdx].pt);
        }

        Mat K = ( cv::Mat_<double>(3,3)<<
            ref_->camera_->fx_, 0, ref_->camera_->cx_,
            0, ref_->camera_->fy_, ref_->camera_->cy_,
            0, 0, 1
        );
        Mat rvec, tvec, inliers;
        cv::solvePnPRansac(pts3d, pts2d, K, Mat(), rvec, tvec, false, 100, 4.0, 0.99, inliers);
        num_inliers_ = inliers.rows;
        cout << "pnp inliers: " << num_inliers_ << endl;
        T_c_r_estimated_ = SE3(
            SO3(rvec.at<double>(0,0), rvec.at<double>(1,0), rvec.at<double>(2,0)),
            Vector3d(tvec.at<double>(0,0), tvec.at<double>(1,0), tvec.at<double>(2,0))
        );

        //using bundle adjustment to optimize the pose
        typedef g2o::BlockSolver<g2o::BlockSolverTraits<6,2>> Block;
        Block::LinearSolverType* linearSolver = new g2o::LinearSolverDense<Block::PoseMatrixType>();
        Block* solver_ptr = new Block(linearSolver);
        g2o::OptimizationAlgorithmLevenberg* solver = new g2o::OptimizationAlgorithmLevenberg(solver_ptr);
        g2o::SparseOptimizer optimizer;
        optimizer.setAlgorithm(solver);

        g2o::VertexSE3Expmap* pose = new g2o::VertexSE3Expmap();
        pose->setId ( 0 );
        pose->setEstimate ( g2o::SE3Quat (
            T_c_r_estimated_.rotation_matrix(), 
            T_c_r_estimated_.translation()
        ) );
        optimizer.addVertex ( pose );

        // edges
        for ( int i=0; i<inliers.rows; i++ )
        {
            int index = inliers.at<int>(i,0);
            // 3D -> 2D projection
            EdgeProjectXYZ2UVPoseOnly* edge = new EdgeProjectXYZ2UVPoseOnly();
            edge->setId(i);
            edge->setVertex(0, pose);
            edge->camera_ = curr_->camera_.get();
            edge->point_ = Vector3d( pts3d[index].x, pts3d[index].y, pts3d[index].z );
            edge->setMeasurement( Vector2d(pts2d[index].x, pts2d[index].y) );
            edge->setInformation( Eigen::Matrix2d::Identity() );
            optimizer.addEdge( edge );
        }
    
        optimizer.initializeOptimization();
        optimizer.optimize(10);
        
        T_c_r_estimated_ = SE3 (
            pose->estimate().rotation(),
            pose->estimate().translation()
        );
    }

    bool VisualOdometry::checkEstimatedPose()
    {
        // check if the estimated pose is good 
        if(num_inliers_ < min_inliers_)
        {
            cout << "reject because inlier is too small: " << num_inliers_ << endl;
            return false;
        }

        // if the motion is too large , it is probably wrong
        Sophus::Vector6d d = T_c_r_estimated_.log();
        if(d.norm() > 5.0)
        {
            cout << "reject becaus motion is too large: " << d.norm() << endl;
            return false;
        }
        return true;
    }

    bool VisualOdometry::checkKeyFrame()
    {
        Sophus::Vector6d d = T_c_r_estimated_.log();
        Vector3d trans = d.head<3>();
        Vector3d rot = d.tail<3>();
        if(rot.norm() > key_frame_min_rot || trans.norm() > key_frame_min_trans)
            return true;
        return false;
    }

    void VisualOdometry::addKeyFrame()
    {
        cout << "adding a key-frame " << endl;
        map_->insertKeyFrame(curr_);
    }

}