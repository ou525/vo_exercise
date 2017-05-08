#include "myslam/camera.h"
namespace myslam
{
    Camera::Camera()
    {

    }

    Vector3d world2camera( const Vec3d& p_w, const SE3& T_c_w )
    {  
        return T_c_w*p_w;
    }

    Vector3d camera2world( const Vec3d& p_c, const SE3& T_c_w )
    {
        return T_c_w.inverse()*p_c;
    }

    Vector2d camera2pixel( const Vec3d& p_c )
    {
        return Vector2d(
            fx_ * p_c(0,0)/p_c(2,0) + cx_,
            fy_ * p_c(1,0)/p_c(2,0) + cy_,
        );
    }

    Vector3d pixel2camera( const Vec2d& p_p, double depth=1 )
    {
        return Vector3d(
            (p_p(0,0) - cx_ )*depth/fx_,
            (p_p(1,0) - cy_ )*depth/fy_,
            depth
        );
    }

    Vector3d pixel2world ( const Vec2d& p_p, const SE3& T_c_w )
    {
        return camera2world( pixel2camera(p_p, depth), T_c_w);
    }

    Vector2d world2pixel ( const Vec3d& p_w, const SE3 T_c_w )
    {
        return camera2pixel( world2camera(p_w, T_c_w) );
    }
}