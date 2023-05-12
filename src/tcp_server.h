#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <boost/array.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/asio.hpp>
#pragma clang diagnostic pop
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

#include "session.h"

using boost::asio::ip::tcp;

// The glue between asio's sockets and the third party library.
class connection : public boost::enable_shared_from_this< connection >
{
 public:
   typedef boost::shared_ptr< connection > pointer;

   static pointer
   create( boost::asio::io_service &io_service, Session *session )
   {
      // std::cout << "creating connection" << std::endl;
      return pointer( new connection( io_service, session ) );
   }

   ~connection() { socket_.close(); }
   
   tcp::socket &
   socket()
   {
      return socket_;
   }

   void
   start()
   {
      // std::cout << "starting connection" << std::endl;
      // Put the socket into non-blocking mode.

      _session_impl->set_socket( &socket_ );
      socket_.non_blocking( true );
      socket_.set_option(boost::asio::ip::tcp::no_delay(true));

      start_operations();

      // std::cout << "ending connection" << std::endl;
   }

 private:
   connection( boost::asio::io_service &io_service, Session *session )
       : socket_( io_service ), _session_impl( session ), read_in_progress_( false ),
         write_in_progress_( false )
   {
   }

   void
   start_operations()
   {
      // std::cout << "start_operations: " << std::endl;
      // std::cout << "  want_read: " << _session_impl->want_read() << " " << read_in_progress_ << std::endl;
      // std::cout << "  want_write: " << _session_impl->want_write() << " " << write_in_progress_ << std::endl;
      // Start a read operation if the third party library wants one.
      if ( _session_impl->want_read() && !read_in_progress_ )
      {
         // std::cout << "reading" << std::endl;
         read_in_progress_ = true;
         socket_.async_read_some( boost::asio::null_buffers(),
                                  boost::bind( &connection::handle_read,
                                               shared_from_this(),
                                               boost::asio::placeholders::error ) );
      }

      // Start a write operation if the third party library wants one.
      if ( _session_impl->want_write() && !write_in_progress_ )
      {
         // std::cout << "writing" << std::endl;
         write_in_progress_ = true;
         socket_.async_write_some( boost::asio::null_buffers(),
                                   boost::bind( &connection::handle_write,
                                                shared_from_this(),
                                                boost::asio::placeholders::error ) );
      }
   }

   void
   handle_read( boost::system::error_code ec )
   {
      // std::cout << "handle_read: " << std::endl;
      read_in_progress_ = false;

      // Notify third party library that it can perform a read.
      if ( !ec )
         _session_impl->do_read( ec );

      // The third party library successfully performed a read on the socket.
      // Start new read or write operations based on what it now wants.
      if ( !ec || ec == boost::asio::error::would_block )
         start_operations();

      // Otherwise, an error occurred. Closing the socket cancels any outstanding
      // asynchronous read or write operations. The connection object will be
      // destroyed automatically once those outstanding operations complete.
      else
         socket_.close();
   }

   void
   handle_write( boost::system::error_code ec )
   {
      // std::cout << "handle_write: " << std::endl;
      write_in_progress_ = false;

      // Notify third party library that it can perform a write.
      if ( !ec )
         _session_impl->do_write( ec );

      // The third party library successfully performed a write on the socket.
      // Start new read or write operations based on what it now wants.
      if ( !ec || ec == boost::asio::error::would_block )
         start_operations();

      // Otherwise, an error occurred. Closing the socket cancels any outstanding
      // asynchronous read or write operations. The connection object will be
      // destroyed automatically once those outstanding operations complete.
      else
         socket_.close();
   }

 private:
   tcp::socket socket_;
   Session *_session_impl;
   bool read_in_progress_;
   bool write_in_progress_;
};

class Server
{
 public:
   Server( boost::asio::io_service &io_service, unsigned short port )
       : _has_client(false), _port( port ), 
       acceptor_( io_service, tcp::endpoint( tcp::v4(), port ) ), _session( 0 )
   {
   }

   ~Server() { delete _session; }

   void
   set_session( Session *session )
   {
      _session = session;
   }

   bool has_client() const { return _has_client; }

   void
   start_accept()
   {
      if ( _session )
      {
         _connection =
             connection::create( acceptor_.get_io_service(), _session );

         acceptor_.async_accept( _connection->socket(),
                                 boost::bind( &Server::handle_accept, this,
                                              _connection,
                                              boost::asio::placeholders::error ) );
      }
      else
         std::cout << "No session object has been set. Ignoring connection request."
                   << std::endl;
   }

   // Force the server out of the accept loop
   void stop()
   {
      acceptor_.close();
      _session->wake();
   }

 private:
   void
   handle_accept( connection::pointer new_connection,
                  const boost::system::error_code &error )
   {
      // std::cout << "handle_accept" << std::endl;

      if ( !error )
      {
         _has_client = true;
         new_connection->start();
         start_accept();
      }

   }

   bool _has_client;
   unsigned short _port;
   tcp::acceptor acceptor_;
   Session *_session;
   connection::pointer _connection; // Keep a pointer to the connection awaiting an accept
};

/** @brief TCP_Server.
 * @details
 */

class TCP_Server
{
   friend std::ostream &operator<<( std::ostream &os, TCP_Server const &f );

 public:
   /// @name Initialization
   ///@{
   TCP_Server( unsigned short port ) : _io_service(), _server( _io_service, port ) {}
   ///@}

   /// @name Duplication
   ///@{
   ///@}

   /// @name Move
   ///@{
   ///@}

   /// @name Destruction
   ///@{
   ~TCP_Server()
   {
      if ( !stopped() )
         stop();
   }
   ///@}

   /// @name Access
   ///@{
   Server const& server() const { return _server; }
   ///@}
   /// @name Measurement
   ///@{
   ///@}
   /// @name Comparison
   ///@{
   ///@}
   /// @name Status report
   ///@{
   bool
   stopped()
   {
      return _io_service.stopped();
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
   void
   set_session( Session *session )
   {
      _server.set_session( session );
   }
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
   void
   start()
   {
      _server.start_accept();
      _io_service.run();
   }

   void
   stop()
   {
      _io_service.stop();
      _server.stop();
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
   boost::asio::io_service _io_service;
   Server _server;
   ///@}

 private:
   /* data */

}; // end of class TCP_Server

#endif // TCP_SERVER_H
