#ifndef PYTHON_SOCKET_THREAD_H
#define PYTHON_SOCKET_THREAD_H

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include "thread_adaptor.h"
#include "tcp_server.h"

/** @brief Python_Socket_Thread.
 * @details
 */

class Python_Socket_Thread
{
   friend std::ostream &operator<<( std::ostream &os, Python_Socket_Thread const &f );

 public:
   /// @name Initialization
   ///@
   Python_Socket_Thread( int port, Session *session ) : _comm( new TCP_Server( port ) ), _terminated( false ) 
   {
   	_comm->set_session( session );
   }
   ///@}

   /// @name Duplication
   ///@{
   ///@}

   /// @name Move
   ///@{
   ///@}

   /// @name Destruction
   ///@{
   ~Python_Socket_Thread() { delete _comm; }
   ///@}

   /// @name Access
   ///@{
    boost::condition& condition() { return _condition; }
    boost::mutex& mutex() { return _mutex; }
    TCP_Server const* server() const { return _comm; }
   ///@}
   /// @name Measurement
   ///@{
   ///@}
   /// @name Comparison
   ///@{
   ///@}
   /// @name Status report
   ///@{
   void
   terminate()
   {
      _terminated = true;
      _comm->stop();
   }
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
   void run();
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

   static void
   do_thread( void *param )
   {
      static_cast< Python_Socket_Thread * >( param )->run();
   }

 protected:
   ///@{
   ///@}

 private:
   bool _terminated;
   boost::try_mutex _mutex;
   boost::condition _condition;
   TCP_Server *_comm;

}; // end of class Python_Socket_Thread

#endif // PYTHON_SOCKET_THREAD_H
