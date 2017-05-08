#ifndef CONFIG_H
#define CONFIG_H
#include "myslam/common_include.h"


namespace myslam
{
    class Config
    {
        private:
            typedef std::shared_ptr<Config> Ptr;
            cv::FileStorage file_;

            Config() {} //private constructor makes a singleton

        public:
            ~Config();  //close the file when deconstructing

            //set a new config file 
            static void setParameterFile( const std::string& filename );

            //access the parameter values
            template<typename T>
            static T get( const std::string& key )
            {
                return T( Config::Config_->file_[key]);
            } 
    }
}

#endif //MAP_H