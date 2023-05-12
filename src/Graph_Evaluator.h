// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
#ifndef GRAPH_EVALUATOR
#define GRAPH_EVALUATOR

#include <vector>

/**
 * @brief Graph_Evaluator.
 * @details Given a set of
 */

class Graph_Evaluator
{
	friend std::ostream& operator<<( std::ostream& os, Graph_Evaluator const& f );

public:

	/// @name Initialization
	///@{
	Graph_Evaluator() {}
	Graph_Evaluator( std::vector< double > x_vals, std::vector< double > y_vals ):
		_plot_x( x_vals ), _plot_y( y_vals ) {}
	///@}

	/// @name Duplication
	///@{
	///@}

	/// @name Destruction
	///@{
	~Graph_Evaluator() {}
	///@}

	/// @name Access
	///@{
	std::vector< double > const& x_vals() const { return _plot_x; }
	std::vector< double > const& y_vals() const { return _plot_y; }

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
	double evaluate( double x )
	{
		double value = 0.;
		std::vector< double >::iterator lb, ub;
		ub = std::upper_bound( _plot_x.begin(), _plot_x.end(), std::abs( x ) );

		if ( ub == _plot_x.end() )
		{
			value = _plot_y.back();
		}
		else if ( ub == _plot_x.begin() )
		{
			value = _plot_y.front();
		}
		else
		{
			// x is in range [_plot_x.begin(), _plot_x.end()]
			lb = ub - 1;
			double weight = ( std::abs( x ) - *lb ) / ( *ub - *lb );
			value = weight * _plot_y[ lb - _plot_x.begin() ] + ( 1.f - weight ) * _plot_y[ ub - _plot_x.begin() ];
		}

		return value;
	}

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

protected:
	///@{
	///@}

private:
	std::vector< double > _plot_x, _plot_y;

};	// end of class Graph_Evaluator

#endif // GRAPH_EVALUATOR

