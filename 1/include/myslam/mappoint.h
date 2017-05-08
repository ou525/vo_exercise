#ifndef MAPPOINT_H
#define MAPPOINT_H

namespace myslam
{
    class Frame;
    class MapPoint
    {
        public:
            typedef std::shared_ptr<MapPoint> Ptr;
            unsigned long id_; //id of point
            Vector3d      pos_;     //position of world
            Vector2d      norm_;    //Normal of viewing direction
            Mat           descriptor_; //Descriptor for matching
            int           observed_times_;  //being observed by feature matching algo
            int           correct_times_;   //being an inliers in pose estimation

            MapPoint();
            MapPoint( long id , Vector3d position, Vector3d norm );

            //factory function
            static MapPoint::Ptr createMapPoint();         
    }
}

#endif //MAPPOINT_H