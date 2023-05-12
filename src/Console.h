// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/* -*-c++-*- */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#ifndef Q_MOC_RUN
#include <boost/thread/thread.hpp>
#endif

#include <QMainWindow>
#include <QSerialPortInfo>

#include <btBulletDynamicsCommon.h>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/ArgumentParser>

#include "Communicator.h"
#include "CameraUpdateCallback.h"
#include "Config_Memento.h"
#include "Graph_Evaluator.h"
#include "graphwidget.h"
#include "lighting_grid.h"
#include "RadioButtonGroup.h"
#include "QCustomPlot/qcustomplot.h"
#include "QPyConsole/qpyconsole.h"
#include "python_socket_thread.h"
#include "scene_model.h"
#include "SerialPortDialog/settingsdialog.h"
#include "tcp_server.h"
#include "viewingWindowQt.h"

class Virtual_Machine;
void wrap_console();

namespace Ui
{
    class MainWindow;
}

/** @brief Console.
 * @details
 */

class Console: public QMainWindow
{
    Q_OBJECT

  public:

    /// @name Initialization
    ///@{
    Console( osg::ArgumentParser& args, QWidget* parent = 0 );
    ///@}

    /// @name Duplication
    ///@{
    ///@}

    /// @name Destruction
    ///@{
    ~Console()
    {
        if ( _thread != nullptr )
        {
            _thread->join();
            delete _thread;
            _thread = nullptr;
        }

        if ( _command_port )
        {
            delete _command_port;
            _command_port = nullptr;
        }
    }
    ///@}

    /// @name Access
    ///@{
    Scene_Model_Ptr model() { return _model; }
    int retries( void ) const;
    std::string port( void ) const;
    std::string hostname( void ) const;
    Communicator* get_communicator( void ) { return _comm; }
    Virtual_Machine* vm() { return _vm; }

    ///@}
    /// @name Measurement
    ///@{
    ///@}
    /// @name Comparison
    ///@{
    ///@}
    /// @name Status report
    ///@{
    bool motion_and_display_ganged();
    bool open_field_turning_enabled();
    ///@}
    /// @name Status setting
    ///@{
    void update_shader_widget();
    ///@}
    /// @name Cursor movement
    ///@{
    ///@}
    /// @name Element change
    ///@{
    void set_viewer( Viewing_Window_Qt* v ) { _viewer = v; };
    ///@}
    /// @name Removal
    ///@{
    ///@}
    /// @name Resizing
    ///@{
    ///@}
    /// @name Transformation
    ///@{
    ///@}
    /// @name Conversion
    ///@{
    ///@}
    /// @name Basic operations
    ///@{
    void blank_display();
    void disable_motion();
    void do_open( QString fileName );
    void load_config( Config_Memento const* cm );
    void manual_speed_down();
    void manual_speed_up();
    void reset_position();
    void select_scene();
    void toggle_python_source( bool flag );
    void process_socket_command( std::string& string_data );
    ///@}
    /// @name Thread Safe Operations
    ///@{
    /// These routines are thread-safe for calling Qt methods from the python
    /// interpreter in Virtual_Machine
    void signal_blank_display() { QMetaObject::invokeMethod( this, "set_display_blanking_callback" ); }
    void signal_connect() { QMetaObject::invokeMethod( this, "setup_connection" ); }
    void signal_disable_motion() { QMetaObject::invokeMethod( this, "disable_motion_callback" ); }
    void signal_blanking_and_disable_motion() { QMetaObject::invokeMethod( this, "motion_and_blanking_callback" ); }
    void signal_jump_to_start( int id );
    void signal_switch_scene( int id )
    {
        if ( id > 0 && id <= _file_widget->names().size() )
        {
            _file_widget->select( id - 1 );
            QMetaObject::invokeMethod( _file_widget, "selected", Qt::DirectConnection, Q_ARG( int, id ) );
        }
    }
    ///@}
    /// @name Miscellaneous
    ///@{
    bool move_to_object( std::string name );
    bool move_to_position( float x, float y, float z, float direction );
    void output_string( std::string output_text );
    bool switch_scene( std::string name );
    ///@}
    /// @name Obsolete
    ///@{
    ///@}
    /// @name Inapplicable
    ///@{
    ///@}

  public Q_SLOTS:
    void start_viewer();

  protected:
    ///@{
    void keyPressEvent( QKeyEvent* event );
    void move_to_start();
    void populate_start_locations();
    void save_config( Config_Memento* cm );
    ///@}

  private Q_SLOTS:

    void about();
    void do_open();
    void do_close();
    void do_open_config();
    void do_save_config();
    void do_open_replay();
    void do_image_open();
    void do_OSG_open();
    void reset_sliders( int value );
    void set_camera_maximums( int newMax );
    void update_viewport_left( int value );
    void update_viewport_right( int value );
    void set_x_focal_length( int value );
    void set_y_focal_length( int value );
    void set_gang_motion( bool value );
    void set_distortion_enabled( bool value );
    void set_center();
    void setup_connection( bool reset = true );
    void motion_and_blanking_callback();
    void disable_motion_callback();
    void set_start_location( int value );
    void jump_to_location();

    ///@{ @name Display Callbacks
    void reset_display();
    void distribute_horizontally();
    void distribute_vertically();
    void smooth_display();
    void linearize_edges();
    void use_annulus( bool on_or_off ) { if ( _viewer ) _viewer->use_annulus( on_or_off ); }
    void set_inner_radius( int radius );
    void set_outer_radius( int radius );
    void set_minor_axis( int radius );
    void set_major_axis( int radius );
    void set_inner_radius( double radius );
    void set_outer_radius( double radius );
    void set_minor_axis( double radius );
    void set_major_axis( double radius );
    void set_center_x( double x );
    void set_center_x( int x );
    void set_center_y( double y );
    void set_center_y( int y );
    void set_rotate_x( double x );
    void set_rotate_x( int x );
    void set_rotate_y( double y );
    void set_rotate_y( int y );
    void set_rotate_z( double z );
    void set_rotate_z( int z );
    void set_frame_packing( bool value );
    ///@}

    ///@{ @name Lighting Callbacks
    void set_ambient_color_callback();
    void set_global_brightness_callback();
    void set_display_blanking_callback();
    void set_global_brightness_target( int value );
    void set_background_color_callback();
    void set_diffuse_color_callback();
    void set_diffuse_power( int power );
    void update_lights( bool value ) { update_lights( 1.f );  }
    void update_lights( int value ) { update_lights( ( float ) value );  }
    void update_lights( double value );
    void set_attenuation( int value );
    ///@}

    ///@{ @name Indicator Callbacks
    void set_indicator_size( int new_size );
    void set_indicator_border_size( int new_size );
    void set_indicator_position( int ignored );
    void set_indicator_screen( int screen );
    void set_indicator_mode( int index );
    ///@}

    ///@{ @name Field of View Callbacks
    void set_field_of_view( int fov );
    void set_vert_field_of_view( int fov );
    void set_vert_field_of_view( double fov );
    void set_rotated_cameras( bool yes_or_no );
    ///@}

    ///@{ @name Calibration Slots
    void calibrate_x();
    void calibrate_y();
    void calibrate_z();
    void enable_editing( bool value );
    void update_calibration_values_callback();
    ///@}

    ///@{ @name Configuration Widgets
    void enable_velocity_smoothing( bool yes_or_no );
    void set_interval_for_velocity_smoothing( int value );
    void enable_segment_smoothing( bool yes_or_no );
    void enable_pre_blend_heading_smoothing( bool yes_or_no );
    void enable_intermediary_heading_smoothing( bool yes_or_no );
    void enable_post_blend_heading_smoothing( bool yes_or_no );
    void set_interval_for_segment_smoothing( int value );
    void set_interval_for_pre_blending_smoothing( int value );
    void set_interval_for_intermediary_smoothing( int value );
    void set_interval_for_post_blending_smoothing( int value );
    void update_world_units( int index );
    void set_output_serial_port_name( int index );
    void set_data_server_port_name( int index );
    void activate_command_serial_port_name( int index );
    void connect_command_serial_port();
    void enable_file_export_callback( bool yes_or_no );
    void do_set_export_file_name();
    void set_output_format( int value );
    void enable_treadmill_output_callback( bool yes_or_no ) { _viewer->set_export_treadmill( yes_or_no ); };
    void enable_reduced_output_callback( bool yes_or_no ) { _viewer->set_reduced_output( yes_or_no ); };
    void auto_heading_callback();
    void set_crossbar_width( double value );
    void restrict_vertical_motion( bool yes_or_no );
    void set_minimum_velocity_thresold( double value );
    void set_output_rate( int value );
    void toggle_physics_debugging( bool yes_or_no ) { _model->debug_physics( yes_or_no ); };
    void show_invisible_objects( bool yes_or_no );
    void change_shader( int value );
    void update_offset( double value );
    void save_scene_graph();

    // Sockets
    void switch_python_source(bool);
    void start_python_socket();
    void start_output_socket();
    void start_command_socket();
    void python_results( const std::string& result );

    ///@}

    ///@{ @name Python Slots
    void process_serial_command();
    ///@}

    ///@{ @name Turning Widgets
    void enable_threshold_turning( bool value );
    void update_plot( int row, int column );
    void update_velocity_plot( int row, int column );
    void add_row();
    void delete_row();
    void add_row_velocity();
    void delete_row_velocity();
    void update_threshold_weight( int val );
    void update_threshold_scale( double val );
    void update_auto_heading_turn_rate( double val );
    ///@}

    ///@{ @name File Widget Slot
    void switch_scene( int id );
    void update_timer();
    void update_ratio_line( );
    void process_command();

    ///@}

  private:
    friend class Config_Memento;

    void initialize_plot_widgets( std::vector< double > x, std::vector< double > y,
                                  std::vector< double > vx, std::vector< double > vy,
                                  bool create_plot = true );
    void update_plot( );
    void update_velocity_plot();
    void enable_blanking_widgets( bool onOrOff );
    void update_viewport( int value, int lower, int upper );
    void do_calibration( QPushButton* button, float* rx, float* ry, float* rz );
    void set_ambient_color( QColor color );
    void set_global_brightness( int i, QColor color );
    void set_background_color( QColor color );
    void set_diffuse_color( QColor color );
    void set_export_filename( QString const& fileName );
    float compute_turning_mixture( float val );
    void set_command_serial_port_name( int index );

    Ui::MainWindow* _ui;
    Lighting_Grid* _lighting_grid;
    osg::ArgumentParser& _arguments;
    Scene_Model_Ptr _model;
    Virtual_Machine* _vm;
    boost::thread *_thread, *_output_thread, *_command_thread, *_python_thread;
    Viewing_Window_Qt* _viewer;
    Communicator* _comm;
    Python_Socket_Thread *_output_pst, *_command_pst, *_python_pst;
    Camera_Update_Callback* _callback;
    bool _firstTime, _pythonSourceIsSerial;
    bool _connected, _python_connected, _output_connected, _command_connected;
    bool _collada_loaded;
    int _connection_counter;
    bool _use_image;
    int _last_camera;
    bool _calibrating;
    int _activeScene;
    std::string _image_file;
    std::string _osg_file;
    std::vector< std::pair< int, int > > _frame;
    std::vector< triple< bool, int, int > > _distort;
    std::vector< std::pair< int, int > > _center;
    float _Vfwdf;
    float _Vssf;
    float _Omegaf;
    int _calibration_runs[3];
    osg::Vec4 _clear_color;
    std::vector< QColor > _brightness;
    std::vector< Graph_Widget* > _graphics_view;
    bool _blank_display;
    bool _use_auto_heading;
    bool _motion_disabled;
    Radio_Button_Group* _file_widget;
    Radio_Button_Group* _start_location_widgets;
    int _start_index;
    osg::Vec3d _manual_speed;
    QCustomPlot* _plot_ratio_widget;
    QCustomPlot* _plot_velocity_widget;
    double _plot_ratio_max_y, _plot_velocity_max_y;
    QCPItemLine* _ratio_line;
    QCPItemLine* _smoothed_ratio_line;
    QCPItemLine* _velocity_ratio_line;
    QCPItemLine* _smoothed_velocity_ratio_line;
    Graph_Evaluator _plot, _v_plot;
    QTimer* _clock_timer;
    QTimer* _plot_timer;
    QTime _time;
    QPyConsole* _python_widget;
    QList<QSerialPortInfo> _available_ports;
    SettingsDialog* _settings;
    SettingsDialog::Settings _command_port_settings;
    SettingsDialog::Settings _output_port_settings;
    SettingsDialog::Settings _server_port_settings;
    QSerialPort* _command_port;
    QString _command_data;
};

#endif
