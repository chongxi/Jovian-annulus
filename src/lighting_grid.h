/********************************************************************************
** Form generated from reading UI file 'console.ui'
**
** Created by: Qt User Interface Compiler version 5.9.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LIGHTING_GRID_H
#define LIGHTING_GRID_H

#include <map>
#include <vector>

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "Globals.h"
#include "scene_model.h"

typedef std::pair< QObject*, size_t> QObject_Mapping;


class Lighting_Grid : public QObject
{
   Q_OBJECT

 public:
   /// @name Initialization
   ///@{
   Lighting_Grid( QWidget *parent, Scene_Model_Ptr model );
   ///@}

   /// @name Access
   ///@{
   ///@}
   /// @name Measurement
   ///@{
   ///@}
   /// @name Comparison
   ///@{
   ///@}
   /// @name Status report
   ///@{
   ///@}
   /// @name Status setting
   ///@{
   ///@}
   /// @name Cursor movement
   ///@{
   ///@}
   /// @name Element change
   ///@{
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
   void populate();
   Light to_light( int index );
   void from_light( Light const& light, int index );
   void update_lighting();
   ///@}
   /// @name Miscellaneous
   ///@{
   ///@}
   /// @name Obsolete
   ///@{
   ///@}
   /// @name Inapplicable
   ///@{
   ///@}

 public Q_SLOTS:
   void toggle_light( bool onOrOff );
   void set_value( int value );
   void set_value( double value );
   void set_color_callback();
   void set_color( QColor color );
   void set_power( int power );

 private:
   void update_lighting( int index );
   void setupUi( QWidget *parent );

   bool _scene_ready, _lights_initialized;
   std::vector<Light> _light_state;
   std::vector< QCheckBox * > light_enabled;
   std::vector< QSlider * > light_x_slider;
   std::vector< QDoubleSpinBox * > light_x_spin_box;
   std::vector< QSlider * > light_y_slider;
   std::vector< QDoubleSpinBox * > light_y_spin_box;
   std::vector< QSlider * > light_z_slider;
   std::vector< QDoubleSpinBox * > light_z_spin_box;
   std::vector< QPushButton * > light_color;
   std::vector< QLabel * > diffuse_label;
   std::vector< QSpinBox * > diffuse_power_level;

   std::map< QObject *, QObject_Mapping > _widget_pair;
   Scene_Model_Ptr _model;
};

#endif // LIGHTING_GRID_H
