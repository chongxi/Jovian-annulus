// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
#include <algorithm>
using std::min;
using std::max;

#include <boost/make_shared.hpp>
#include <boost/multi_array.hpp>

#include <QtCore/QTimer>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QWindow>

#include <osg/io_utils>

#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Point>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osg/TexEnv>
#include <osg/TexMat>
#include <osg/TexGen>
#include <osg/TexGenNode>
#include <osg/Version>

#include <osgDB/ReadFile>

#include <osgGA/NodeTrackerManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/UFOManipulator>

#include <osgUtil/Statistics>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <CameraUpdateCallback.h>
#include <Globals.h>
#include <MyGraphicsWindowQt.h>
#include <hud.h>
#include <viewingWindowQt.h>
using namespace::boost::asio;

int INVISIBLE_MASK = 0x01;


Viewing_Window_Qt::Viewing_Window_Qt( int nCameras, std::vector< Graph_Widget* > g,
                                      osg::ArgumentParser& arguments, Scene_Model_Ptr model ) :
    QObject( ), _model( model ), osgViewer::Viewer( arguments ), widgets( 0 ), _num_displays( 1 ),
    _use_annulus( false ), _inner_radius( 0.2 ), _outer_radius( 1.0 ),
    _minor_axis( 1.0 ), _major_axis( 1.0 ), _center( 0, 0 ),
    _h_fov( 90.f ), _v_fov( 45.0f ), _rotate_camera( true ),
    _initiated( false ), _x_center_distorts_y( false ),
    _x_focal_length( 1 ), _y_focal_length( 1 ),
    _use_distortion( 1 ), _clear_color( osg::Vec4( 0.f, 0.f, 0.f, 1.f ) ),
    _bottom_texcoord( 1 ), _indicator( 0 ), _indicator_size( 100 ),
    _indicator_border_size( 20 ), _indicator_on( 1 ), _hud( 0 ), _buffer_samples( 60 ),
    _avg_frame_buffer( _buffer_samples ), _average_frames( false ), _port( 0 ), _output_session( 0 ),
    _graph( g ), _packFrames( false ), _last_output_time( -10000.0 ), _output_rate( 1.f / 60.f ),
    _output_format( 0 ), _output_treadmill_data( false ), _use_reduced_output( false ),
    _custom_widget_enabled( false ), _tex_width( -1 ), _tex_height( -1 )
{
    _x_focal_length[0] = 1.0f;
    _y_focal_length[0] = 1.0f;
    _bottom_texcoord[0] = osg::Vec2( -0.5f, -0.5f );
    _use_distortion[0] = true;

    _key_switch_manipulator = new osgGA::KeySwitchMatrixManipulator;

    _key_switch_manipulator->addMatrixManipulator( '1', "Trackball", new osgGA::TrackballManipulator() );
    _key_switch_manipulator->addMatrixManipulator( '2', "UFO", new osgGA::UFOManipulator() );

    setCameraManipulator( _key_switch_manipulator.get() );
    addEventHandler( new osgViewer::StatsHandler );

    _indicator = new osg::Switch;
    _childNum = 0;

    for ( int i = 0; i < nCameras; i++ )
    {
        _graph[i]->set_viewer( this );
        _global_brightness.push_back( osg::Vec4( 1.f, 1.f, 1.f, 1.f ) );
    }

    viewerInit();
    _swapBuffers = boost::make_shared< bool >( true );
    setSceneData( _model->scene_root() );
    setThreadingModel( suggestBestThreadingModel() );
    average_frames();
}

Viewing_Window_Qt::~Viewing_Window_Qt()
{
    _timer.stop();
    disconnect( &_timer );
    stopThreading();

    for ( int i = 0; i < _num_displays; i++ )
        widgets[ i ]->hide();

    delete [] widgets;

    if ( _port )
        delete _port;

    if ( _output_session )
        _output_session = 0; // Clean up is the responsibility of Console
}

void
Viewing_Window_Qt::initialize_cameras( int num_displays, int starting_display )
{
    _num_displays = num_displays;
    _whichScreen = _num_displays - 1;

    widgets = new QWidget*[ _num_displays ];

    _x_focal_length.resize( _num_displays );
    _y_focal_length.resize( _num_displays );
    _use_distortion.resize( _num_displays );
    _bottom_texcoord.resize( _num_displays );

    _screenNum = starting_display;

    osgQt::GraphicsWindowQt* widget;
    for ( int i = 0; i < _num_displays; i++ )
    {
        _x_focal_length[i] = 1.0f;
        _y_focal_length[i] = 1.0f;
        _bottom_texcoord[i] = osg::Vec2( -0.5f, -0.5f );
        _use_distortion[i] = true;
        widget = multipleWindowWithDistortion( i, num_displays, starting_display );
        widgets[ i ] = widget->getGLWidget();
    }

    osg::Node* data;

    if ( _packFrames )
    {
        _frame_switch.push_back( new osg::Switch );
        for ( int idx = 0; idx < 3; idx++ )
            _frame_switch.back()->addChild( createDistortionSubgraphWithPacking( 0, idx, getSceneData(),
                                                                                 _clear_color,
                                                                                 true ) );
        _frame_switch.back()->setAllChildrenOff();
        data = _frame_switch.back();
    }
    else
        data = createDistortionSubgraph( 0, _clear_color, true );

    getCamera()->setProjectionMatrixAsPerspective( _h_fov, 1.f, 0.1, 10000.0 );
    //getCamera()->setViewMatrix( osg::Matrixd::rotate( osg::inDegrees( 90.0f ), 0.0, 1.0, 0.0 ) * getCamera()->getViewMatrix() );

    QRect screenres = QApplication::desktop()->screenGeometry( _traits->screenNum );

    for ( int i = 0; i < _num_displays; i++ )
    {
        widgets[ i ]->show();
#ifdef __APPLE__
        widgets[ i ]->move( screenres.x() + _traits->x, screenres.y() + _traits->y );
        widgets[ i ]->resize( _width, _height );
#endif
    }

    // initialize the global timer to be relative to the current time.
    osg::Timer::instance()->setStartTick();

    // pass on the start tick to all the associated event queues
    setStartTick( osg::Timer::instance()->getStartTick() );

    connect( &_timer, SIGNAL( timeout() ), this, SLOT( paintEvent() ) );

    QDesktopWidget desktopWidget;
    QRect r( desktopWidget.screenGeometry( starting_display ) );
    _indicator_pos = osg::Vec2( r.right() - _num_displays * _indicator_size, r.top() );

    // Create a lighting Shader
    _timer.start( 1 );
}

void
Viewing_Window_Qt::set_serial_port( std::string port )
{
    try
    {
        // what baud rate do we communicate at
        serial_port_base::baud_rate baud( 115200 );
        // how big is each "packet" of data (default is 8 bits)
        serial_port_base::character_size csize( 8 );
        // what flow control is used (default is none)
        serial_port_base::flow_control flow( serial_port_base::flow_control::none );
        // what parity is used (default is none)
        serial_port_base::parity parity( serial_port_base::parity::none );
        // how many stop bits are used (default is one)
        serial_port_base::stop_bits stop( serial_port_base::stop_bits::one );

        _port = new serial_port( _io, port );
        _port->set_option( baud );
        _port->set_option( csize );
        _port->set_option( flow );
        _port->set_option( parity );
        _port->set_option( stop );
    }
    catch ( ... )
    {
        clear_serial_port();
        std::cout << "Unable to set serial port to\"" << port << "\"" << std::endl;
    }
}

// Outputs an event string. Prepends a time stamp, appends a newline
void
Viewing_Window_Qt::output_event( std::ostringstream const& event_string,
                                 std::ostringstream const& float_event_string,
                                 bool force_write,
                                 float time_stamp ) const
{
    double refTime;
    if ( time_stamp > 0 )
        refTime = time_stamp;
    else
        refTime = _currSimTime;

    std::ostringstream command, float_command;

    float_command << refTime - _startRefTime << "," << float_event_string.str() << std::endl;
    _model->output_event( float_command );

    if ( force_write || ( refTime - _last_output_time > _output_rate ) )
    {
        _last_output_time = refTime;
        command << ( int )( ( refTime - _startRefTime ) * 1000.0 ) << ","
                << event_string.str() << std::endl;

        bool to_console = (_output_session || _port) ? false : true;
        if (_output_session )
            _output_session->write( command.str() );

        if ( _port )
            write( *_port, buffer( command.str().c_str(), command.str().size() ) );

        if ( to_console )
        {
            std::cout << command.str();
            std::cout.flush();
        }
    }
}

void
Viewing_Window_Qt::myRenderingTraversals()
{
    bool _outputMasterCameraLocation = false;
    if ( _outputMasterCameraLocation )
    {
        Views views;
        getViews( views );

        for ( Views::iterator itr = views.begin();
                itr != views.end();
                ++itr )
        {
            osgViewer::View* view = *itr;
            if ( view )
            {
                const osg::Matrixd& m = view->getCamera()->getInverseViewMatrix();
                OSG_NOTICE << "View " << view << ", Master Camera position(" << m.getTrans() << "), rotation(" << m.getRotate() << ")" << std::endl;
            }
        }
    }

    Contexts contexts;
    getContexts( contexts );

    // check to see if windows are still valid
#if OSG_VERSION_LESS_THAN( 3, 0, 0 )
    checkWindowStatus();
#else
    checkWindowStatus( contexts );
#endif
    if ( _done ) return;

    double beginRenderingTraversals = elapsedTime();

    osg::FrameStamp* frameStamp = getViewerFrameStamp();

    if ( getViewerStats() && getViewerStats()->collectStats( "scene" ) )
    {
        unsigned int frameNumber = frameStamp ? frameStamp->getFrameNumber() : 0;

        Views views;
        getViews( views );
        for ( Views::iterator vitr = views.begin();
                vitr != views.end();
                ++vitr )
        {
            View* view = *vitr;
            osg::Stats* stats = view->getStats();
            osg::Node* sceneRoot = view->getSceneData();
            if ( sceneRoot && stats )
            {
                osgUtil::StatsVisitor statsVisitor;
                sceneRoot->accept( statsVisitor );
                statsVisitor.totalUpStats();

                unsigned int unique_primitives = 0;
                osgUtil::Statistics::PrimitiveCountMap::iterator pcmitr;
                for ( pcmitr = statsVisitor._uniqueStats.GetPrimitivesBegin();
                        pcmitr != statsVisitor._uniqueStats.GetPrimitivesEnd();
                        ++pcmitr )
                {
                    unique_primitives += pcmitr->second;
                }

                stats->setAttribute( frameNumber, "Number of unique StateSet", static_cast<double>( statsVisitor._statesetSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Group", static_cast<double>( statsVisitor._groupSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Transform", static_cast<double>( statsVisitor._transformSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique LOD", static_cast<double>( statsVisitor._lodSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Switch", static_cast<double>( statsVisitor._switchSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Geode", static_cast<double>( statsVisitor._geodeSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Drawable", static_cast<double>( statsVisitor._drawableSet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Geometry", static_cast<double>( statsVisitor._geometrySet.size() ) );
                stats->setAttribute( frameNumber, "Number of unique Vertices", static_cast<double>( statsVisitor._uniqueStats._vertexCount ) );
                stats->setAttribute( frameNumber, "Number of unique Primitives", static_cast<double>( unique_primitives ) );

                unsigned int instanced_primitives = 0;
                for ( pcmitr = statsVisitor._instancedStats.GetPrimitivesBegin();
                        pcmitr != statsVisitor._instancedStats.GetPrimitivesEnd();
                        ++pcmitr )
                {
                    instanced_primitives += pcmitr->second;
                }

                stats->setAttribute( frameNumber, "Number of instanced Stateset", static_cast<double>( statsVisitor._numInstancedStateSet ) );
                stats->setAttribute( frameNumber, "Number of instanced Group", static_cast<double>( statsVisitor._numInstancedGroup ) );
                stats->setAttribute( frameNumber, "Number of instanced Transform", static_cast<double>( statsVisitor._numInstancedTransform ) );
                stats->setAttribute( frameNumber, "Number of instanced LOD", static_cast<double>( statsVisitor._numInstancedLOD ) );
                stats->setAttribute( frameNumber, "Number of instanced Switch", static_cast<double>( statsVisitor._numInstancedSwitch ) );
                stats->setAttribute( frameNumber, "Number of instanced Geode", static_cast<double>( statsVisitor._numInstancedGeode ) );
                stats->setAttribute( frameNumber, "Number of instanced Drawable", static_cast<double>( statsVisitor._numInstancedDrawable ) );
                stats->setAttribute( frameNumber, "Number of instanced Geometry", static_cast<double>( statsVisitor._numInstancedGeometry ) );
                stats->setAttribute( frameNumber, "Number of instanced Vertices", static_cast<double>( statsVisitor._instancedStats._vertexCount ) );
                stats->setAttribute( frameNumber, "Number of instanced Primitives", static_cast<double>( instanced_primitives ) );
            }
        }
    }

    osgViewer::ViewerBase::Scenes scenes;
    getScenes( scenes );

    for ( osgViewer::ViewerBase::Scenes::iterator sitr = scenes.begin();
            sitr != scenes.end();
            ++sitr )
    {
        osgViewer::Scene* scene = *sitr;
        osgDB::DatabasePager* dp = scene ? scene->getDatabasePager() : 0;
        if ( dp )
        {
            dp->signalBeginFrame( frameStamp );
        }

        if ( scene->getSceneData() )
        {
            // fire off a build of the bounding volumes while we
            // are still running single threaded.
            scene->getSceneData()->getBound();
        }
    }

    // OSG_NOTICE<<std::endl<<"Start frame"<<std::endl;


    osgViewer::ViewerBase::Cameras cameras;
    getCameras( cameras );

    osgViewer::ViewerBase::Contexts::iterator itr;

    bool doneMakeCurrentInThisThread = false;

    if ( _endDynamicDrawBlock.valid() )
    {
        _endDynamicDrawBlock->reset();
    }

    // dispatch the rendering threads
    if ( _startRenderingBarrier.valid() ) _startRenderingBarrier->block();

    // reset any double buffer graphics objects
    for ( osgViewer::ViewerBase::Cameras::iterator camItr = cameras.begin();
            camItr != cameras.end();
            ++camItr )
    {
        osg::Camera* camera = *camItr;
        osgViewer::Renderer* renderer = dynamic_cast<osgViewer::Renderer*>( camera->getRenderer() );
        if ( renderer )
        {
            if ( !renderer->getGraphicsThreadDoesCull() && !( camera->getCameraThread() ) )
            {
                renderer->cull();
            }
        }
    }

    for ( itr = contexts.begin();
            itr != contexts.end();
            ++itr )
    {
        if ( _done ) return;
        if ( !( ( *itr )->getGraphicsThread() ) && ( *itr )->valid() )
        {
            doneMakeCurrentInThisThread = true;
            makeCurrent( *itr );
            ( *itr )->runOperations();
        }
    }

    // OSG_NOTICE<<"Joing _endRenderingDispatchBarrier block "<<_endRenderingDispatchBarrier.get()<<std::endl;

    // wait till the rendering dispatch is done.
    if ( _endRenderingDispatchBarrier.valid() ) _endRenderingDispatchBarrier->block();

    for ( itr = contexts.begin();
            itr != contexts.end();
            ++itr )
    {
        if ( _done ) return;

        if ( !( ( *itr )->getGraphicsThread() ) && ( *itr )->valid() )
        {
            doneMakeCurrentInThisThread = true;
            makeCurrent( *itr );
            if ( *_swapBuffers )
                ( *itr )->swapBuffers();
        }
    }

    for ( osgViewer::ViewerBase::Scenes::iterator sitr = scenes.begin();
            sitr != scenes.end();
            ++sitr )
    {
        osgViewer::Scene* scene = *sitr;
        osgDB::DatabasePager* dp = scene ? scene->getDatabasePager() : 0;
        if ( dp )
        {
            dp->signalEndFrame();
        }
    }

    // wait till the dynamic draw is complete.
    if ( _endDynamicDrawBlock.valid() )
    {
        // osg::Timer_t startTick = osg::Timer::instance()->tick();
        _endDynamicDrawBlock->block();
        // OSG_NOTICE<<"Time waiting "<<osg::Timer::instance()->delta_m(startTick, osg::Timer::instance()->tick())<<std::endl;;
    }

    if ( _releaseContextAtEndOfFrameHint && doneMakeCurrentInThisThread )
    {
        //OSG_NOTICE<<"Doing release context"<<std::endl;
        releaseContext();
    }

    if ( getViewerStats() && getViewerStats()->collectStats( "update" ) )
    {
        double endRenderingTraversals = elapsedTime();

        // update current frames stats
        getViewerStats()->setAttribute( frameStamp->getFrameNumber(), "Rendering traversals begin time ", beginRenderingTraversals );
        getViewerStats()->setAttribute( frameStamp->getFrameNumber(), "Rendering traversals end time ", endRenderingTraversals );
        getViewerStats()->setAttribute( frameStamp->getFrameNumber(), "Rendering traversals time taken", endRenderingTraversals - beginRenderingTraversals );
    }

    _requestRedraw = false;
}


void
Viewing_Window_Qt::paintEvent()
{
    double frameTime;
    osg::Vec3d eye, center, up;

    if ( _packFrames )
        *_swapBuffers = false;

    double refTime = getViewerFrameStamp()->getReferenceTime();
    frameTime = refTime - _lastRefTime;

    int frame_count = _packFrames ? 3 : 1;
    // Frame rate is already predivided by the frame count
    float frame_rate = _model->average_framerate();

    for ( int i = 0; i < frame_count; i++ )
    {
        if ( _initiated )
        {
            getCamera()->getViewMatrixAsLookAt( eye, center, up );
            _model->set_eye_position( center );
            _model->reset_traversal();
        }

        if ( _packFrames )
            for ( int j = 0; j < _num_displays; j++ )
                _frame_switch[ j ]->setSingleChildOn( i );

        advance();
        eventTraversal();
        updateTraversal();
        if ( _packFrames && i == 2 )
            *_swapBuffers = true;


        if ( _model->physics_enabled() )
        {
            _currSimTime = _prevSimTime + frame_rate;
            _model->step_simulation( frame_rate,
                                     3, frameTime / ( double )( 6 * frame_count ) );
            _prevSimTime = _currSimTime;
        }

        myRenderingTraversals();


        if ( _initiated )
        {
            osg::Vec3d dir = center - eye;
            osg::Vec3d x_axis( 1, 0, 0 );
            dir.normalize();
            osg::Vec3d cross = dir ^ x_axis;
            int sign = ( up * cross ) < 0 ? 1 : -1;
            double cos_theta = dir * x_axis;
            double speed;
            btVector3 body_center = _model->camera_position();

            //btVector3 tv = body_center - _lastPosition;
            //std::cout << "center - <" << body_center.x() << ", " << body_center.y() << ", " << body_center.z() << ">" << std::endl;
            //std::cout << "_lastPosition - <" << _lastPosition.x() << ", " << _lastPosition.y() << ", " << _lastPosition.z() << ">" << std::endl;
            //std::cout << "tv - <" << tv.x() << ", " << tv.y() << ", " << tv.z() << ">" << std::endl;
            //std::cout << "dist - " << tv.length() << " " << frameTime << std::endl;

            if ( !_model->motion_disabled() )
            {
                speed = ( ( body_center - _lastPosition ).length() ) / frameTime;
                speed = speed < 5.e-4 ? 0 : speed;
            }
            else
            {
                Camera_Update_Callback* callback = dynamic_cast<Camera_Update_Callback*>( getCamera()->getUpdateCallback() );

                osg::Vec3d const& vec = callback->displacement();
                speed = vec.length() / frameTime;
                callback->clear();
            }

            _lastPosition = body_center;
            center.x() = body_center.x() - _model->scene_bound()->xMin();
            center.y() = body_center.y() - _model->scene_bound()->yMin();
            center.z() = body_center.z();
            Motion_Data* md = ( Motion_Data* )_model->tracker()->getUserData();
            std::ostringstream command, float_command;
            if ( _use_reduced_output )
                command << ( int )( center.x() * 100.f ) << "," << ( int )( center.y() * 100.f ) << ",";
            else
                command << ( int )( center.x() * 100.f ) << "," << ( int )( center.y() * 100.f ) << ","
                        << ( int )( center.z() * 100 ) << ","
                        << ( int )( speed * 100.0 ) << ","
                        << ( int )( sign * ( 18000.0 / M_PI ) * acos( cos_theta ) ) << ","
                        << _output_format << ",";

            float_command << center.x() << "," << center.y() << "," << center.z() << ","
                          << speed << ","
                          << sign * ( 180.0 / M_PI ) * acos( cos_theta ) << ","
                          << _output_format << ",";

            if ( _output_format > 0 )
            {
                switch ( _output_format )
                {
                    case 1:
                    {
                        command << ( int )( md->velocity * 100.f ) << ","
                                << ( int )( ( 18000.0 / M_PI ) * md->angle ) << ",";
                        float_command << ( md->velocity * 100.f ) << ","
                                      << ( ( 18000.0 / M_PI ) * md->angle ) << ",";
                        break;
                    }
                    case 2:
                    {
                        command << ( int )( md->velocity * 100.f ) << ","
                                << ( int )( ( 18000.0 / M_PI ) * md->roll ) << ","
                                << ( int )( ( 18000.0 / M_PI ) * md->pitch ) << ","
                                << ( int )( ( 18000.0 / M_PI ) * md->yaw ) << ",";
                        float_command << ( md->velocity * 100.f ) << ","
                                      << ( ( 18000.0 / M_PI ) * md->roll ) << ","
                                      << ( ( 18000.0 / M_PI ) * md->pitch ) << ","
                                      << ( ( 18000.0 / M_PI ) * md->yaw ) << ",";
                        break;
                    }
                    default:
                        // Unknown format id, but we don't want to print that on every frame,
                        // so we just ignore it
                        break;

                }
            }

            if ( _output_treadmill_data )
            {
                if ( !_use_reduced_output )
                    command << ( int )md->raw.x() << "," << ( int )md->raw.y() << ","
                            << ( int )md->raw.z() << "," << ( int )md->raw.w() << ",";
                float_command << ( int )md->raw.x() << "," << ( int )md->raw.y() << ","
                              << ( int )md->raw.z() << "," << ( int )md->raw.w() << ",";
            }

            command << _model->contact_count();
            float_command << _model->contact_count();

            if ( _model->contact_count() )
            {
                _model->use_dynamics( true );

                std::set< osg::Node* > const& contact_nodes( _model->contact_nodes() );

                std::set< osg::Node* >::const_iterator node_it;
                for ( node_it = contact_nodes.begin();
                        node_it != contact_nodes.end(); node_it++ )
                {
                    command << "," << ( *node_it )->getName();
                    float_command << "," << ( *node_it )->getName();
                }
            }
            else
                _model->use_dynamics( false );

            output_event( command, float_command, false, _currSimTime );
        }
    }

    if ( _indicator_on > 0 )
    {
        _indicator->setSingleChildOn( _childNum + frame_count );
        if ( ( _indicator_on == 1 && _initiated ) || _indicator_on == 2 )
            _childNum = ( _childNum == 1 ? 0 : 1 );
    }
    else
        _indicator->setAllChildrenOff( );

    _lastRefTime = refTime;

    if ( _average_frames && frameTime > 0 && frameTime < 1.0 )
    {
        if ( _ignored_frames_count > 0 )
            _ignored_frames_count--;
        else
        {
            if ( _avg_frame_buffer.full() )
            {
                // Remove first element from the accumulated value
                _avg_frame_time -= _avg_frame_buffer.front();
                _avg_frame_buffer.push_back( frameTime );
                _avg_frame_time += frameTime;
            }
            else
            {
                // Filling the buffer
                _avg_frame_buffer.push_back( frameTime );
                _avg_frame_time += frameTime;
            }

            _avg_frame_count++;
            if ( _avg_frame_count >= _buffer_samples )
            {
                float swap_frame_rate = _avg_frame_time / _buffer_samples;
                int hertz = ( int ) ( 1.f / swap_frame_rate );

                if ( _avg_frame_buffer.capacity() < hertz )
                {
                    _avg_frame_buffer.resize( hertz, 0.f );
                    _avg_frame_buffer.clear();
                    _buffer_samples = hertz;
                    _avg_frame_time = 0.f;
                }

                _model->set_average_framerate( swap_frame_rate / ( double )frame_count );
                _avg_frame_count = 0;
            }
        }
    }


    if ( _packFrames )
        *_swapBuffers = true;

};

void
Viewing_Window_Qt::load_osg( std::string filename )
{
    _model->load_osg( filename );
    setup_slaves();
}

void
Viewing_Window_Qt::load_model( osg::ArgumentParser& arguments )
{
    _model->load_model( arguments );
    setup_slaves();
}

void
Viewing_Window_Qt::load_image( std::string filename, bool flip, bool use_texture_rectangle )
{
    _model->load_image( filename, flip, use_texture_rectangle );
    setup_slaves();
}

void
Viewing_Window_Qt::load_data( )
{
    setup_slaves();

    // Needed here to initialize the simulation time
    advance();

    _currSimTime = getFrameStamp()->getSimulationTime();
    _prevSimTime = getFrameStamp()->getSimulationTime();
}

void
Viewing_Window_Qt::update_viewport( int i, int leftOffset, int rightOffset )
{
    osg::Viewport* vp = getSlave( i )._camera->getViewport();

    vp->setViewport( leftOffset, vp->y(), rightOffset - leftOffset, vp->height() );
}

QWidget*
Viewing_Window_Qt::addViewWidget( osg::Camera* camera )
{
    if ( _custom_widget_enabled )
    {
        MyGraphicsWindowQt* gw = dynamic_cast<MyGraphicsWindowQt*>( camera->getGraphicsContext() );
        return gw ? gw->getOpenGLWidget() : NULL;
    }
    else
    {
        osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>( camera->getGraphicsContext() );
        return gw ? gw->getGLWidget() : NULL;
    }
}

void
Viewing_Window_Qt::setup_slaves()
{
    double aspectRatioScale = ( ( float )_width / ( float )_height ) * ( _num_displays == 1 ? 1.0 : ( double )_num_displays - 1 );

    QDesktopWidget desktopWidget;
    QRect r( desktopWidget.screenGeometry( _screenNum ) );
    _indicator = new osg::Switch;
    _hud = createHUD( _indicator, _num_displays, r.left(), r.right(),
                      r.top(), r.bottom(), _indicator_pos,
                      _indicator_size, _indicator_border_size );

    _frame_switch.clear();

    stopThreading();

    for ( int i = 0; i < _num_displays; i++ )
    {
        osgViewer::ViewerBase::Cameras cameras;
        getCameras( cameras );
        // Remove slaves
        for ( osgViewer::ViewerBase::Cameras::iterator it = cameras.begin(); it != cameras.end(); ++it )
        {
            int index = findSlaveIndexForCamera( *it );
            osg::View::Slave& slave = getSlave( index );
            slave._camera->removeChildren( 0, slave._camera->getNumChildren() );
            removeSlave( index );
        }
        osg::Node* data = createDistortionSubgraph( i, _clear_color, i == _whichScreen );
    }

    setSceneData( _model->scene_root() );
    startThreading();
}

void
Viewing_Window_Qt::set_track_node()
{
    _key_switch_manipulator->addMatrixManipulator( '3', "NodeTracker", _model->tracker().get() );
}

void
Viewing_Window_Qt::set_update_callback( osg::NodeCallback* callback, bool reset )
{
    getCamera()->setUpdateCallback( callback );
    if ( callback != 0 )
    {
        if ( reset )
        {
            _startRefTime = getFrameStamp()->getReferenceTime();
            _currSimTime = getFrameStamp()->getSimulationTime();
            _prevSimTime = getFrameStamp()->getSimulationTime();
        }

        _lastRefTime = 0.f;
        _lastPosition = _model->camera_position();
        _childNum = 0;
        _initiated = true;
    }
    else
    {
        _childNum = 2;
        _initiated = false;
    }
}
void
setupRTTCubeMap( osg::TextureCubeMap* texture,
                 unsigned tex_width, unsigned tex_height )
{
    texture->setTextureSize( tex_width, tex_height );

    texture->setInternalFormat( GL_RGB );
    texture->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    texture->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    texture->setWrap( osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE );
    texture->setFilter( osg::TextureCubeMap::MIN_FILTER, osg::TextureCubeMap::LINEAR );
    texture->setFilter( osg::TextureCubeMap::MAG_FILTER, osg::TextureCubeMap::LINEAR );
}

osg::Geometry*
Viewing_Window_Qt::createSkyboxDistortionMesh( const osg::Vec3& origin, const osg::Vec3& widthVector, const osg::Vec3& heightVector )
{
    osg::Vec3d eye( 0.0, 0.0, 0.0 );

    bool centerProjection = false;

    // create the quad to visualize.
    osg::Geometry* geometry = new osg::Geometry();

    geometry->setSupportsDisplayList( false );

    osg::Vec3 xAxis( widthVector );
    float width = widthVector.length();
    xAxis /= width;

    osg::Vec3 yAxis( heightVector );
    float height = heightVector.length();
    yAxis /= height;

    int noSteps = 50;

    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec3Array* texcoords = new osg::Vec3Array;
    osg::Vec4Array* colors = new osg::Vec4Array;

    osg::Vec3 bottom = origin;
    osg::Vec3 dx = xAxis * ( width / ( ( float )( noSteps - 1 ) ) );
    osg::Vec3 dy = yAxis * ( height / ( ( float )( noSteps - 1 ) ) );

    osg::Vec2 dx_texcoord( 1.0f / ( float )( noSteps - 1 ), 0.0f );
    osg::Vec2 dy_texcoord( 0.0f, 1.0f / ( float )( noSteps - 1 ) );

    osg::Vec3d screenCenter = origin + widthVector * 0.5f + heightVector * 0.5f;
    float screenRadius = heightVector.length() * 0.5f;

    float max_radius = height;
    if ( width < max_radius ) max_radius = width;
    max_radius /= 2.f;
    osg::Vec2 center = osg::Vec2( width / 2.f, height / 2.f ) + _center;

    float delta_radius = max_radius * ( _outer_radius - _inner_radius ) / ( float )( noSteps - 1 );
    float radius = max_radius * _inner_radius;
    float h_fov_2 = _h_fov / 2.f;
    float v_fov_2 = _v_fov / 2.f;

    _graph[0]->compute_basis_matrix();
    typedef boost::multi_array<osg::Vec3, 2> array_type;
    typedef array_type::index index;
    array_type texcoords_array( boost::extents[noSteps][noSteps] );

    //std::cout << std::endl;
    //std::cout << std::endl;

    int i, j;
    for ( i = 0; i < noSteps; ++i )
    {
        osg::Vec3 cursor = bottom + dy * ( float )i;
        //std::cout << i << ", ";

        for ( j = 0; j < noSteps; ++j )
        {
            osg::Vec2 pos( cursor.x() / width, cursor.y() / height );
            double theta = osg::PI_2 + -h_fov_2 + pos.x() * _h_fov;
#if 0
            double phi = ( osg::DegreesToRadians( 45.0 ) - v_fov_2 ) + pos.y() * _v_fov;
            osg::Vec3 start_vec, end_vec, face_ul, face_ur, face_ll, face_lr;
            double start_theta, end_theta, offset_theta;
            if ( osg::RadiansToDegrees( theta ) >= 45 && osg::RadiansToDegrees( theta ) < 135 )
            {
                face_ul = osg::Vec3( -0.5, 0.5, 0.5 );
                face_ur = osg::Vec3( 0.5, 0.5, 0.5 );
                face_ll = osg::Vec3( -0.5, 0.5, -0.5 );
                face_lr = osg::Vec3( 0.5, 0.5, -0.5 );
                offset_theta = theta - osg::DegreesToRadians( 45.0 );
            }
            else if ( osg::RadiansToDegrees( theta ) >= -45 && osg::RadiansToDegrees( theta ) < 45 )
            {
                face_ul = osg::Vec3( -0.5, -0.5, 0.5 );
                face_ur = osg::Vec3( -0.5, 0.5, 0.5 );
                face_ll = osg::Vec3( -0.5, -0.5, -0.5 );
                face_lr = osg::Vec3( -0.5, 0.5, -0.5 );
                offset_theta = theta - osg::DegreesToRadians( -45.0 );
            }
            else if ( osg::RadiansToDegrees( theta ) >= 135 && osg::RadiansToDegrees( theta ) < 225 )
            {
                face_ul = osg::Vec3( 0.5, 0.5, 0.5 );
                face_ur = osg::Vec3( 0.5, -0.5, 0.5 );
                face_ll = osg::Vec3( 0.5, 0.5, -0.5 );
                face_lr = osg::Vec3( 0.5, -0.5, -0.5 );
                offset_theta = theta - osg::DegreesToRadians( 135.0 );
            }

            start_vec = osg::Vec3( face_ll.x() * cos( phi ) + face_ul.x() * sin( phi ),
                                   face_ll.y() * cos( phi ) + face_ul.y() * sin( phi ),
                                   face_ll.z() * cos( phi ) + face_ul.z() * sin( phi ) );
            end_vec = osg::Vec3( face_lr.x() * cos( phi ) + face_ur.x() * sin( phi ),
                                 face_lr.y() * cos( phi ) + face_ur.y() * sin( phi ),
                                 face_lr.z() * cos( phi ) + face_ur.z() * sin( phi ) );

            osg::Vec3 texcoord( start_vec.x() * cos( offset_theta ) + end_vec.x() * sin( offset_theta ),
                                start_vec.y() * cos( offset_theta ) + end_vec.y() * sin( offset_theta ),
                                start_vec.z() * cos( offset_theta ) + end_vec.z() * sin( offset_theta ) );
            texcoord.normalize();
#else
            double phi = osg::PI_2 - ( -v_fov_2 + pos.y() * _v_fov );
            double offset = sin( phi - osg::PI_2 ) * _y_focal_length[0] * cos( 4.0 * ( theta + osg::PI_4 ) );
#if 0
            // Point on cone of`height', bottom centered at 0, 0, 0 with radius 'rad'
            double x = rad * (1.f - pos.y() ) * cos( theta );
            double y = rad * (1.f - pos.y() ) * sin( theta );
            double z = pos.y() * height;
            osg::Vec3 texcoord( cos( theta ),
                                sin( theta ),
                                cos( phi ) );
#else
            osg::Vec3 texcoord( sin( phi ) * cos( theta ),
                                sin( phi ) * sin( theta ),
                                cos( phi ) + offset );
#endif
#endif

            //std::cout << i << ", " << j << ", " << osg::RadiansToDegrees( theta ) << ", "
            //          << osg::RadiansToDegrees( phi-osg::PI_2 ) << ", " << offset << std::endl;

            //std::cout << texcoord.x() << ", " << texcoord.y() << ", " << texcoord.z() << " ";
            texcoords_array[i][j] = texcoord;

            cursor += dx;
        }
        //std::cout << std::endl;
    }

    for ( i = 0; i < noSteps; ++i )
    {
        osg::Vec3 cursor = bottom + dy * ( float )i;
        osg::Vec2 eval_tc = _bottom_texcoord[0] + dy_texcoord * ( float )i;
        for ( j = 0; j < noSteps; ++j )
        {
            QPointF pt = _graph[0]->evaluate( eval_tc.x() - _bottom_texcoord[0].x(),
                                              1. - ( eval_tc.y() - _bottom_texcoord[0].y() ) );

            osg::Vec2 pos( cursor.x() / width, cursor.y() / height );
            osg::Vec3 texcoord = texcoords_array[i][j];

            if ( _use_annulus )
            {
                float angle = pt.x() * boost::math::constants::two_pi< float >();
                osg::Vec3 crsr( center.x() + _minor_axis * radius * cosf( angle ),
                                center.y() + _major_axis * radius * sinf( angle ), 0. );
                //std::cout << i << ", " << j << ", " << angle << " - ";
                //std::cout << crsr << ", " << pt.x() << ", " << pt.y() << std::endl;
                vertices->push_back( crsr );
            }
            else
                vertices->push_back( cursor );
            colors->push_back( _global_brightness[ 0 ] );
            //colors->push_back( osg::Vec4( (texcoord.x()+1.0)/2.0, (texcoord.y()+1.0)/2.0, (texcoord.z()+1.0)/2.0, 1.0f ) );
            texcoords->push_back( texcoord );

            cursor += dx;
            eval_tc += dx_texcoord;
        }
        radius += delta_radius;
    }

    // pass the created vertex array to the points geometry object.
    geometry->setVertexArray( vertices );
    geometry->setColorArray( colors, osg::Array::BIND_PER_VERTEX );

    geometry->setTexCoordArray( 0, texcoords );

    for ( i = 0; i < noSteps - 1; ++i )
    {
        osg::DrawElementsUShort* elements = new osg::DrawElementsUShort( osg::PrimitiveSet::QUAD_STRIP );
        for ( j = 0; j < noSteps; ++j )
        {
            elements->push_back( j + ( i + 1 )*noSteps );
            elements->push_back( j + ( i )*noSteps );
        }
        geometry->addPrimitiveSet( elements );
    }

    return geometry;
}

osg::Node*
Viewing_Window_Qt::createDistortionSubgraph( int index, const osg::Vec4& clearColour,
                                             bool addHud )
{
    osg::Group* distortionNode = new osg::Group;

    unsigned int tex_width, tex_height;

    if ( _tex_height < 0 || _tex_width < 0 )
    {
        // Type cast the arguments to log as windows (VS2010) whines about ambiguous call
        tex_width = ( unsigned int ) pow( 2, ceil( log( ( float )( _traits->width ) ) / log( 2.f ) ) );
        tex_height = ( unsigned int ) pow( 2, ceil( log( ( float )( _traits->height ) ) / log( 2.f ) ) );

        if ( tex_width > tex_height )
            tex_height = tex_width;
        else
            tex_width = tex_height;
    }
    else
    {
        tex_width = _tex_width;
        tex_height = _tex_height;
    }

    _cubeTexture = new osg::TextureCubeMap;
    _cubeTexture->setTextureSize( tex_width, tex_height );
    _cubeTexture->setInternalFormat( GL_RGB );
    _cubeTexture->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _cubeTexture->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );
    _cubeTexture->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    _cubeTexture->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    _cubeTexture->setWrap( osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE );

#if 0
    osg::Camera::RenderTargetImplementation renderTargetImplementation = osg::Camera::SEPERATE_WINDOW;
    GLenum buffer = GL_FRONT;
#else
    osg::Camera::RenderTargetImplementation renderTargetImplementation = osg::Camera::FRAME_BUFFER_OBJECT;
    GLenum buffer = GL_FRONT;
#endif

    // front face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName( "Front face camera" );
        camera->setGraphicsContext( _gwqt.get() );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        camera->setDrawBuffer( buffer );
        camera->setReadBuffer( buffer );
        camera->setAllowEventFocus( false );
        camera->setCullMask( ~INVISIBLE_MASK );
        camera->setClearColor( clearColour );
        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::POSITIVE_Y );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd() );
    }

    // left face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName( "Left face camera" );
        camera->setGraphicsContext( _gwqt.get() );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        camera->setDrawBuffer( buffer );
        camera->setReadBuffer( buffer );
        camera->setAllowEventFocus( false );
        camera->setCullMask( ~INVISIBLE_MASK );
        camera->setClearColor( clearColour );

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::NEGATIVE_X );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd::rotate( osg::inDegrees( -90.0f ), 0.0, 1.0, 0.0 ) * osg::Matrixd::rotate( osg::inDegrees( -90.0f ), 0.0, 0.0, 1.0 ) );
    }

    // right face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName( "Right face camera" );
        camera->setGraphicsContext( _gwqt.get() );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        camera->setDrawBuffer( buffer );
        camera->setReadBuffer( buffer );
        camera->setAllowEventFocus( false );
        camera->setCullMask( ~INVISIBLE_MASK );
        camera->setClearColor( clearColour );

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::POSITIVE_X );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd::rotate( osg::inDegrees( 90.0f ), 0.0, 1.0, 0.0 ) * osg::Matrixd::rotate( osg::inDegrees( 90.0f ), 0.0, 0.0, 1.0 ) );
    }

    // back face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName( "Back face camera" );
        camera->setGraphicsContext( _gwqt.get() );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        camera->setDrawBuffer( buffer );
        camera->setReadBuffer( buffer );
        camera->setAllowEventFocus( false );
        camera->setCullMask( ~INVISIBLE_MASK );
        camera->setClearColor( clearColour );

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::NEGATIVE_Y );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd::rotate( osg::inDegrees( 180.0f ), 1.0, 0.0, 0.0 ) );
    }

    // top face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName( "Top face camera" );
        camera->setGraphicsContext( _gwqt.get() );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        GLenum cbuffer = _traits->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer( cbuffer );
        camera->setReadBuffer( cbuffer );
        camera->setAllowEventFocus( false );
        camera->setClearColor( clearColour );
        camera->setCullMask( ~INVISIBLE_MASK );

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::POSITIVE_Z );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd::rotate( osg::inDegrees( -90.0f ), 1.0, 0.0, 0.0 ) );
    }

    // bottom face
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setGraphicsContext( _gwqt.get() );
        camera->setName( "Bottom face camera" );
        camera->setViewport( new osg::Viewport( 0, 0, tex_width, tex_height ) );
        camera->setDrawBuffer( buffer );
        camera->setReadBuffer( buffer );
        camera->setAllowEventFocus( false );
        camera->setCullMask( ~INVISIBLE_MASK );

        camera->setClearColor( clearColour );
        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( renderTargetImplementation );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::COLOR_BUFFER, _cubeTexture, 0, osg::TextureCubeMap::NEGATIVE_Z );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd::rotate( osg::inDegrees( 90.0f ), 1.0, 0.0, 0.0 ) * osg::Matrixd::rotate( osg::inDegrees( 180.0f ), 0.0, 0.0, 1.0 ) );
    }

    getCamera()->setProjectionMatrixAsPerspective( 90.0f, 1.0, 1, 1000.0 );


    // distortion correction set up.
    {
        osg::Geode* geode = new osg::Geode();
        geode->addDrawable( createSkyboxDistortionMesh( osg::Vec3( 0.0f, 0.0f, 0.0f ), osg::Vec3( _width, 0.0f, 0.0f ), osg::Vec3( 0.0f, _height, 0.0f ) ) );

        // new we need to add the texture to the mesh, we do so by creating a
        // StateSet to contain the Texture StateAttribute.
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setTextureAttributeAndModes( 0, _cubeTexture, osg::StateAttribute::ON );
        stateset->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setGraphicsContext( _gwqt );
        camera->setClearMask( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
        camera->setClearColor( clearColour );
        camera->setViewport( new osg::Viewport( 0, 0, _width, _height ) );
        GLenum cbuffer = _traits->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer( cbuffer );
        camera->setReadBuffer( cbuffer );
        camera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
        camera->setAllowEventFocus( false );
        //camera->setInheritanceMask(camera->getInheritanceMask() & ~osg::CullSettings::CLEAR_COLOR & ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        camera->setProjectionMatrixAsOrtho2D( 0, _width, 0, _height );
        camera->setViewMatrix( osg::Matrix::identity() );

        // add subgraph to render
        camera->addChild( geode );

        camera->setName( "DistortionCorrectionCamera" );

        addSlave( camera.get(), osg::Matrixd(), osg::Matrixd(), false );
    }

    getCamera()->setNearFarRatio( 0.0001f );

    // set up the hud camera
    return distortionNode;
}

///////////////////////////////////////////////////////////////////////////
// in-line GLSL source code for the "microshader" example

static const char* shaderVertSource =
{
    "// passthru - Simple pass through vertex shader\n"
    "void main(void)\n"
    "{\n"
    "    gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n"
    "    gl_Position = ftransform();\n"
    "}\n"
};

static const char* shaderFragSource =
{
    "uniform sampler2D texture1;\n"
    "uniform sampler2D texture2;\n"
    "uniform sampler2D texture3;\n"
    "void main(void)\n"
    "{\n"
    "    vec4  color1;\n"
    "    vec4  color2;\n"
    "    vec4  color3;\n"
    "    float  lum1;\n"
    "    float  lum2;\n"
    "    float  lum3;\n"
    "    color1 = texture2D( texture1, gl_TexCoord[0].st );\n"
    "    lum1 = color1.r * 0.2989 + color1.g * 0.5870 + color1.b * 0.1140;\n"
    "    color2 = texture2D( texture2, gl_TexCoord[0].st );\n"
    "    lum2 = color2.r * 0.2989 + color2.g * 0.5870 + color2.b * 0.1140;\n"
    "    color3 = texture2D( texture3, gl_TexCoord[0].st );\n"
    "    lum3 = color3.r * 0.2989 + color3.g * 0.5870 + color3.b * 0.1140;\n"
    "    gl_FragColor = vec4( lum1, lum2, lum3, 1.0 );\n"
    "}\n"
};

///////////////////////////////////////////////////////////////////////////

osg::Node*
Viewing_Window_Qt::createDistortionSubgraphWithPacking( int index, int idx,
                                                        osg::Node* subgraph,
                                                        const osg::Vec4& clearColour,
                                                        bool addHud )
{
    osg::Group* distortionNode = new osg::Group;

    unsigned int tex_width = ( unsigned int ) pow( 2, ceil( log( ( float )( _traits->width ) ) / log( 2.f ) ) );
    unsigned int tex_height = ( unsigned int ) pow( 2, ceil( log( ( float )( _traits->height ) ) / log( 2.f ) ) );

    if ( tex_width > tex_height )
        tex_height = tex_width;
    else
        tex_width = tex_height;

    _texture.push_back( new osg::Texture2D );
    _texture.back()->setTextureSize( tex_width, tex_height );
    _texture.back()->setInternalFormat( GL_RGBA );
    _texture.back()->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    _texture.back()->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );

    osg::Program* program = new osg::Program;
    program->setName( "texture_shader" );
    program->addShader( new osg::Shader( osg::Shader::VERTEX, shaderVertSource ) );
    program->addShader( new osg::Shader( osg::Shader::FRAGMENT, shaderFragSource ) );

    {
        osg::Camera* camera = new osg::Camera;

        // set clear the color and depth buffer
        camera->setClearColor( clearColour );
        camera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // just inherit the main cameras view
        camera->setReferenceFrame( osg::Transform::RELATIVE_RF );
        camera->setProjectionMatrix( osg::Matrixd::identity() );
        camera->setViewMatrix( osg::Matrixd::identity() );

        // set viewport
        camera->setViewport( 0, 0, tex_width, tex_height );

        // set the camera to render before the main camera.
        camera->setRenderOrder( osg::Camera::PRE_RENDER, idx );

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );

        // attach the texture and use it as the color buffer.
        camera->attach( osg::Camera::BufferComponent( osg::Camera::COLOR_BUFFER ),
                        _texture.back() );
        // Only render visible objects
        camera->setCullMask( ~INVISIBLE_MASK );

        // add subgraph to render
        camera->addChild( subgraph );

        distortionNode->addChild( camera );
    }

    if ( idx == 2 )
    {
        // create the quad to visualize.
        osg::Geometry* polyGeom = new osg::Geometry();

        polyGeom->setSupportsDisplayList( false );

        osg::Vec3 origin( 0.0f, 0.0f, 0.0f );
        osg::Vec3 xAxis( 1.0f, 0.0f, 0.0f );
        osg::Vec3 yAxis( 0.0f, 1.0f, 0.0f );
        float height = _traits->height;
        float width = _traits->width;
        int noSteps = 50;

        osg::Vec3Array* vertices = new osg::Vec3Array;
        osg::Vec2Array* texcoords = new osg::Vec2Array;
        osg::Vec4Array* colors = new osg::Vec4Array;

        osg::Vec3 bottom = origin;
        osg::Vec3 dx = xAxis * ( width / ( ( float )( noSteps - 1 ) ) );
        osg::Vec3 dy = yAxis * ( height / ( ( float )( noSteps - 1 ) ) );

        osg::Vec2 dx_texcoord( 1.0f / ( float )( noSteps - 1 ), 0.0f );
        osg::Vec2 dy_texcoord( 0.0f, 1.0f / ( float )( noSteps - 1 ) );

        osg::Vec2 texcoord = _bottom_texcoord[index];
        int i, j;
        osg::Vec2 xy_min( 1, 1 );
        osg::Vec2 xy_max( -1, -1 );

        _graph[index]->compute_basis_matrix();

        for ( i = 0; i < noSteps; ++i )
        {
            //osg::Vec3 cursor = bottom+dy*(float)i;
            osg::Vec2 texcoord = _bottom_texcoord[index] + dy_texcoord * ( float )i;
            for ( j = 0; j < noSteps; ++j )
            {
                QPointF pt = _graph[index]->evaluate( texcoord.x() - _bottom_texcoord[index].x(),
                                                      1. - ( texcoord.y() - _bottom_texcoord[index].y() ) );
                osg::Vec3 cursor( width * pt.x(), height - ( height * pt.y() ), 0. );
                vertices->push_back( cursor );

                if ( _use_distortion[index] )
                {
                    float th = texcoord.x();
                    float h = texcoord.y() / _y_focal_length[index];
                    float xh = sinf( th / _x_focal_length[index] );
                    float yh = h;
                    float zhx = cosf( th / _x_focal_length[index] );
                    float zhy;
                    if ( _x_center_distorts_y )
                        zhy = cosf( th / _y_focal_length[index] );
                    else
                        zhy = cosf( ( th -  _bottom_texcoord[index].x() - 0.5f ) / _y_focal_length[index] );

                    osg::Vec2 texcoordh( _x_focal_length[index] * xh / zhx - _bottom_texcoord[index].x(),
                                         _y_focal_length[index] * yh / zhy - _bottom_texcoord[index].y() );

                    xy_min.x() = min( xy_min.x(), texcoordh.x() );
                    xy_max.x() = max( xy_max.x(), texcoordh.x() );
                    xy_min.y() = min( xy_min.y(), texcoordh.y() );
                    xy_max.y() = max( xy_max.y(), texcoordh.y() );

                    texcoords->push_back( texcoordh );
                }
                else
                    texcoords->push_back( osg::Vec2( texcoord.x() - _bottom_texcoord[index].x(),
                                                     texcoord.y() - _bottom_texcoord[index].y() ) );
                colors->push_back( _global_brightness[ index ] );

                //              cursor += dx;
                texcoord += dx_texcoord;
            }
        }

        if ( _use_distortion[index] )
        {
            osg::Vec2 scale( 1.f / ( xy_max.x() - xy_min.x() ),  1.f / ( xy_max.y() - xy_min.y() ) );

            for ( osg::Vec2Array::iterator it = texcoords->begin(); it < texcoords->end(); it++ )
            {
                it->x() = ( it->x() - xy_min.x() ) * scale.x();
                it->y() = ( it->y() - xy_min.y() ) * scale.y();
            }
        }

        // pass the created vertex array to the points geometry object.
        polyGeom->setVertexArray( vertices );

        polyGeom->setColorArray( colors );
        polyGeom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );

        polyGeom->setTexCoordArray( 0, texcoords );


        for ( i = 0; i < noSteps - 1; ++i )
        {
            osg::DrawElementsUShort* elements = new osg::DrawElementsUShort( osg::PrimitiveSet::QUAD_STRIP );
            for ( j = 0; j < noSteps; ++j )
            {
                elements->push_back( j + ( i + 1 )*noSteps );
                elements->push_back( j + ( i )*noSteps );
            }
            polyGeom->addPrimitiveSet( elements );
        }


        // new we need to add the texture to the Drawable, we do so by creating a
        // StateSet to contain the Texture StateAttribute.
        osg::StateSet* stateset = polyGeom->getOrCreateStateSet();
        for ( int i = 0; i < 3; i++ )
            stateset->setTextureAttributeAndModes( i, _texture[ index * 3 + i ],
                                                   osg::StateAttribute::ON );
        stateset->addUniform( new osg::Uniform( "texture1", 0 ) );
        stateset->addUniform( new osg::Uniform( "texture2", 1 ) );
        stateset->addUniform( new osg::Uniform( "texture3", 2 ) );
        stateset->setAttributeAndModes( program,
                                        osg::StateAttribute::ON |
                                        osg::StateAttribute::OVERRIDE );
        stateset->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

        osg::Geode* geode = new osg::Geode();
        geode->addDrawable( polyGeom );

        // set up the camera to render the textured quad
        osg::Camera* camera = new osg::Camera;

        // just inherit the main cameras view
        camera->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
        camera->setViewMatrix( osg::Matrix::identity() );
        camera->setProjectionMatrixAsOrtho2D( 0, _traits->width, 0, _traits->height );

        // set the camera to render before the main camera.
        camera->setRenderOrder( osg::Camera::NESTED_RENDER );

        // add subgraph to render
        camera->addChild( geode );
        if ( addHud )
            camera->addChild( _hud );

        distortionNode->addChild( camera );
    }
    return distortionNode;
}

osgQt::GraphicsWindowQt*
Viewing_Window_Qt::multipleWindowWithDistortion( int i, int num_displays,
                                                 int starting_display )
{
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();

    _multipleScreens = num_displays > 1;

    osg::GraphicsContext::ScreenIdentifier si;
    si.readDISPLAY();

    // displayNum has not been set so reset it to 0.
    if ( si.displayNum < 0 ) si.displayNum = 0;
    si.screenNum = starting_display;

    wsi->getScreenResolution( si, _width, _height );

    _traits = new osg::GraphicsContext::Traits;
    _traits->hostName = si.hostName;
    _traits->displayNum = si.displayNum;

    if ( _multipleScreens )
    {
        _traits->screenNum = si.screenNum + i;
        if ( _custom_widget_enabled )
            _traits->x = 0;
        else
        {
            QRect screenres = QApplication::desktop()->screenGeometry( _traits->screenNum );
            _traits->x = screenres.x();
        }
        _traits->width = _width;
    }
    else
    {
        _traits->screenNum = si.screenNum;
        if ( _custom_widget_enabled )
            _traits->x = ( i * _width ) / num_displays;
        else
        {
            QRect screenres = QApplication::desktop()->screenGeometry( _traits->screenNum );
            _traits->x = screenres.x() + ( i * _width ) / num_displays;
        }
        _traits->width = _width / num_displays;
    }
    _traits->y = 0;
    _traits->height = _height;
    _traits->windowDecoration = false;
    _traits->doubleBuffer = true;
    _traits->sharedContext = 0;
    _traits->vsync = 1;
    _traits->supportsResize = false;

    _gwqt = new osgQt::GraphicsWindowQt( _traits.get(), 0, 0, Qt::FramelessWindowHint );
    _gwqt->setClearColor( _clear_color );
    _gwqt->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    return _gwqt;
}
