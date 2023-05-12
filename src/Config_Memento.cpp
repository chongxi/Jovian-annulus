// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015

#ifndef Q_MOC_RUN
#define WIN32_LEAN_AND_MEAN
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace bpt = boost::property_tree;
#endif

#include <iostream>
using std::cout;
using std::endl;
#include <utility> // for make_pair

#include <Config_Memento.h>
#include <Console.h>
#include <ui_console.h>

template <typename T>
std::vector<T> as_vector( bpt::ptree const& pt, bpt::ptree::key_type const& key )
{
    std::vector<T> r;
    BOOST_FOREACH ( bpt::ptree::value_type const & item, pt.get_child( key ) )
    {
        r.push_back( item.second.get_value<T>() );
    }
    return r;
}

template <typename T>
void to_vector ( bpt::ptree& pt, int level, std::vector<T>& v )
{
    if ( pt.empty() )
    {
        v.push_back( pt.get_value<T>() );
    }
    else
    {
        for ( bpt::ptree::iterator pos = pt.begin(); pos != pt.end(); )
        {
            to_vector( pos->second, level + 1, v );
            ++pos;
        }
    }
    return;
}

std::string indent( int level )
{
    std::string s;
    for ( int i = 0; i < level; i++ ) s += "  ";
    return s;
}

void printTree ( bpt::ptree& pt, int level )
{
    if ( pt.empty() )
    {
        std::cerr << "\"" << pt.data() << "\"";
    }
    else
    {
        if ( level ) std::cerr << endl;
        std::cerr << indent( level ) << "{" << endl;
        for ( bpt::ptree::iterator pos = pt.begin(); pos != pt.end(); )
        {
            std::cerr << indent( level + 1 ) << "\"" << pos->first << "\": ";
            printTree( pos->second, level + 1 );
            ++pos;
            if ( pos != pt.end() )
            {
                std::cerr << ",";
            }
            std::cerr << std::endl;
        }
        std::cerr << indent( level ) << " }";
    }
    return;
}

// Initialization
Config_Memento::Config_Memento()
    : _initialized( false ), _major_version( 2 ), _minor_version( 11 ), _light_info( 8 )
{
}

Config_Memento::Config_Memento( Console_Ptr console )
    : _initialized( false ), _major_version( 2 ), _minor_version( 11 ), _light_info( 8 )
{
   initialize( console.get() );
}

// Duplication
// Element Change
void
Config_Memento::initialize( Console* console )
{
    _numCameras = console->_ui->numDisplaysSpinBox->value();
    _num_displays = console->_ui->numDisplaysSpinBox->value();
    _starting_display = console->_ui->startingDisplaySpinBox->value();

    _inner_radii = console->_ui->segmentInnerRadius->value();
    _outer_radii = console->_ui->segmentOuterRadius->value();

    _annulus_center_x = 0;
    _annulus_center_y = 0;

    _retries = console->retries();
    from_string<int>( _port, console->port(), std::dec );
    _host = console->hostname();
    has_lighting_info = false;
    _initialized = true;
}

// Basic operations
void
Config_Memento::load( std::ifstream& config )
{
    if ( config.peek() == '{' )
        read( config );
    else
    {
        char v[256];
        char ch;
        config >> v;
        std::string str( v );
        int pos = str.find( "." );
        from_string<int>( _major, str.substr( 0, pos ), std::dec );
        from_string<int>( _minor, str.substr( pos + 1 ), std::dec );

        if ( _major == 1 && _minor == 0 )
        {
            cout << "Can only process version 1.1 or later configuration files." << endl;
            return;
        }

        if ( !( _major <= _major_version || ( _major_version == _major && _minor_version <= _minor ) ) )
        {
            cout << "Can only process version 1.1 or later configuration files." << endl;
            return;
        }

        if ( _major == 1 )
            read_v1( config );
        else if ( _major == 2 )
            read_v2( config );
    }
}

void
Config_Memento::save( std::ofstream& config )
{
    bpt::ptree tree;
    tree.put( "jovian.major_version", _major_version );
    tree.put( "jovian.minor_version", _minor_version );
    tree.put( "jovian.number_of_cameras", _numCameras );
    tree.put( "jovian.number_of_displays", _num_displays );
    tree.put( "jovian.starting_display", _starting_display );

    // ----------- Setup Parameters ----------------------------
    tree.put( "jovian.setup.gang_motion_and_blanking", _gangMotion );
    tree.put( "jovian.setup.gang_cameras", _gangCameras );
    tree.put( "jovian.setup.x_center_distors_y", _xCenterDistorts );

    {
        bpt::ptree node;
        {
            bpt::ptree cam_node;
            for ( int i = 0; i < _numCameras; i++ )
            {
                bpt::ptree element, sub_node;

                element.put_value( _frame[i].first );
                sub_node.push_back( std::make_pair( "", element ) );
                element.put_value( _frame[i].second );
                sub_node.push_back( std::make_pair( "", element ) );
                std::ostringstream oss;
                oss << "camera_";
                oss << ( i + 1 );

                cam_node.put_child( bpt::ptree::path_type( oss.str().c_str() ), sub_node );
            }
            node.put_child( bpt::ptree::path_type( "horizontal.blanking" ), cam_node );
        }
        {
            bpt::ptree cam_node;
            for ( int i = 0; i < _numCameras; i++ )
            {
                bpt::ptree sub_node;

                sub_node.put( "enable", _distort[i].first );
                sub_node.put( "x_focal_length", _distort[i].second );
                sub_node.put( "y_focal_length", _distort[i].third );
                sub_node.put( "x_center", _center[i].first );
                sub_node.put( "y_center", _center[i].second );
                std::ostringstream oss;
                oss << "camera_";
                oss << ( i + 1 );

                cam_node.put_child( bpt::ptree::path_type( oss.str().c_str() ), sub_node );
            }
            node.put_child( bpt::ptree::path_type( "warping" ), cam_node );
        }
        tree.put_child( bpt::ptree::path_type( "jovian.setup.distortion" ), node );
    }
    {
        bpt::ptree node;
        node.put( "type", _connection_type );
        {
            bpt::ptree sub_node;

            sub_node.put( "hostname", _host );
            sub_node.put( "connection_port", _port );
            sub_node.put( "number_of_retries", _retries );
            node.put_child( bpt::ptree::path_type( "parameters" ), sub_node );
        }
        tree.put_child( bpt::ptree::path_type( "jovian.setup.connection" ), node );
    }

    // ----------- Display Parameters ----------------------------
    tree.put( "jovian.display.frame_packing", _frame_packing );
    {
        bpt::ptree node;
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _use_annulus );
            node.put_child( bpt::ptree::path_type( "use_annulus" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "inner", _annulus_inner_radius );
            sub_node.put( "outer", _annulus_outer_radius );
            node.put_child( bpt::ptree::path_type( "radius" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "minor", _annulus_minor_axis );
            sub_node.put( "major", _annulus_major_axis );
            node.put_child( bpt::ptree::path_type( "axis" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "x", _annulus_center_x );
            sub_node.put( "y", _annulus_center_y );
            node.put_child( bpt::ptree::path_type( "center" ), sub_node );
        }
        tree.put_child( bpt::ptree::path_type( "jovian.display.annulus" ), node );
    }
    tree.put( "jovian.display.__comment__",
              "This next section describes the positions of the controls in the display widgets."
              " The bottom left point is the origin, and the first point in the list." );
    {
        bpt::ptree node, cam_node;
        for ( int i = 0; i < _numCameras; i++ )
        {
            bpt::ptree grid;
            {
                bpt::ptree element, sub_node;

                element.put_value( _graph[i].size() );
                sub_node.push_back( std::make_pair( "", element ) );
                element.put_value( _graph[i][0].size() );
                sub_node.push_back( std::make_pair( "", element ) );
                grid.put_child( bpt::ptree::path_type( "size" ), sub_node );
            }
            bpt::ptree row;
            for ( int j = 0; j < _graph[i].size(); j++ )
            {
                bpt::ptree col;
                for ( int k = 0; k < _graph[i][0].size(); k++ )
                {
                    bpt::ptree element, sub_node;
                    element.put_value( _graph[i][j][k].first );
                    sub_node.push_back( std::make_pair( "", element ) );
                    element.put_value( _graph[i][j][k].second );
                    sub_node.push_back( std::make_pair( "", element ) );
                    col.push_back( std::make_pair( "", sub_node ) );
                }
                std::ostringstream oss;
                oss << "row_";
                oss << ( j + 1 );
                row.push_back( std::make_pair( oss.str().c_str(), col ) );
            }
            grid.put_child( bpt::ptree::path_type( "points" ), row );
            node.put_child( bpt::ptree::path_type( "grid" ), grid );
            std::ostringstream oss;
            oss << "camera_";
            oss << ( i + 1 );

            cam_node.put_child( bpt::ptree::path_type( oss.str().c_str() ), node );
        }
        tree.put_child( bpt::ptree::path_type( "jovian.display.configuration" ), cam_node );
    }

    // ----------- Lighting Parameters ----------------------------
    tree.put( "jovian.lighting.ambient_color", _ambient.name().toStdString() );
    tree.put( "jovian.lighting.background_color", _background.name().toStdString() );
    bpt::ptree cam_node;
    {
        for ( int i = 0; i < _numCameras; i++ )
        {
            bpt::ptree sub_node;

            sub_node.put( "", _global_brightness[i].name().toStdString() );
            std::ostringstream oss;
            oss << "camera_";
            oss << ( i + 1 );

            cam_node.put_child( bpt::ptree::path_type( oss.str().c_str() ), sub_node );
        }
    }
    tree.put_child( bpt::ptree::path_type( "jovian.lighting.global_brightness" ), cam_node );
    bpt::ptree light_node;
    {
        for ( int i = 0; i < 8; i++ )
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _light_info[i].enabled );
            sub_node.put( "x", _light_info[i].x_pos );
            sub_node.put( "y", _light_info[i].y_pos );
            sub_node.put( "z", _light_info[i].z_pos );
            sub_node.put( "color", _light_info[i].color );
            sub_node.put( "intensity", _light_info[i].power );
            std::ostringstream oss;
            oss << "light_";
            oss << ( i + 1 );

            light_node.put_child( bpt::ptree::path_type( oss.str().c_str() ), sub_node );
        }
    }
    tree.put_child( bpt::ptree::path_type( "jovian.lighting.light_information" ), light_node );
    tree.put( "jovian.lighting.shading_method", _shader_choice );
    tree.put( "jovian.lighting.texture_resolution", _texture_resolution );

    // ----------- Calibration Parameters ----------------------------
    tree.put( "jovian.calibration.average_multiple_calibration_runs", _average_runs );
    {
        bpt::ptree sub_node;

        sub_node.put( "value", _calib_x );
        sub_node.put( "gain", _gain_x );
        tree.put_child( bpt::ptree::path_type( "jovian.calibration.x_axis" ), sub_node );
    }
    {
        bpt::ptree sub_node;

        sub_node.put( "value", _calib_y );
        sub_node.put( "gain", _gain_y );
        tree.put_child( bpt::ptree::path_type( "jovian.calibration.y_axis" ), sub_node );
    }
    {
        bpt::ptree sub_node;

        sub_node.put( "value", _calib_z );
        sub_node.put( "gain", _gain_z );
        tree.put_child( bpt::ptree::path_type( "jovian.calibration.z_axis" ), sub_node );
    }

    // ----------- Configuration Parameters ----------------------------
    {
        bpt::ptree sub_node;

        sub_node.put( "enabled", _enable_vel_smoothing );
        sub_node.put( "interval", _vel_smoothing_interval );
        tree.put_child( bpt::ptree::path_type( "jovian.configuration.smoothing.velocity" ), sub_node );
    }
    {
        bpt::ptree node;
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _enable_segment_smoothing );
            sub_node.put( "interval", _segment_interval );
            node.put_child( bpt::ptree::path_type( "segment" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _enable_input_smoothing );
            sub_node.put( "interval", _input_interval );
            node.put_child( bpt::ptree::path_type( "input" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _enable_intermediary_smoothing );
            sub_node.put( "interval", _intermediary_interval );
            node.put_child( bpt::ptree::path_type( "intermediary" ), sub_node );
        }
        {
            bpt::ptree sub_node;

            sub_node.put( "enabled", _enable_output_smoothing );
            sub_node.put( "interval", _output_interval );
            node.put_child( bpt::ptree::path_type( "output" ), sub_node );
        }
        tree.put_child( bpt::ptree::path_type( "jovian.configuration.smoothing.heading" ), node );
    }
    tree.put( "jovian.configuration.miscellaneous.ball_radius", _ball_radius );
    {
        bpt::ptree element, sub_node;

        element.put_value( _inner_radii );
        sub_node.push_back( std::make_pair( "", element ) );
        element.put_value( _outer_radii );
        sub_node.push_back( std::make_pair( "", element ) );
        tree.put_child( bpt::ptree::path_type( "jovian.configuration.miscellaneous.segment_radii" ), sub_node );
    }
    {
        bpt::ptree element, sub_node;

        element.put_value( _x_offset );
        sub_node.push_back( std::make_pair( "", element ) );
        element.put_value( _y_offset );
        sub_node.push_back( std::make_pair( "", element ) );
        element.put_value( _z_offset );
        sub_node.push_back( std::make_pair( "", element ) );
        tree.put_child( bpt::ptree::path_type( "jovian.configuration.miscellaneous.camera_offset" ), sub_node );
    }
    {
        bpt::ptree sub_node;

        sub_node.put( "enabled", _crossbar_enabled );
        sub_node.put( "width", _crossbar_width );
        tree.put_child( bpt::ptree::path_type( "jovian.configuration.miscellaneous.crossbar" ), sub_node );
    }
    tree.put( "jovian.configuration.miscellaneous.minimum_velocity_thresold", _minimum_velocity_thresold );
    tree.put( "jovian.configuration.miscellaneous.restrict_vertical_motion", _restrict_vertical_motion );

    tree.put( "jovian.configuration.io.data_server_port", _data_server_port_name );
    tree.put( "jovian.configuration.io.python_command_port", _command_port_name );
    tree.put( "jovian.configuration.io.output_port", _output_port_name );
    tree.put( "jovian.configuration.io.output_rate", _output_rate );
    tree.put( "jovian.configuration.io.output_format", _output_format_id );
    tree.put( "jovian.configuration.io.treadmill_output", _treadmill_output );
    tree.put( "jovian.configuration.io.reduced_output", _reduced_output );

    tree.put( "jovian.configuration.rotated_cameras.__comment__",
              "When used with the annulus code, 'enable' is unused, and 'offset' is the vertical FOV." );

    tree.put( "jovian.configuration.rotated_cameras.enable", _rotatedCameras );
    tree.put( "jovian.configuration.rotated_cameras.field_of_view", _fov );
    tree.put( "jovian.configuration.rotated_cameras.offset", _fov_offset );

    tree.put( "jovian.configuration.frame_indicator.mode", _frame_mode );
    tree.put( "jovian.configuration.frame_indicator.size", _indicator_size );
    tree.put( "jovian.configuration.frame_indicator.border", _border_size );
    tree.put( "jovian.configuration.frame_indicator.screen", _screen_select );
    tree.put( "jovian.configuration.frame_indicator.horizontal_position", _indicator_horizontal_pos );
    tree.put( "jovian.configuration.frame_indicator.vertical_position", _indicator_vertical_pos );

    // ----------- Heading Direction Parameters ----------------------------
    tree.put( "jovian.heading_direction.heading_direction_control.enable", _enable_threshold_turning );
    tree.put( "jovian.heading_direction.heading_direction_control.min_threshold", _min_threshold_scale );
    tree.put( "jovian.heading_direction.heading_direction_control.max_threshold", _max_threshold_scale );
    tree.put( "jovian.heading_direction.heading_direction_control.manual_heading_direction_contribution", _current_threshold_scale );
    tree.put( "jovian.heading_direction.heading_direction_control.auto_heading_turning_rate", _auto_heading_turning_rate );

    {
        bpt::ptree plot;
        {
            bpt::ptree element;

            element.put_value( _plot_x.size() );
            plot.put_child( bpt::ptree::path_type( "size" ), element );
        }
        bpt::ptree points;
        for ( int j = 0; j < _plot_x.size(); j++ )
        {
            bpt::ptree element, sub_node;
            element.put_value( _plot_x[j] );
            sub_node.push_back( std::make_pair( "", element ) );
            element.put_value( _plot_y[j] );
            sub_node.push_back( std::make_pair( "", element ) );
            points.push_back( std::make_pair( "", sub_node ) );
        }
        plot.put_child( bpt::ptree::path_type( "points" ), points );
        tree.put_child( bpt::ptree::path_type( "jovian.heading_direction.pitch_roll_vs_degrees_per_sec_plot" ), plot );
    }

    {
        bpt::ptree plot;
        {
            bpt::ptree element;

            element.put_value( _plot_vel_x.size() );
            plot.put_child( bpt::ptree::path_type( "size" ), element );
        }
        bpt::ptree points;
        for ( int j = 0; j < _plot_vel_x.size(); j++ )
        {
            bpt::ptree element, sub_node;
            element.put_value( _plot_vel_x[j] );
            sub_node.push_back( std::make_pair( "", element ) );
            element.put_value( _plot_vel_y[j] );
            sub_node.push_back( std::make_pair( "", element ) );
            points.push_back( std::make_pair( "", sub_node ) );
        }
        plot.put_child( bpt::ptree::path_type( "points" ), points );
        tree.put_child( bpt::ptree::path_type( "jovian.heading_direction.treadmill_speed_vs_manual_heading" ), plot );
    }
    bpt::write_json( config, tree );

    /*
    config << _start_location << endl;

    */
}

// Implementation
void
Config_Memento::read( std::ifstream& config )
{
    bpt::ptree tree;
    bpt::read_json( config, tree );

    int major_version  = tree.get( "jovian.major_version", -1 );
    int minor_version  = tree.get( "jovian.minor_version", -1 );
    _numCameras  = tree.get( "jovian.number_of_cameras", 1 );
    _num_displays  = tree.get( "jovian.number_of_displays", 1 );
    _starting_display  = tree.get( "jovian.starting_display", 1 );

    // ----------- Setup Parameters ----------------------------
    _gangMotion  = tree.get( "jovian.setup.gang_motion_and_blanking", false );
    _gangCameras  = tree.get( "jovian.setup.gang_cameras", false );
    _xCenterDistorts  = tree.get( "jovian.setup.x_center_distors_y", false );

    _center.resize( _numCameras );
    _distort.resize( _numCameras );
    _frame.resize( _numCameras );
    _global_brightness.resize( _numCameras );

    {
        bpt::ptree node = tree.get_child( bpt::ptree::path_type( "jovian.setup.distortion" ) );

        {
            bpt::ptree cam_node = node.get_child( bpt::ptree::path_type( "horizontal.blanking" ) );

            for ( int i = 0; i < _numCameras; i++ )
            {
                std::ostringstream oss;
                oss << "camera_";
                oss << ( i + 1 );

                std::vector<int> v = as_vector<int>( cam_node, oss.str().c_str() );
                _frame[i].first = v[0];
                _frame[i].second = v[1];
            }
        }
        {
            bpt::ptree cam_node = node.get_child( bpt::ptree::path_type( "warping" ) );
            for ( int i = 0; i < _numCameras; i++ )
            {
                std::ostringstream oss;
                oss << "camera_";
                oss << ( i + 1 );

                bpt::ptree sub_node = cam_node.get_child( bpt::ptree::path_type( oss.str().c_str() ) );

                _distort[i].first  = sub_node.get( "enable", -1 );
                _distort[i].second  = sub_node.get( "x_focal_length", -1 );
                _distort[i].third  = sub_node.get( "y_focal_length", -1 );
                _center[i].first  = sub_node.get( "x_center", -1 );
                _center[i].second  = sub_node.get( "y_center", -1 );
            }
        }
        {
            bpt::ptree node = tree.get_child( bpt::ptree::path_type( "jovian.setup.connection" ) );
            _connection_type = node.get( "type", "TCP" );
            {
                bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "parameters" ) );

                _host  = sub_node.get( "hostname", "localhost" );
                _port  = sub_node.get( "connection_port", 22222 );
                _retries  = sub_node.get( "number_of_retries", 3 );
            }
        }
    }

    // ----------- Display Parameters ----------------------------
    _frame_packing  = tree.get( "jovian.display.frame_packing", -1 );
    {
        bpt::ptree node = tree.get_child( bpt::ptree::path_type( "jovian.display.annulus" ) );
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "use_annulus" ) );

            _use_annulus  = sub_node.get( "enabled", false );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "radius" ) );

            _annulus_inner_radius  = sub_node.get( "inner", 0.2 );
            _annulus_outer_radius  = sub_node.get( "outer", 1.0 );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "axis" ) );

            _annulus_minor_axis  = sub_node.get( "minor", 1.0 );
            _annulus_major_axis  = sub_node.get( "major", 1.0 );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "center" ) );

            _annulus_center_x  = sub_node.get( "x", 0 );
            _annulus_center_y  = sub_node.get( "y", 0 );
        }
    }

    {
        bpt::ptree node, cam_node = tree.get_child( bpt::ptree::path_type( "jovian.display.configuration" ) );

        for ( int i = 0; i < _numCameras; i++ )
        {
            std::ostringstream oss;
            oss << "camera_";
            oss << ( i + 1 );

            node = cam_node.get_child( bpt::ptree::path_type( oss.str().c_str() ) );

            bpt::ptree grid = node.get_child( bpt::ptree::path_type( "grid" ) );
            std::vector<int> sizes = as_vector<int>( grid, "size" );
            bpt::ptree points = grid.get_child( bpt::ptree::path_type( "points" ) );

            GraphPositions g;
            g.resize( sizes[0] );

            for ( int k = 0; k < sizes[0]; k++ )
            {
                std::ostringstream oss;
                oss << "row_";
                oss << ( k + 1 );
                bpt::ptree row = points.get_child( oss.str().c_str() );
                std::vector<float> v;
                to_vector<float>( row, 0, v );

                qreal x, y;
                g[k].resize( sizes[1] );

                for ( int j = 0; j < sizes[1]; j++ )
                {
                    g[k][j] = std::make_pair( v[2 * j], v[2 * j + 1] );
                }
            }

            _graph.push_back( g );
        }
    }

    // ----------- Lighting Parameters ----------------------------
    _ambient = QColor( tree.get( "jovian.lighting.ambient_color", "white" ).c_str() );
    _diffuse = QColor( tree.get( "jovian.lighting.diffuse_color", "white" ).c_str() );
    _power = tree.get( "jovian.lighting.intensity", 10 );
    try
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.lighting.lighting_grid" ) );
        _light_rows = sub_node.get( "rows", 2 );
        _light_cols = sub_node.get( "columns", 2 );
        _override_height = sub_node.get( "override_height", false );
        _light_height = sub_node.get( "height", 1.0 );
    }
    catch ( ... )
    {
        std::cerr << "Config file missing lighting_grid section" << std::endl;
        _light_rows = 2;
        _light_cols = 2;
        _override_height = false;
        _light_height = 1.0;
    }

    try
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.lighting.attenuation" ) );
        _constant = sub_node.get( "constant", 0 );
        _linear = sub_node.get( "linear", 0 );
        _quadratic = sub_node.get( "quadratic", 0 );
    }
    catch ( ... )
    {
        std::cerr << "Config file missing attenuation section" << std::endl;
        _constant = 0;
        _linear = 0;
        _quadratic = 0;
    }

    _background = QColor( tree.get( "jovian.lighting.background_color", "black" ).c_str() );
    bpt::ptree cam_node = tree.get_child( bpt::ptree::path_type( "jovian.lighting.global_brightness" ) );
    for ( int i = 0; i < _numCameras; i++ )
    {
        std::ostringstream oss;
        oss << "camera_";
        oss << ( i + 1 );

        _global_brightness[ i ] = QColor( cam_node.get<std::string>( oss.str().c_str() ).c_str() );
    }

    try
    {
       bpt::ptree light_node =
           tree.get_child( bpt::ptree::path_type( "jovian.lighting.light_information" ) );
       {
          for ( int i = 0; i < 8; i++ )
          {
             std::ostringstream oss;
             oss << "light_";
             oss << ( i + 1 );
             bpt::ptree sub_node =
                 light_node.get_child( bpt::ptree::path_type( oss.str().c_str() ) );

             _light_info[ i ].enabled = sub_node.get( "enabled", false );
             _light_info[ i ].x_pos = sub_node.get( "x", 0.f );
             _light_info[ i ].y_pos = sub_node.get( "y", 0.f );
             _light_info[ i ].z_pos = sub_node.get( "z", 0.f );
             _light_info[ i ].color = sub_node.get( "color", "#191919" );
             _light_info[ i ].power = sub_node.get( "intensity", 0 );
          }
       }
       has_lighting_info = true;
    }
    catch ( ... )
    {
       std::cerr << "Config file missing lighting information section" << std::endl;
       has_lighting_info = false;
    }

    _shader_choice = tree.get( "jovian.lighting.shading_method", "OSG Default" );
    _texture_resolution  = tree.get( "jovian.lighting.texture_resolution", "Computed" );

    // ----------- Calibration Parameters ----------------------------
    _average_runs  = tree.get( "jovian.calibration.average_multiple_calibration_runs", -1 );
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.calibration.x_axis" ) );

        _calib_x  = sub_node.get( "value", 0.0 );
        _gain_x  = sub_node.get( "gain", 1.0 );
    }
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.calibration.y_axis" ) );

        _calib_y  = sub_node.get( "value", 0.0 );
        _gain_y  = sub_node.get( "gain", 1.0 );
    }
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.calibration.z_axis" ) );

        _calib_z  = sub_node.get( "value", 0.0 );
        _gain_z  = sub_node.get( "gain", 1.0 );
    }

    // ----------- Configuration Parameters ----------------------------
    {
        bpt::ptree node = tree.get_child( bpt::ptree::path_type( "jovian.configuration.smoothing.velocity" ) );

        _enable_vel_smoothing  = node.get( "enabled", false );
        _vel_smoothing_interval  = node.get( "interval", 0 );
    }
    {
        bpt::ptree node = tree.get_child( bpt::ptree::path_type( "jovian.configuration.smoothing.heading" ) );
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "segment" ) );

            _enable_segment_smoothing  = sub_node.get( "enabled", false );
            _segment_interval  = sub_node.get( "interval", 0 );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "input" ) );

            _enable_input_smoothing  = sub_node.get( "enabled", false );
            _input_interval  = sub_node.get( "interval", 0 );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "intermediary" ) );

            _enable_intermediary_smoothing  = sub_node.get( "enabled", false );
            _intermediary_interval  = sub_node.get( "interval", 0 );
        }
        {
            bpt::ptree sub_node = node.get_child( bpt::ptree::path_type( "output" ) );

            _enable_output_smoothing  = sub_node.get( "enabled", false );
            _output_interval  = sub_node.get( "interval", 0 );
        }
    }
    _ball_radius  = tree.get( "jovian.configuration.miscellaneous.ball_radius", 1.0 );
    {
        std::vector<float> v = as_vector<float>( tree, "jovian.configuration.miscellaneous.segment_radii" );

        _inner_radii = v[0];
        _outer_radii = v[1];
    }
    {
        std::vector<float> v = as_vector<float>( tree, "jovian.configuration.miscellaneous.camera_offset" );
        bpt::ptree element, sub_node = tree.get_child( bpt::ptree::path_type( "jovian.configuration.miscellaneous.camera_offset" ) );

        _x_offset = v[0];
        _y_offset = v[1];
        _z_offset = v[2];
    }
    {
        bpt::ptree sub_node = tree.get_child( bpt::ptree::path_type( "jovian.configuration.miscellaneous.crossbar" ) );

        _crossbar_enabled  = sub_node.get( "enabled", -1 );
        _crossbar_width  = sub_node.get( "width", -1 );
    }
    _minimum_velocity_thresold  = tree.get( "jovian.configuration.miscellaneous.minimum_velocity_thresold", 0.001 );
    _restrict_vertical_motion  = tree.get( "jovian.configuration.miscellaneous.restrict_vertical_motion", false );

    _data_server_port_name  = tree.get( "jovian.configuration.io.data_server_port", "" );
    _command_port_name  = tree.get( "jovian.configuration.io.python_command_port", "" );
    _output_port_name  = tree.get( "jovian.configuration.io.output_port", "" );
    _output_rate  = tree.get( "jovian.configuration.io.output_rate", 60 );
    _output_format_id  = tree.get( "jovian.configuration.io.output_format", 0 );
    _treadmill_output  = tree.get( "jovian.configuration.io.treadmill_output", false );
    _reduced_output  = tree.get( "jovian.configuration.io.reduced_output", false );

    _rotatedCameras  = tree.get( "jovian.configuration.rotated_cameras.enable", true );
    _fov  = tree.get( "jovian.configuration.rotated_cameras.field_of_view", 90.0 );
    _fov_offset  = tree.get( "jovian.configuration.rotated_cameras.offset", 56.4 );

    _frame_mode  = tree.get( "jovian.configuration.frame_indicator.mode", 1 );
    _indicator_size  = tree.get( "jovian.configuration.frame_indicator.size", 200 );
    _border_size  = tree.get( "jovian.configuration.frame_indicator.border", 20 );
    _screen_select  = tree.get( "jovian.configuration.frame_indicator.screen", 1 );
    _indicator_horizontal_pos  = tree.get( "jovian.configuration.frame_indicator.horizontal_position", 0 );
    _indicator_vertical_pos  = tree.get( "jovian.configuration.frame_indicator.vertical_position", 0 );

    // ----------- Heading Direction Parameters ----------------------------
    _enable_threshold_turning = tree.get( "jovian.heading_direction.heading_direction_control.enable", false );
    _min_threshold_scale = tree.get( "jovian.heading_direction.heading_direction_control.min_threshold", 0.9 );
    _max_threshold_scale = tree.get( "jovian.heading_direction.heading_direction_control.max_threshold", 1.0 );
    _current_threshold_scale = tree.get( "jovian.heading_direction.heading_direction_control.manual_heading_direction_contribution", 0.95 );
    _auto_heading_turning_rate = tree.get( "jovian.heading_direction.heading_direction_control.auto_heading_turning_rate", 90.0 );

    {
        bpt::ptree plot = tree.get_child( bpt::ptree::path_type( "jovian.heading_direction.pitch_roll_vs_degrees_per_sec_plot" ) );
        {
            int v = plot.get( "size", 1 );

            _plot_x.resize( v );
            _plot_y.resize( v );
        }
        bpt::ptree points = plot.get_child( bpt::ptree::path_type( "points" ) );

        std::vector<float> v;
        to_vector<float>( points, 0, v );

        for ( int j = 0; j < _plot_x.size(); j++ )
        {
            _plot_x[j] = v[2 * j];
            _plot_y[j] = v[2 * j + 1];
        }
    }

    {
        bpt::ptree plot = tree.get_child( bpt::ptree::path_type( "jovian.heading_direction.treadmill_speed_vs_manual_heading" ) );
        {
            int v = plot.get( "size", 1 );

            _plot_vel_x.resize( v );
            _plot_vel_y.resize( v );
        }
        bpt::ptree points = plot.get_child( bpt::ptree::path_type( "points" ) );

        std::vector<float> v;
        to_vector<float>( points, 0, v );

        for ( int j = 0; j < _plot_vel_x.size(); j++ )
        {
            _plot_vel_x[j] = v[2 * j];
            _plot_vel_y[j] = v[2 * j + 1];
        }
    }
}

void
Config_Memento::read_v1( std::ifstream& config )
{
    char ch;
    int pos;

    config >> _numCameras;

    config >> _gangCameras;

    if ( _minor >= 3 )
        config >> _xCenterDistorts;

    _center.resize( _numCameras );
    _distort.resize( _numCameras );
    _frame.resize( _numCameras );
    _global_brightness.resize( _numCameras );

    for ( int i = 0; i < _numCameras; i++ )
    {
        int lower, upper;
        bool checked;
        int focalLength_y, focalLength_x;

        config >> lower >> ch >> upper;

        _frame[ i ].first = lower;
        _frame[ i ].second = upper;

        if ( _minor == 1 )
        {
            config >> ch >> checked >> ch >> focalLength_y;
            focalLength_x = focalLength_y;
        }
        else
            config >> ch >> checked >> ch >> focalLength_y >> ch >> focalLength_x;

        _distort[ i ].first = checked;
        _distort[ i ].second = focalLength_y;
        if ( _minor == 1 )
            _distort[ i ].third = focalLength_y;
        else
            _distort[ i ].third = focalLength_x;

        config >> ch >> lower >> ch >> upper;
        _center[ i ].first = lower;
        _center[ i ].second = upper;
    }

    config >> _retries >> ch >> _port >> ch >> _host;

    if ( _minor >= 9 )
        config >> _gangMotion;

    // Graph_Widget
    if ( _minor >= 6 )
    {
        for ( int i = 0; i < _numCameras; i++ )
        {
            char ch;
            int r, c;
            GraphPositions g;

            config >> r >> ch >> c;
            g.resize( r );

            for ( int i = 0; i < r; i++ )
            {
                qreal x, y;
                g[i].resize( c );

                for ( int j = 0; j < c; j++ )
                {
                    config >> ch >> x >> ch >> y >> ch >> ch;

                    g[i][j] = std::make_pair( x, y );
                }
            }

            _graph.push_back( g );
        }
    }

    if ( _minor >= 4 )
    {
        char s[256];
        std::string sss;

        // Lighting Controls
        config >> s;
        std::string ss( s );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        _ambient = QColor( sss.c_str() );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        _diffuse = QColor( sss.c_str() );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _power, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        _background = QColor( sss.c_str() );

        if ( _minor == 4 )
        {
            ss = ss.substr( pos + 1 );
            pos = ss.find( "," );
            sss = ss.substr( 0, pos );
            QColor color = QColor( sss.c_str() );
            for ( int i = 0; i < _numCameras; i++ )
                _global_brightness[ i ] = color;
        }
        else
        {
            config >> s;
            ss = std::string( s );
            for ( int i = 0; i < _numCameras; i++ )
            {
                pos = ss.find( "," );
                sss = ss.substr( 0, pos );
                _global_brightness[ i ] = QColor( sss.c_str() );
                ss = ss.substr( pos + 1 );
            }
        }

        // Calibration Parameters
        config >> s;
        ss = std::string( s );
        pos = ss.find( "," );
        from_string<float>( _calib_x, ss.substr( 0, pos ), std::dec );
        sss = ss.substr( pos + 1 );
        from_string<float>( _gain_x, sss, std::dec );

        config >> s;
        ss = std::string( s );
        pos = ss.find( "," );
        from_string<float>( _calib_y, ss.substr( 0, pos ), std::dec );
        sss = ss.substr( pos + 1 );
        from_string<float>( _gain_y, sss, std::dec );

        config >> s;
        ss = std::string( s );
        pos = ss.find( "," );
        from_string<float>( _calib_z, ss.substr( 0, pos ), std::dec );
        sss = ss.substr( pos + 1 );
        from_string<float>( _gain_z, sss, std::dec );

    }

    if ( _minor >= 5 )
    {
        double value;
        int iVal;

        if ( _minor < 8 )
        {
            // Used to be maxAnimalVelocity
            config >> value;

            // Used to be pathThreshold
            config >> value;
        }

        if ( _minor >= 7 )
            config >> _ball_radius;
        else
        {
            config >> iVal;
            _ball_radius = iVal;
        }

        config >> _output_port_name;

        config >> _rotatedCameras >> ch >> _fov >> ch >> _fov_offset;

        std::string sss;

        char s[256];
        config >> s;
        std::string ss( s );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _frame_mode, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _indicator_size, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _border_size, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _screen_select, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _indicator_horizontal_pos, sss, std::dec );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        from_string<int>( _indicator_vertical_pos, sss, std::dec );
    }
}

void
Config_Memento::read_v2( std::ifstream& config )
{
    char ch;
    int pos;

    config >> _numCameras >> ch >> _num_displays >> ch >> _starting_display;
    config >> _gangCameras;
    config >> _xCenterDistorts;

    _center.resize( _numCameras );
    _distort.resize( _numCameras );
    _frame.resize( _numCameras );
    _global_brightness.resize( _numCameras );

    for ( int i = 0; i < _numCameras; i++ )
    {
        int lower, upper;
        bool checked;
        int focalLength_y, focalLength_x;

        config >> lower >> ch >> upper;

        _frame[ i ].first = lower;
        _frame[ i ].second = upper;

        config >> ch >> checked >> ch >> focalLength_y >> ch >> focalLength_x;

        _distort[ i ].first = checked;
        _distort[ i ].second = focalLength_y;
        _distort[ i ].third = focalLength_x;

        config >> ch >> lower >> ch >> upper;
        _center[ i ].first = lower;
        _center[ i ].second = upper;
    }

    config >> _retries >> ch >> _port >> ch >> _host;

    config >> _gangMotion;
    config >> _frame_packing;

    // Graph_Widget
    for ( int i = 0; i < _numCameras; i++ )
    {
        char ch;
        int r, c;
        GraphPositions g;

        config >> r >> ch >> c;
        g.resize( r );

        for ( int i = 0; i < r; i++ )
        {
            qreal x, y;
            g[i].resize( c );

            for ( int j = 0; j < c; j++ )
            {
                config >> ch >> x >> ch >> y >> ch >> ch;

                g[i][j] = std::make_pair( x, y );
            }
        }

        _graph.push_back( g );
    }

    char s[256];
    std::string sss;

    // Lighting Controls
    config >> s;
    std::string ss( s );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    _ambient = QColor( sss.c_str() );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    _diffuse = QColor( sss.c_str() );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _power, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    _background = QColor( sss.c_str() );

    config >> s;
    ss = std::string( s );
    for ( int i = 0; i < _numCameras; i++ )
    {
        pos = ss.find( "," );
        sss = ss.substr( 0, pos );
        _global_brightness[ i ] = QColor( sss.c_str() );
        ss = ss.substr( pos + 1 );
    }

    // Calibration Parameters
    config >> s;
    ss = std::string( s );
    pos = ss.find( "," );
    from_string<float>( _calib_x, ss.substr( 0, pos ), std::dec );
    sss = ss.substr( pos + 1 );
    from_string<float>( _gain_x, sss, std::dec );

    config >> s;
    ss = std::string( s );
    pos = ss.find( "," );
    from_string<float>( _calib_y, ss.substr( 0, pos ), std::dec );
    sss = ss.substr( pos + 1 );
    from_string<float>( _gain_y, sss, std::dec );

    config >> s;
    ss = std::string( s );
    pos = ss.find( "," );
    from_string<float>( _calib_z, ss.substr( 0, pos ), std::dec );
    sss = ss.substr( pos + 1 );
    from_string<float>( _gain_z, sss, std::dec );

    if ( _minor >= 7 )
        config >> _start_location;

    double value;
    int iVal;

    config >> _enable_vel_smoothing >> ch >> _vel_smoothing_interval;
    if ( _minor >= 10 )
        config >> _enable_segment_smoothing >> ch >> _segment_interval;
    if ( _minor >= 5 )
    {
        config >> _enable_input_smoothing >> ch >> _input_interval;
        if ( _minor >= 6 )
            config >> _enable_intermediary_smoothing >> ch >> _intermediary_interval;
        else
        {
            _enable_intermediary_smoothing = false;
            _intermediary_interval = 32;
        }

        config >> _enable_output_smoothing >> ch >> _output_interval;
    }
    else
    {
        _enable_input_smoothing = _enable_output_smoothing = false;
        _input_interval = _output_interval = 32;
    }

    if ( _minor >= 4 )
    {
        config >> _ball_radius;
        if ( _minor >= 9 )
            config >> _inner_radii >> ch >> _outer_radii;
        if ( _minor >= 8 )
            config >> _x_offset >> ch >> _y_offset >> ch >> _z_offset;

        config >> _crossbar_enabled >> ch >> _crossbar_width;
        config >> _minimum_velocity_thresold;
        config >> _restrict_vertical_motion;
    }
    else
    {
        if ( _minor >= 3 )
        {
            config >> _minimum_velocity_thresold;
        }
        else
        {
            // Set sensible defaults
            _minimum_velocity_thresold = 1.e-6;
        }
        config >> _ball_radius;
        _x_offset = 0;
        _y_offset = 0;
        _z_offset = 0;
        _crossbar_width = 3.0f;
        _crossbar_enabled = false;
        _restrict_vertical_motion = false;
    }

    if ( _minor >= 7 )
    {
        config >> s;
        ss = std::string( s );
        pos = ss.find( "," );
        _output_port_name = ss.substr( 0, pos );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        _command_port_name = ss.substr( 0, pos );

        ss = ss.substr( pos + 1 );
        pos = ss.find( "," );
        _data_server_port_name = ss.substr( 0, pos );
    }
    else
        config >> _output_port_name;

    if ( _minor >= 11 )
        config >> _output_format_id;
    else
        _output_format_id = 0;

    config >> _output_rate;
    config >> _rotatedCameras >> ch >> _fov >> ch >> _fov_offset;

    config >> s;
    ss = std::string( s );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _frame_mode, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _indicator_size, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _border_size, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _screen_select, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _indicator_horizontal_pos, sss, std::dec );

    ss = ss.substr( pos + 1 );
    pos = ss.find( "," );
    sss = ss.substr( 0, pos );
    from_string<int>( _indicator_vertical_pos, sss, std::dec );

    if ( _minor >= 1 )
    {
        config >> _shader_choice;
    }

    if ( _minor >= 2 )
    {
        int r;

        if ( _minor >= 5 )
            config >> _enable_threshold_turning >> ch
                   >> _min_threshold_scale >> ch
                   >> _max_threshold_scale >> ch
                   >> _current_threshold_scale;
        else
        {
            _enable_threshold_turning = false;
            _min_threshold_scale = 0.9;
            _max_threshold_scale = 1.0;
            _current_threshold_scale = 0.95;
        }

        // Ratio plot widget
        config >> r >> ch;
        _plot_x.resize( r );
        _plot_y.resize( r );

#define degrees( _x_ ) _x_ * 180.f / M_PI

        for ( int i = 0; i < r; i++ )
        {
            double x, y;
            config >> ch >> x >> ch >> y >> ch >> ch;

            _plot_x[i] = x;
            _plot_y[i] = degrees( y );
        }

        // velocity plot widget
        config >> r >> ch;
        _plot_vel_x.resize( r );
        _plot_vel_y.resize( r );

        for ( int i = 0; i < r; i++ )
        {
            double x, y;
            config >> ch >> x >> ch >> y >> ch >> ch;

            _plot_vel_x[i] = x;
            _plot_vel_y[i] = y;
        }

    }
}

