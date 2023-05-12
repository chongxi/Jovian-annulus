// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015

#include <algorithm>
#include <cmath>
using std::min;
using std::max;
#include <iostream>

#include <boost/algorithm/clamp.hpp>

#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Quat>
#include <osg/io_utils>

#include "CameraSegmentUpdateCallback.h"

CameraSegmentUpdateCallback::CameraSegmentUpdateCallback( btRigidBody* body,
                                                          Collision_World* dynamics_world,
                                                          osg::ref_ptr< osgGA::NodeTrackerManipulator >tracker,
                                                          ContactNodes const& nodes,
                                                          osg::Vec3f& start_dir ):
    CameraThresholdUpdateCallback( body, dynamics_world, tracker, nodes, start_dir )
{}

void
CameraSegmentUpdateCallback::operator()( osg::Node* node, osg::NodeVisitor* nv )
{
    traverse( node, nv );
    if ( _initializing )
    {
        const osg::FrameStamp* frameStamp = nv->getFrameStamp();
        if ( frameStamp )
        {
            _initializing = false;
            _last_unwrapped_angle = acos( osg::Vec3f( 1, 0, 0 ) * _start_dir );
            _last_position = _body->getWorldTransform().getOrigin();
        }
    }
    else
    {
        Motion_Data* md = ( Motion_Data* )_tracker->getUserData();
        update( node );
        md->motion.set( 0.f, 0.f, 0.f );
    }
}


void
CameraSegmentUpdateCallback::update( osg::Node* node )
{
    if ( !_traversed )
    {
        osg::Vec3d up, forward;
        osg::Vec3d x_axis( 1, 0, 0 );
        int sign;

        _traversed = true;
        Motion_Data* md = ( Motion_Data* )_tracker->getUserData();
        if ( ! _first && md->motion.length() > _minimum_velocity_thresold )
        {
            _first = true;
            _startPos = _body->getWorldTransform();
        }

        osg::MatrixTransform* pat = dynamic_cast< osg::MatrixTransform* >( node );

        osg::Vec3d vec = md->motion;

        // Find angle to transform ball coordinate system to front
        osg::Matrixd ball_to_camera = compute_ball_to_camera_matrix( up, forward, sign );
        osg::Vec3d v_fwd = ball_to_camera * vec;
        //v_fwd.normalize();
        osg::Vec3d x_axis_ws = ball_to_camera * x_axis; // X in world/camera space
        float flip = 1.0f;

        btVector3 current_position = _body->getWorldTransform().getOrigin();

        float weight_max = 0.f;
        double ah_angular_rate = 0;
        std::vector<double> weights;
        std::vector<Segment_Node const*> valid_segments;

        // Compute the weighting for the various segments
        if ( _contact_nodes.s_nodes.size() > 0 )
        {
            osg::Vec3f world_pt( current_position[0],
                                 current_position[1],
                                 current_position[2] );

            for ( std::map< std::string, Segment_Node const* >::const_iterator it = _contact_nodes.s_nodes.begin();
                    it != _contact_nodes.s_nodes.end() && _mapping[ it->second->base_name ]; it++ )
            {
                osg::Vec3f xform_pt = world_pt * ( *it ).second->local_to_world;
                float weight = 0.f;
                float dist;
                if ( xform_pt.x() < 0.f || xform_pt.x() > ( *it ).second->direction.length() )
                {
                    osg::Vec3f end_pt( 0, 0, 0 );
                    // We know the point is beyond an endpoint, so just see if it's
                    // past the second.
                    if ( ( *it ).second->direction.x() < 0 )
                    {
                        if ( xform_pt.x() < 0.f )
                            end_pt = ( *it ).second->direction;
                    }
                    else
                    {
                        if ( xform_pt.x() > 0.f )
                            end_pt = ( *it ).second->direction;
                    }
                    dist = ( xform_pt - end_pt ).length();
                }
                else
                    dist = fabs( xform_pt.y() );

                if ( dist < ( *it ).second->outer )
                {
                    valid_segments.push_back( ( *it ).second );
                    if ( dist < ( *it ).second->inner )
                        weight = 1.f;
                    else
                        weight = 1.f - ( ( dist - ( *it ).second->inner ) / ( ( *it ).second->outer - ( *it ).second->inner ) );
                    //std::cout << " " << ( *it ).first << " - " << xform_pt << ", " << weight << ", " << dist << std::endl;
                    weight_max = std::max( weight, weight_max );
                    weights.push_back( weight );
                }
            }
        }

        //weight_max *= ( 1.f - _turning_mixture );

        //std::cout << " ^ " << md->yaw << ", <" << vec << "> " << std::endl;
        //std::cout << " ^^ <" << forward << ">, <" << v_fwd << ">" << std::endl;
        double blended_angle = _last_unwrapped_angle;
        osg::Vec3d vv, tv;
        double d_angle = 0.0;
        double ratio = _plot.x_vals().back();
        double blended_dangle = 0.0;
        osg::Vec3d ahv, ttv;
        //std::cout << "  " << _turning_mixture << ", " << blended_angle  << std::endl;

        if ( md->motion.length() > _minimum_velocity_thresold )
        {

            if ( _turning_mixture > 0.f )
            {
                if ( md->roll == 0.f )
                    ratio = _plot.x_vals().back();
                else
                    // Use -pitch as forward is negative pitch
                    ratio = -md->pitch / md->roll;

                _ratio_values[0] = ratio;
            }

            _euler_vector = osg::Vec3d( md->pitch, md->roll, md->yaw );

            if ( _use_input_heading_smoothing )
            {
                if ( _input_ratio.full() )
                {
                    // Remove first element from the accumulated value
                    _input_smoothing_val -= _input_ratio.front();
                    _input_ratio.push_back( _euler_vector );
                    _input_smoothing_val += _input_ratio.back();
                }
                else
                {
                    // Filling the buffer
                    _input_ratio.push_back( _euler_vector );
                    _input_smoothing_val += _input_ratio.back();
                }

                _euler_vector = _input_smoothing_val / _input_ratio.size();
            }
            else
            {
                _input_ratio.push_back( _euler_vector );
            }

            if ( _turning_mixture < 1.f )
            {
                ahv = computeTurningVector( vec, ball_to_camera, up, ( 1.f - weight_max ) * _euler_vector.z() );
            }

            if ( _turning_mixture > 0.f )
            {
                if ( _euler_vector.y() == 0.f )
                    ratio = _plot.x_vals().back();
                else
                    // Use -pitch as forward is negative pitch
                    ratio = -_euler_vector.x() / _euler_vector.y();

                _ratio_values[2] = ratio;
                ttv = computeThresholdVector( ratio, vec, ball_to_camera, up, md, &d_angle );
                //d_angle *= ( 1.f - weight_max );
                // Scale threshold vector by cos( theta ), so that translation is
                // minimized with a stronger roll component.
                osg::Vec3d v = vec;
                v.normalize();
                double cos_theta = v * x_axis;
                ttv *= fabs( cos_theta );
            }

            smooth_velocities( ttv, ahv );

            tv = ttv * _turning_mixture + ahv * ( 1.f - _turning_mixture );
            //tv *= ( 1.f - weight_max );
            //d_angle *= ( 1.f - weight_max );
            //std::cout << "  *  " << d_angle << ", <" << ttv << ">" << ", <" << ahv << ">" << ", <" << tv << ">" << std::endl;

            // Compute any segment-based turning here
            if ( valid_segments.size() > 0 )
            {
                osg::Vec3f pred_dir = forward + tv;
                std::vector<double> rates;
                pred_dir.normalize();

                for ( std::vector< Segment_Node const* >::iterator it = valid_segments.begin();
                        it != valid_segments.end(); it++ )
                {
                    // Both norm_direction and pred_dir are in world space and normalized
                    double cos_theta = ( *it )->norm_direction * pred_dir;
                    double angle = acos( cos_theta );
                    osg::Vec3d cross = pred_dir ^ ( *it )->norm_direction ;
                    if ( std::abs( angle ) > 1.57079 )
                    {
                        cos_theta = ( -( *it )->norm_direction ) * pred_dir;
                        cross = pred_dir ^ ( -( *it )->norm_direction );
                    }

                    cross.normalize();
                    sign = ( up * cross ) > 0 ? -1 : 1;

                    // Clamp to ensure the dot product is in range
                    cos_theta = boost::algorithm::clamp( cos_theta, -1.0, 1.0 );

                    double unwrapped = WrapPosNegPI( sign * acos( cos_theta ) );
                    rates.push_back( unwrapped );
                }

                for ( size_t i = 0; i < rates.size(); i++ )
                {
                    // We're not actually smoothing as we do with the other parameters,
                    // but reducing the turning rate so that the computed rate is achived
                    // over the requested interval (which we turn from ms to number of frames)
                    if ( _use_segment_heading_smoothing )
                        ah_angular_rate += ( ( weights[i] * rates[i] ) / ( float )( _segment_rate.size() ) );
                    else
                        ah_angular_rate += ( weights[i] * rates[i] );
                }

            }

            if ( _turning_mixture < 1.f )
                ah_angular_rate = weight_max * ah_angular_rate +
                                  ( 1.f - weight_max ) * _euler_vector.z();

            if ( _use_intermediary_heading_smoothing )
            {
                if ( _intermediary_angle.full() )
                {
                    // Remove first element from the accumulated value
                    _intermediary_smoothing_val -= _intermediary_angle.front();
                    _intermediary_angle.push_back( d_angle );
                    _intermediary_smoothing_val += _intermediary_angle.back();
                }
                else
                {
                    // Filling the buffer
                    _intermediary_angle.push_back( d_angle );
                    _intermediary_smoothing_val += _intermediary_angle.back();
                }

                d_angle = _intermediary_smoothing_val / _intermediary_angle.size();

            }
            else
            {
                _intermediary_angle.push_back( d_angle );
            }

            // Segment weights have already been pre-multiplied
            blended_dangle = d_angle * _turning_mixture + ah_angular_rate * ( 1.f - _turning_mixture );

            //std::cout << "  ** " << ah_angular_rate << ", " << blended_dangle << ", " << blended_angle << std::endl;

            if ( _use_output_heading_smoothing )
            {
                if ( _output_angle.full() )
                {
                    // Remove first element from the accumulated value
                    _output_smoothing_val -= _output_angle.front();
                    _output_angle.push_back( blended_dangle );
                    _output_smoothing_val += _output_angle.back();
                }
                else
                {
                    // Filling the buffer
                    _output_angle.push_back( blended_dangle );
                    _output_smoothing_val += _output_angle.back();
                }

                // Smooth the post-blend rate by using a weighted average of
                // the calculated rate and the smoothed value. We do this
                // because auto-heading is computing an absolute delta to pull
                // us back to the path, and we don't really want to smooth it
                // as it will lead to oscillations
                blended_dangle = _output_smoothing_val / _output_angle.size() * _turning_mixture
                                 + blended_dangle * ( 1.f - _turning_mixture );
            }
            else
            {
                _output_angle.push_back( blended_dangle );
            }

            blended_angle += blended_dangle;
        }
        else
            blended_angle = _last_unwrapped_angle;

        //std::cout << " *** " << blended_dangle  << ", " << blended_angle << ", " << _output_smoothing_val << std::endl;

        //        std::cout << "  " << "<" << ahv << ">, <" << tv << ">" << std::endl;
        _ratio_values[3] = tv.length() / _frame_rate;

        if ( _restrict_vertical_motion )
            tv.z() = -std::abs( 10.f * _body->getLinearVelocity().z() );
        else
            tv.z() = _body->getLinearVelocity().z();

        //std::cout << "  #  " << _body->getLinearVelocity().z() << std::endl;

        _last_position = _body->getWorldTransform().getOrigin();

        if ( !_use_dynamics )
        {
            btTransform start, end;

            btVector3 step( tv.x(), tv.y(), 0.f );
            btVector3 targetPosition = current_position + step;

            //          std::cout << "  " << current_position << ", " << targetPosition << std::endl;

            start.setIdentity();
            end.setIdentity();

            start.setOrigin( current_position );
            end.setOrigin( targetPosition );

            btCollisionWorld::ClosestConvexResultCallback callback( current_position, targetPosition );
            callback.m_collisionFilterGroup = COL_CAMERA;

            _dynamics_world->world->convexSweepTest ( ( btConvexShape* )( _body->getCollisionShape() ), start, end, callback, _dynamics_world->world->getDispatchInfo().m_allowedCcdPenetration );

            if ( callback.hasHit() )
            {
                btVector3 punch( 2.f * tv.x() / _frame_rate, 2.f * tv.y() / _frame_rate, tv.z() );
                _body->setAngularVelocity( btVector3( 0.f, 0.f, 0.f ) );
                _body->setLinearVelocity( punch );
            }
            else
            {
                _body->getWorldTransform().setOrigin( targetPosition );
                btVector3 punch( 0.f, 0.f, tv.z() );
                // Need these to dampen motion when transitioning from dynamics back to kinematics
                _body->setAngularVelocity( btVector3( 0.f, 0.f, 0.f ) );
                _body->setLinearVelocity( punch );
            }
        }
        else
        {
            btVector3 punch( 2.f * tv.x() / _frame_rate, 2.f * tv.y() / _frame_rate, tv.z() );
            _body->setAngularVelocity( btVector3( 0.f, 0.f, 0.f ) );
            _body->setLinearVelocity( punch );
        }

        {
            // Block for updating tracker
            const osg::Quat& rot = _tracker->getRotation();
            osg::Matrix mat;
            mat.makeRotate( -WrapTwoPi( blended_angle - _last_unwrapped_angle ), _y_axis );
            osg::Quat r;
            r.set( mat );
            _tracker->setRotation( r * rot );
            ( r * rot ).get( mat );
            _last_unwrapped_angle = blended_angle;
        }
    }
}

