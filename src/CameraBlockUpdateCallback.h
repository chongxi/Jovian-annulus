// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/* -*-c++-*- */

#ifndef CAMERABLOCKUPDATECALLBACK_H
#define CAMERABLOCKUPDATECALLBACK_H

#include <btBulletDynamicsCommon.h>

#include "CameraThresholdUpdateCallback.h"

class CameraBlockUpdateCallback : public CameraThresholdUpdateCallback
{
 public:
   CameraBlockUpdateCallback( btRigidBody *body, Collision_World *dynamics_world,
                              osg::ref_ptr< osgGA::NodeTrackerManipulator > tracker,
                              ContactNodes const &nodes, osg::Vec3f &start_dir );

   void operator()( osg::Node *node, osg::NodeVisitor *nv );
   void update( osg::Node *node );
   void update_threshold_turning( osg::Node *node );

   void
   enable_threshold_turning( bool yes_or_no )
   {
      _enable_turning = yes_or_no;
      if ( _enable_turning )
         _turning_mixture = _old_turning_mixture;
      else
         _old_turning_mixture = _turning_mixture;
   }

 protected:
   float _old_turning_mixture;
};

#endif
