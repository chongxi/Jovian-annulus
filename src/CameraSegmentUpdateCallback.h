// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/* -*-c++-*- */

#ifndef CAMERASEGMENTUPDATECALLBACK_H
#define CAMERASEGMENTUPDATECALLBACK_H

#include <btBulletDynamicsCommon.h>

#include "CameraThresholdUpdateCallback.h"

class CameraSegmentUpdateCallback : public CameraThresholdUpdateCallback
{
  public:
    CameraSegmentUpdateCallback( btRigidBody* body,
                                 Collision_World* dynamics_world,
                                 osg::ref_ptr< osgGA::NodeTrackerManipulator > tracker,
                                 ContactNodes const& nodes,
                                 osg::Vec3f& start_dir );

    void operator()( osg::Node* node, osg::NodeVisitor* nv );
    void update( osg::Node* node );
    void update_segment_mapping( std::map<std::string, bool> new_map ) { _mapping = new_map; }

  private:
  	btVector3 _last_position;
  	std::map<std::string, bool> _mapping;
};

#endif
