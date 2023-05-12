// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015

#ifndef RADIO_BUTTON_GROUP_H
#define RADIO_BUTTON_GROUP_H

#include <utility>
#include <vector>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QRadioButton>
#include <QSpacerItem>
#include <QVBoxLayout>

/**
 * @brief Creates a Radio Button Group that emits a signal when an item is selected
 * @details A convenience method for creating a single flexible widget that presents
 * a radio button with a user-specified name. Useful for selecting from multiple
 * loaded files.
 *
 */
class Radio_Button_Group: public QObject
{
	Q_OBJECT

public:

	Radio_Button_Group( QWidget* parent, QVBoxLayout* layout, QSpacerItem* spacer );
	~Radio_Button_Group();

	void load( QString name );
	void clear();
	void select( int index );
	std::vector<std::string> const& names() const { return _names; }

public Q_SLOTS:
	void selected( int index );

Q_SIGNALS:
	void activated( int index );

private:

	QWidget* _parent;
	QVBoxLayout* _group_box_layout;
	QSpacerItem* _spacer;
	QButtonGroup* _grouper;
	std::vector< QFrame* > _widgets;
	std::vector<std::string> _names;

};

#endif // RADIO_BUTTON_GROUP_H

