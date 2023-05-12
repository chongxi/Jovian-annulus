/********************************************************************************
** Form generated from reading UI file 'console.ui'
**
** Created by: Qt User Interface Compiler version 5.9.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/
#include <iostream>

#include <QColorDialog>

#include "lighting_grid.h"

Lighting_Grid::Lighting_Grid( QWidget *parent, Scene_Model_Ptr model )
    : _scene_ready( false ), _lights_initialized( false ), _light_state( 8 ),
      light_enabled( 8 ), light_x_slider( 8 ), light_x_spin_box( 8 ), light_y_slider( 8 ),
      light_y_spin_box( 8 ), light_z_slider( 8 ), light_z_spin_box( 8 ), light_color( 8 ),
      diffuse_label( 8 ), diffuse_power_level( 8 ), _model( model )
{
   setupUi( parent );

   for ( int i = 0; i < 8; ++i )
   {
      connect( light_enabled[ i ], SIGNAL( toggled( bool ) ), this,
               SLOT( toggle_light( bool ) ) );
      connect( light_x_slider[ i ], SIGNAL( valueChanged( int ) ), this,
               SLOT( set_value( int ) ) );
      connect( light_x_spin_box[ i ], SIGNAL( valueChanged( double ) ), this,
               SLOT( set_value( double ) ) );
      connect( light_y_slider[ i ], SIGNAL( valueChanged( int ) ), this,
               SLOT( set_value( int ) ) );
      connect( light_y_spin_box[ i ], SIGNAL( valueChanged( double ) ), this,
               SLOT( set_value( double ) ) );
      connect( light_z_slider[ i ], SIGNAL( valueChanged( int ) ), this,
               SLOT( set_value( int ) ) );
      connect( light_z_spin_box[ i ], SIGNAL( valueChanged( double ) ), this,
               SLOT( set_value( double ) ) );

      connect( light_color[ i ], SIGNAL( clicked() ), this,
               SLOT( set_color_callback() ) );

      connect( diffuse_power_level[ i ], SIGNAL( valueChanged( int ) ), this,
               SLOT( set_power( int ) ) );
   }
}

void
Lighting_Grid::populate()
{
   _scene_ready = true;
   osg::BoundingBox *bound = _model->scene_bound();
   osg::Vec3f lower_bound = bound->corner( 4 );
   osg::Vec3f upper_bound = bound->corner( 7 );
   std::vector< osg::LightSource * > lights = _model->get_lights();

   for ( int i = 0; i < 8; ++i )
   {
      osg::LightSource *ls = lights[ i ];
      osg::Light *light = ls->getLight();
      osg::MatrixTransform *light_xform = _model->get_light_transforms()[ i ];
      osg::Vec4f v = light_xform->getMatrix().preMult( light->getPosition() );

      light_x_spin_box[ i ]->setMinimum( lower_bound.x() );
      light_x_spin_box[ i ]->setMaximum( upper_bound.x() );
      light_x_slider[ i ]->setMinimum( lower_bound.x() * 100 );
      light_x_slider[ i ]->setMaximum( upper_bound.x() * 100 );
      light_x_spin_box[ i ]->setValue( v.x() );

      light_y_spin_box[ i ]->setMinimum( lower_bound.y() );
      light_y_spin_box[ i ]->setMaximum( upper_bound.y() );
      light_y_slider[ i ]->setMinimum( lower_bound.y() * 100 );
      light_y_slider[ i ]->setMaximum( upper_bound.y() * 100 );
      light_y_spin_box[ i ]->setValue( v.y() );

      light_z_spin_box[ i ]->setMinimum( bound->corner( 0 ).z() );
      light_z_spin_box[ i ]->setMaximum( upper_bound.z() );
      light_z_slider[ i ]->setMinimum( bound->corner( 0 ).z() * 100 );
      light_z_slider[ i ]->setMaximum( upper_bound.z() * 100 );
      light_z_spin_box[ i ]->setValue( v.z() );

      osg::Vec4f const &diff = light->getDiffuse();
      QColor color( diff.r() * 255, diff.g() * 255, diff.b() * 255 );
      diffuse_label[ i ]->setText( color.name() );
      diffuse_label[ i ]->setPalette( QPalette( color ) );
      diffuse_label[ i ]->setAutoFillBackground( true );
      // diffuse_power_level[ i ];
   }
   if ( _lights_initialized )
   {
      for ( int i = 0; i < 8; ++i )
         from_light( _light_state[ i ], i );
      _lights_initialized = false;
   }
}

Light
Lighting_Grid::to_light( int index )
{
    Light light;
    light.enabled = light_enabled[index]->isChecked();
    light.x_pos = light_x_spin_box[index]->value();
    light.y_pos = light_y_spin_box[index]->value();
    light.z_pos = light_z_spin_box[index]->value();
    light.color = diffuse_label[index]->text().toStdString();
    light.power = diffuse_power_level[index]->value();

    return light;
}

void
Lighting_Grid::from_light( Light const& light, int index )
{
   _light_state[ index ] = light;
   _lights_initialized = true;
   light_enabled[ index ]->setChecked( light.enabled );
   if ( light.x_pos < light_x_spin_box[ index ]->minimum() )
   {
      light_x_spin_box[ index ]->setMinimum( light.x_pos );
      light_x_slider[ index ]->setMinimum( light.x_pos * 100 );
   }
   else if ( light.x_pos > light_x_spin_box[ index ]->maximum() )
   {
      light_x_spin_box[ index ]->setMaximum( light.x_pos );
      light_x_slider[ index ]->setMaximum( light.x_pos * 100 );
   }
   light_x_spin_box[ index ]->setValue( light.x_pos );

   if ( light.y_pos < light_y_spin_box[ index ]->minimum() )
   {
      light_y_spin_box[ index ]->setMinimum( light.y_pos );
      light_y_slider[ index ]->setMinimum( light.y_pos * 100 );
   }
   else if ( light.y_pos > light_y_spin_box[ index ]->maximum() )
   {
      light_y_spin_box[ index ]->setMaximum( light.y_pos );
      light_y_slider[ index ]->setMaximum( light.y_pos * 100 );
   }
   light_y_spin_box[ index ]->setValue( light.y_pos );

   if ( light.z_pos < light_z_spin_box[ index ]->minimum() )
   {
      light_z_spin_box[ index ]->setMinimum( light.z_pos );
      light_z_slider[ index ]->setMinimum( light.z_pos * 100 );
   }
   else if ( light.z_pos > light_z_spin_box[ index ]->maximum() )
   {
      light_z_spin_box[ index ]->setMaximum( light.z_pos );
      light_z_slider[ index ]->setMaximum( light.z_pos * 100 );
   }
   light_z_spin_box[ index ]->setValue( light.z_pos );

   QColor color( light.color.c_str() );
   diffuse_label[ index ]->setText( color.name() );
   diffuse_label[ index ]->setPalette( QPalette( color ) );
   diffuse_power_level[ index ]->setValue( light.power );
}

void
Lighting_Grid::update_lighting()
{
   for ( int i = 0; i < 8; ++i )
    update_lighting( i );
}

void
Lighting_Grid::toggle_light( bool onOrOff )
{
   QObject *widget = sender();
   size_t index = _widget_pair[ widget ].second;
   update_lighting( index );
}

void
Lighting_Grid::set_value( int value )
{
   QObject *widget = sender();
   dynamic_cast< QDoubleSpinBox * >( _widget_pair[ widget ].first )
       ->setValue( value / 100.f );
}

void
Lighting_Grid::set_value( double value )
{
   QObject *widget = sender();
   dynamic_cast< QSlider * >( _widget_pair[ widget ].first )
       ->setValue( (int)( value * 100.f ) );

   size_t index = _widget_pair[ widget ].second;

   if ( _scene_ready )
   {
      osg::Matrixd mat;

      mat.makeTranslate( light_x_spin_box[ index ]->value(),
                         light_y_spin_box[ index ]->value(),
                         light_z_spin_box[ index ]->value() );

      osg::MatrixTransform *light_xform = _model->get_light_transforms()[ index ];
      osg::MatrixTransform *marker_xform = _model->get_light_markers()[ index ];
      light_xform->setMatrix( mat );
      marker_xform->setMatrix( mat );
   }
}

void
Lighting_Grid::set_color_callback()
{
    QObject* widget = sender(); 

    QColor color = QColorDialog::getColor( QColor( dynamic_cast<QLabel*>(_widget_pair[widget].first)->text() ) );
    if ( color.isValid() )
        set_color( color );
}

void
Lighting_Grid::set_color( QColor color )
{
   QObject *widget = sender();
   QLabel *color_label = dynamic_cast< QLabel * >( _widget_pair[ widget ].first );

   color_label->setText( color.name() );
   color_label->setPalette( QPalette( color ) );
   color_label->setAutoFillBackground( true );

   if ( _scene_ready )
   {
      osg::Vec4 diff = osg::Vec4( color.red() / 255.0, color.green() / 255.0,
                                  color.blue() / 255.0, 1.0 );
      _model->set_diffuse_color( diff, _widget_pair[ widget ].second );
   }
}

void
Lighting_Grid::set_power( int power )
{
   QObject *widget = sender();
   QSpinBox *power_level = dynamic_cast< QSpinBox * >( _widget_pair[ widget ].first );

   if ( power_level->value() != power )
      power_level->setValue( power );

   if ( _scene_ready )
      _model->set_diffuse_power( power_level->value(), _widget_pair[ widget ].second );
}

void
Lighting_Grid::update_lighting( int index )
{
   osg::Matrixd mat;

   mat.makeTranslate( light_x_spin_box[ index ]->value(),
                      light_y_spin_box[ index ]->value(),
                      light_z_spin_box[ index ]->value() );

   osg::MatrixTransform *light_xform = _model->get_light_transforms()[ index ];
   osg::MatrixTransform *marker_xform = _model->get_light_markers()[ index ];
   light_xform->setMatrix( mat );
   marker_xform->setMatrix( mat );

   QColor color = QColor( diffuse_label[ index ]->text() );
   if ( color.isValid() )
   {
      osg::Vec4 diff = osg::Vec4( color.red() / 255.0, color.green() / 255.0,
                                  color.blue() / 255.0, 1.0 );
      _model->set_diffuse_color( diff, index );
   }

   if ( light_enabled[ index ]->isChecked() )
      _model->set_diffuse_power( diffuse_power_level[ index ]->value(), index );
   else
      _model->set_diffuse_power( 0, index );
}

void
Lighting_Grid::setupUi( QWidget *parent )
{
   std::vector< QFrame * > light(8);
   std::vector< QHBoxLayout * > light_layout(8);
   std::vector< QFrame * > light_enabled_frame(8);
   std::vector< QHBoxLayout * > light_enabled_frame_layout(8);
   std::vector< QFrame * > light_x_frame(8);
   std::vector< QHBoxLayout * > light_x_frame_layout(8);
   std::vector< QHBoxLayout * > light_x_layout(8);
   std::vector< QLabel * > light_x_label(8);
   std::vector< QFrame * > light_y_frame(8);
   std::vector< QHBoxLayout * > light_y_frame_layout(8);
   std::vector< QHBoxLayout * > light_y_layout(8);
   std::vector< QLabel * > light_y_label(8);
   std::vector< QFrame * > light_z_frame(8);
   std::vector< QHBoxLayout * > light_z_frame_layout(8);
   std::vector< QHBoxLayout * > light_z_layout(8);
   std::vector< QLabel * > light_z_label(8);
   std::vector< QFrame * > light_color_frame(8);
   std::vector< QHBoxLayout * > light_color_frame_layout(8);
   std::vector< QHBoxLayout * > light_color_layout(8);
   std::vector< QFrame * > light_power_frame(8);
   std::vector< QHBoxLayout * > light_power_frame_layout(8);
   std::vector< QHBoxLayout * > light_power_layout(8);
   std::vector< QLabel * > light_power_label(8);

   QLayout *verticalLayout_48 = parent->layout();
   for (int i = 0; i < 8; ++i)
   {
      QString index_string;
      index_string.setNum( i + 1 );

      light[i] = new QFrame( parent );
      light[i]->setObjectName( QStringLiteral( "light_" ) + index_string );
      light[i]->setFrameShape( QFrame::StyledPanel );
      light[i]->setFrameShadow( QFrame::Raised );
      light_layout[i] = new QHBoxLayout( light[i] );
      light_layout[i]->setSpacing( 0 );
      light_layout[i]->setObjectName( QStringLiteral( "light_layout_" ) + index_string );
      light_layout[i]->setContentsMargins( 0, 1, 0, 1 );
      light_enabled_frame[i] = new QFrame( light[i] );
      light_enabled_frame[i]->setObjectName( QStringLiteral( "light_enabled_frame_" ) +
                                          index_string );
      light_enabled_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_enabled_frame[i]->setFrameShadow( QFrame::Raised );
      light_enabled_frame_layout[i] = new QHBoxLayout( light_enabled_frame[i] );
      light_enabled_frame_layout[i]->setObjectName(
          QStringLiteral( "light_enabled_frame_layout_" ) + index_string );
      light_enabled_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_enabled[i] = new QCheckBox( light_enabled_frame[i] );
      light_enabled[ i ]->setChecked( true );
      light_enabled[i]->setObjectName( QStringLiteral( "light_enabled_" ) + index_string );
      _widget_pair[ light_enabled[i] ] = std::make_pair( light_enabled[i], i);

      light_enabled_frame_layout[i]->addWidget( light_enabled[i] );

      light_layout[i]->addWidget( light_enabled_frame[i] );

      light_x_frame[i] = new QFrame( light[i] );
      light_x_frame[i]->setObjectName( QStringLiteral( "light_x_frame_" ) + index_string );
      light_x_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_x_frame[i]->setFrameShadow( QFrame::Raised );
      light_x_frame_layout[i] = new QHBoxLayout( light_x_frame[i] );
      light_x_frame_layout[i]->setObjectName( QStringLiteral( "light_x_frame_layout_" ) +
                                          index_string );
      light_x_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_x_layout[i] = new QHBoxLayout();
      light_x_layout[i]->setObjectName( QStringLiteral( "light_x_layout_" ) + index_string );
      light_x_label[i] = new QLabel( light_x_frame[i] );
      light_x_label[i]->setObjectName( QStringLiteral( "light_x_label_" ) + index_string );

      light_x_layout[i]->addWidget( light_x_label[i] );

      light_x_slider[i] = new QSlider( light_x_frame[i] );
      light_x_slider[i]->setObjectName( QStringLiteral( "light_x_slider_" ) + index_string );
      light_x_slider[i]->setMaximum( 100 );
      light_x_slider[i]->setValue( 100 );
      light_x_slider[i]->setOrientation( Qt::Horizontal );

      light_x_layout[i]->addWidget( light_x_slider[i] );

      light_x_spin_box[i] = new QDoubleSpinBox( light_x_frame[i] );
      light_x_spin_box[i]->setObjectName( QStringLiteral( "light_x_spin_box_" ) + index_string );
      light_x_spin_box[i]->setMaximum( 1 );
      light_x_spin_box[i]->setSingleStep( 0.01 );
      light_x_spin_box[i]->setValue( 1 );

      _widget_pair[ light_x_slider[i] ] = std::make_pair( light_x_spin_box[i], i);
      _widget_pair[ light_x_spin_box[i] ] = std::make_pair( light_x_slider[i], i);

      light_x_layout[i]->addWidget( light_x_spin_box[i] );

      light_x_frame_layout[i]->addLayout( light_x_layout[i] );

      light_layout[i]->addWidget( light_x_frame[i] );

      light_y_frame[i] = new QFrame( light[i] );
      light_y_frame[i]->setObjectName( QStringLiteral( "light_y_frame_" ) + index_string );
      light_y_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_y_frame[i]->setFrameShadow( QFrame::Raised );
      light_y_frame_layout[i] = new QHBoxLayout( light_y_frame[i] );
      light_y_frame_layout[i]->setObjectName( QStringLiteral( "light_y_frame_layout_" ) +
                                          index_string );
      light_y_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_y_layout[i] = new QHBoxLayout();
      light_y_layout[i]->setObjectName( QStringLiteral( "light_y_layout_" ) + index_string );
      light_y_label[i] = new QLabel( light_y_frame[i] );
      light_y_label[i]->setObjectName( QStringLiteral( "light_y_label_" ) + index_string );

      light_y_layout[i]->addWidget( light_y_label[i] );

      light_y_slider[i] = new QSlider( light_y_frame[i] );
      light_y_slider[i]->setObjectName( QStringLiteral( "light_y_slider_" ) + index_string );
      light_y_slider[i]->setMaximum( 100 );
      light_y_slider[i]->setValue( 100 );
      light_y_slider[i]->setOrientation( Qt::Horizontal );

      light_y_layout[i]->addWidget( light_y_slider[i] );

      light_y_spin_box[i] = new QDoubleSpinBox( light_y_frame[i] );
      light_y_spin_box[i]->setObjectName( QStringLiteral( "light_y_spin_box_" ) + index_string );
      light_y_spin_box[i]->setMaximum( 1 );
      light_y_spin_box[i]->setSingleStep( 0.01 );
      light_y_spin_box[i]->setValue( 1 );

      _widget_pair[ light_y_slider[i] ] = std::make_pair( light_y_spin_box[i], i);
      _widget_pair[ light_y_spin_box[i] ] = std::make_pair( light_y_slider[i], i);

      light_y_layout[i]->addWidget( light_y_spin_box[i] );

      light_y_frame_layout[i]->addLayout( light_y_layout[i] );

      light_layout[i]->addWidget( light_y_frame[i] );

      light_z_frame[i] = new QFrame( light[i] );
      light_z_frame[i]->setObjectName( QStringLiteral( "light_z_frame_" ) + index_string );
      light_z_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_z_frame[i]->setFrameShadow( QFrame::Raised );
      light_z_frame_layout[i] = new QHBoxLayout( light_z_frame[i] );
      light_z_frame_layout[i]->setObjectName( QStringLiteral( "light_z_frame_layout_" ) +
                                          index_string );
      light_z_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_z_layout[i] = new QHBoxLayout();
      light_z_layout[i]->setObjectName( QStringLiteral( "light_z_layout_" ) + index_string );
      light_z_label[i] = new QLabel( light_z_frame[i] );
      light_z_label[i]->setObjectName( QStringLiteral( "light_z_label_" ) + index_string );

      light_z_layout[i]->addWidget( light_z_label[i] );

      light_z_slider[i] = new QSlider( light_z_frame[i] );
      light_z_slider[i]->setObjectName( QStringLiteral( "light_z_slider_" ) + index_string );
      light_z_slider[i]->setMaximum( 100 );
      light_z_slider[i]->setValue( 100 );
      light_z_slider[i]->setOrientation( Qt::Horizontal );

      light_z_layout[i]->addWidget( light_z_slider[i] );

      light_z_spin_box[i] = new QDoubleSpinBox( light_z_frame[i] );
      light_z_spin_box[i]->setObjectName( QStringLiteral( "light_z_spin_box_" ) + index_string );
      light_z_spin_box[i]->setMaximum( 1 );
      light_z_spin_box[i]->setSingleStep( 0.01 );
      light_z_spin_box[i]->setValue( 1 );

      _widget_pair[ light_z_slider[i] ] = std::make_pair( light_z_spin_box[i], i);
      _widget_pair[ light_z_spin_box[i] ] = std::make_pair( light_z_slider[i], i);

      light_z_layout[i]->addWidget( light_z_spin_box[i] );

      light_z_frame_layout[i]->addLayout( light_z_layout[i] );

      light_layout[i]->addWidget( light_z_frame[i] );

      light_color_frame[i] = new QFrame( light[i] );
      light_color_frame[i]->setObjectName( QStringLiteral( "light_color_frame_" ) +
                                        index_string );
      light_color_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_color_frame[i]->setFrameShadow( QFrame::Raised );
      light_color_frame_layout[i] = new QHBoxLayout( light_color_frame[i] );
      light_color_frame_layout[i]->setObjectName( QStringLiteral( "light_color_frame_layout_" ) +
                                              index_string );
      light_color_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_color_layout[i] = new QHBoxLayout();
      light_color_layout[i]->setObjectName( QStringLiteral( "light_color_layout_" ) +
                                         index_string );
      light_color[i] = new QPushButton( light_color_frame[i] );
      light_color[i]->setObjectName( QStringLiteral( "light_color_" ) + index_string );

      light_color_layout[i]->addWidget( light_color[i] );

      diffuse_label[i] = new QLabel( light_color_frame[i] );
      diffuse_label[i]->setObjectName( QStringLiteral( "diffuse_label_" ) + index_string );

      _widget_pair[ light_color[i] ] = std::make_pair( diffuse_label[i], i);
      _widget_pair[ diffuse_label[i] ] = std::make_pair( light_color[i], i);

      QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
      sizePolicy.setHorizontalStretch( 0 );
      sizePolicy.setVerticalStretch( 0 );
      sizePolicy.setHeightForWidth( diffuse_label[i]->sizePolicy().hasHeightForWidth() );
      diffuse_label[i]->setSizePolicy( sizePolicy );
      QPalette palette;
      QBrush brush( QColor( 255, 255, 255, 255 ) );
      brush.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::WindowText, brush );
      QBrush brush1( QColor( 25, 25, 25, 255 ) );
      brush1.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Button, brush1 );
      QBrush brush2( QColor( 37, 37, 37, 255 ) );
      brush2.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Light, brush2 );
      QBrush brush3( QColor( 31, 31, 31, 255 ) );
      brush3.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Midlight, brush3 );
      QBrush brush4( QColor( 12, 12, 12, 255 ) );
      brush4.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Dark, brush4 );
      QBrush brush5( QColor( 16, 16, 16, 255 ) );
      brush5.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Mid, brush5 );
      palette.setBrush( QPalette::Active, QPalette::Text, brush );
      palette.setBrush( QPalette::Active, QPalette::BrightText, brush );
      palette.setBrush( QPalette::Active, QPalette::ButtonText, brush );
      QBrush brush6( QColor( 0, 0, 0, 255 ) );
      brush6.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::Base, brush6 );
      palette.setBrush( QPalette::Active, QPalette::Window, brush1 );
      palette.setBrush( QPalette::Active, QPalette::Shadow, brush6 );
      palette.setBrush( QPalette::Active, QPalette::AlternateBase, brush4 );
      QBrush brush7( QColor( 255, 255, 220, 255 ) );
      brush7.setStyle( Qt::SolidPattern );
      palette.setBrush( QPalette::Active, QPalette::ToolTipBase, brush7 );
      palette.setBrush( QPalette::Active, QPalette::ToolTipText, brush6 );
      palette.setBrush( QPalette::Inactive, QPalette::WindowText, brush );
      palette.setBrush( QPalette::Inactive, QPalette::Button, brush1 );
      palette.setBrush( QPalette::Inactive, QPalette::Light, brush2 );
      palette.setBrush( QPalette::Inactive, QPalette::Midlight, brush3 );
      palette.setBrush( QPalette::Inactive, QPalette::Dark, brush4 );
      palette.setBrush( QPalette::Inactive, QPalette::Mid, brush5 );
      palette.setBrush( QPalette::Inactive, QPalette::Text, brush );
      palette.setBrush( QPalette::Inactive, QPalette::BrightText, brush );
      palette.setBrush( QPalette::Inactive, QPalette::ButtonText, brush );
      palette.setBrush( QPalette::Inactive, QPalette::Base, brush6 );
      palette.setBrush( QPalette::Inactive, QPalette::Window, brush1 );
      palette.setBrush( QPalette::Inactive, QPalette::Shadow, brush6 );
      palette.setBrush( QPalette::Inactive, QPalette::AlternateBase, brush4 );
      palette.setBrush( QPalette::Inactive, QPalette::ToolTipBase, brush7 );
      palette.setBrush( QPalette::Inactive, QPalette::ToolTipText, brush6 );
      palette.setBrush( QPalette::Disabled, QPalette::WindowText, brush4 );
      palette.setBrush( QPalette::Disabled, QPalette::Button, brush1 );
      palette.setBrush( QPalette::Disabled, QPalette::Light, brush2 );
      palette.setBrush( QPalette::Disabled, QPalette::Midlight, brush3 );
      palette.setBrush( QPalette::Disabled, QPalette::Dark, brush4 );
      palette.setBrush( QPalette::Disabled, QPalette::Mid, brush5 );
      palette.setBrush( QPalette::Disabled, QPalette::Text, brush4 );
      palette.setBrush( QPalette::Disabled, QPalette::BrightText, brush );
      palette.setBrush( QPalette::Disabled, QPalette::ButtonText, brush4 );
      palette.setBrush( QPalette::Disabled, QPalette::Base, brush1 );
      palette.setBrush( QPalette::Disabled, QPalette::Window, brush1 );
      palette.setBrush( QPalette::Disabled, QPalette::Shadow, brush6 );
      palette.setBrush( QPalette::Disabled, QPalette::AlternateBase, brush1 );
      palette.setBrush( QPalette::Disabled, QPalette::ToolTipBase, brush7 );
      palette.setBrush( QPalette::Disabled, QPalette::ToolTipText, brush6 );
      diffuse_label[i]->setPalette( palette );
      diffuse_label[i]->setFrameShape( QFrame::StyledPanel );

      light_color_layout[i]->addWidget( diffuse_label[i] );

      light_color_frame_layout[i]->addLayout( light_color_layout[i] );

      light_layout[i]->addWidget( light_color_frame[i] );

      light_power_frame[i] = new QFrame( light[i] );
      light_power_frame[i]->setObjectName( QStringLiteral( "light_power_frame_" ) +
                                        index_string );
      light_power_frame[i]->setFrameShape( QFrame::StyledPanel );
      light_power_frame[i]->setFrameShadow( QFrame::Raised );
      light_power_frame_layout[i] = new QHBoxLayout( light_power_frame[i] );
      light_power_frame_layout[i]->setObjectName( QStringLiteral( "light_power_frame_layout_" ) +
                                              index_string );
      light_power_frame_layout[i]->setContentsMargins( 3, 0, 3, 0 );
      light_power_layout[i] = new QHBoxLayout();
      light_power_layout[i]->setObjectName( QStringLiteral( "light_power_layout_" ) +
                                         index_string );
      light_power_label[i] = new QLabel( light_power_frame[i] );
      light_power_label[i]->setObjectName( QStringLiteral( "light_power_label_" ) +
                                        index_string );

      light_power_layout[i]->addWidget( light_power_label[i] );

      diffuse_power_level[i] = new QSpinBox( light_power_frame[i] );
      diffuse_power_level[i]->setObjectName( QStringLiteral( "diffuse_power_level_" ) +
                                          index_string );
      diffuse_power_level[i]->setMinimum( 0 );
      diffuse_power_level[i]->setMaximum( 100 );
      diffuse_power_level[i]->setValue( 10 );

      // Mostly to get the index
      _widget_pair[ diffuse_power_level[i] ] = std::make_pair( diffuse_power_level[i], i);

      light_power_layout[i]->addWidget( diffuse_power_level[i] );

      light_power_frame_layout[i]->addLayout( light_power_layout[i] );

      light_layout[i]->addWidget( light_power_frame[i] );

      verticalLayout_48->addWidget( light[i] );

      light_enabled[ i ]->setText( QApplication::translate(
          "MainWindow", index_string.toStdString().c_str(), Q_NULLPTR ) );
      light_x_label[ i ]->setText(
          QApplication::translate( "MainWindow", "X", Q_NULLPTR ) );
      light_y_label[ i ]->setText(
          QApplication::translate( "MainWindow", "Y", Q_NULLPTR ) );
      light_z_label[ i ]->setText(
          QApplication::translate( "MainWindow", "Z", Q_NULLPTR ) );
      light_color[ i ]->setText(
          QApplication::translate( "MainWindow", "Light Color", Q_NULLPTR ) );
      //diffuse_label[ i ]->setText(
        //  QApplication::translate( "MainWindow", "#191919", Q_NULLPTR ) );
      light_power_label[ i ]->setText(
          QApplication::translate( "MainWindow", "Intensity", Q_NULLPTR ) );
   }

} // setupUi

