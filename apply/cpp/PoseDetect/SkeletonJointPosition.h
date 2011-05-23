#ifndef SKELETONJOINTPOSITION_H_INCLUDE
#define SKELETONJOINTPOSITION_H_INCLUDE

#include <opencv/cv.h>

#include <XnCppWrapper.h>

class SkeletonJointPosition
{
public:

  SkeletonJointPosition( IplImage* camera, xn::SkeletonCapability& skelton,
    xn::DepthGenerator& depth, XnUserID player )
    :camera_(camera), skelton_(skelton), depth_(depth), player_(player)
  {
  }

  XnSkeletonJointPosition GetJointPosition(XnSkeletonJoint eJoint)
  {
    XnSkeletonJointPosition Joint;
    skelton_.GetSkeletonJointPosition(player_, eJoint, Joint);
    return Joint;
  }

protected:

  IplImage* camera_;
  xn::SkeletonCapability& skelton_;
  xn::DepthGenerator& depth_;
  XnUserID player_;
};

#endif // #ifndef SKELETONJOINTPOSITION_H_INCLUDE
