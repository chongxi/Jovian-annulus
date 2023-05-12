// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/* -*-c++-*- */

#ifndef CAMERAPATHUPDATECALLBACK_H
#define CAMERAPATHUPDATECALLBACK_H

#include <set>
#include <utility>
#include <vector>

#include <osg/AnimationPath>

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h>

#include <osgbDynamics/MotionState.h>

#include "CameraThresholdUpdateCallback.h"

struct PathMovementCallback : public osgbDynamics::MotionStateCallback
{
    osg::MatrixTransform* _track_node;

    PathMovementCallback( osg::MatrixTransform* trackNode );

    virtual void operator()( const btTransform& worldTrans );
};

class CameraPathUpdateCallback : public CameraThresholdUpdateCallback
{
  public:

    CameraPathUpdateCallback( btRigidBody* body, btRigidBody* sliding_body,
                              Collision_World* dynamics_world,
                              osg::ref_ptr< osgGA::NodeTrackerManipulator > tracker,
                              ContactNodes const& nodes, osg::Vec3d start_dir,
                              osg::AnimationPath* ap, double start_time );
    ~CameraPathUpdateCallback();

    void clear();
    void operator()( osg::Node* node, osg::NodeVisitor* nv );
    void useRotations( bool on_or_off );
    void setDisableCameraPathUpdateState( bool on_or_off );
    //  void home() { CameraMotionUpdateCallback::home(); _initializing = true; }
    bool motionState() const { return _disableCameraPathUpdate; }
    void update( osg::Node* node );
    void update_crossbar();
    float getClosetPoint( osg::Vec3d const& A, osg::Vec3d const& B,
                          osg::Vec3f& P, bool segmentClamp = false ) const;
    void set_auto_heading_turn_rate( float value ) { _auto_heading_turn_rate = value; }
    void home( double start_time = 0.0 ) { CameraThresholdUpdateCallback::home( start_time ); _start_time = start_time; }
  private:
    void getInterpolatedControlPoint( double& time,
                                      osg::Vec3f pos,
                                      osg::AnimationPath::ControlPoint& cp ) const;

  protected:

    btRigidBody* _sliding_body;
    btGeneric6DofConstraint* _constraint;
    double _firstTime, _start_time, _deltaTime, _timeIncrement, _minimum_distance;
    bool _averaging, _flip_tangent, _disableCameraPathUpdate;
    osg::AnimationPath* _orig_ap;
    osg::ref_ptr< osg::AnimationPath > _ap;
    osg::AnimationPath::ControlPoint _cp_last, _cp_now;
    osg::Vec3d _pt_last, _pt_now, _tangent, _start_dir;
    float _auto_heading_turn_rate;
};

#endif
