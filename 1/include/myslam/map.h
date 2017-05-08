#ifndef MAP_H
#define MAP_H
#include "myslam/common_include.h"
#include "frame.h"

namespace myslam
{
    class Map
    {
        public:
            typedef std::shared_ptr<Map> Ptr;
            unordered_map<unsigned long, MapPoint Ptr> map_point_;   //all landmarks
            unordered_map<unsigned long, Frame::Ptr>   keyframe_;    //all keyframes

            Map() {}

            void insertKeyFrame( Frame::Ptr frame );
            void insertMapPoint( MapPoint::Ptr map_point );
    }
}

#endif //MAP_H