#ifndef BOOST_THREAD_CONCURRENT_QUEUES_DETAIL_SYNC_QUEUE_BASE_HPP
#define BOOST_THREAD_CONCURRENT_QUEUES_DETAIL_SYNC_QUEUE_BASE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2013-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>

#include <boost/throw_exception.hpp>


#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{

  template <class ValueType, class Queue>
  class sync_queue_base
  {
  public:
    typedef ValueType value_type;
    typedef Queue underlying_queue_type;
    typedef std::size_t size_type;
    typedef queue_op_status op_status;

    // Constructors/Assignment/Destructors
    BOOST_THREAD_NO_COPYABLE(sync_queue_base)
    inline sync_queue_base();
    //template <typename Range>
    //inline explicit sync_queue(Range range);
    inline ~sync_queue_base();

    // Observers
    inline bool empty() const;
    inline bool full() const;
    inline size_type size() const;
    inline bool closed() const;

    // Modifiers
    inline void close();

    inline underlying_queue_type underlying_queue() {
      lock_guard<mutex> lk(mtx_);
      waiting_empty_ = 0;
      return boost::move(data_);
    }

  protected:
    mutable mutex mtx_;
    condition_variable not_empty_;
    size_type waiting_empty_;
    underlying_queue_type data_;
    bool closed_;

    inline bool empty(unique_lock<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.empty();
    }
    inline bool empty(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.empty();
    }

    inline size_type size(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.size();
    }
    inline bool closed(unique_lock<mutex>& lk) const;
    inline bool closed(lock_guard<mutex>& lk) const;

    inline void throw_if_closed(unique_lock<mutex>&);

    inline void wait_until_not_empty(unique_lock<mutex>& lk);
    inline bool wait_until_not_empty_or_closed(unique_lock<mutex>& lk);

    inline void notify_not_empty_if_needed(unique_lock<mutex>& lk)
    {
      if (waiting_empty_ > 0)
      {
        --waiting_empty_;
        lk.unlock();
        not_empty_.notify_one();
      }
    }

  };

  template <class ValueType, class Queue>
  sync_queue_base<ValueType, Queue>::sync_queue_base() :
    waiting_empty_(0), data_(), closed_(false)
  {
    BOOST_ASSERT(data_.empty());
  }

  template <class ValueType, class Queue>
  sync_queue_base<ValueType, Queue>::~sync_queue_base()
  {
  }

  template <class ValueType, class Queue>
  void sync_queue_base<ValueType, Queue>::close()
  {
    {
      lock_guard<mutex> lk(mtx_);
      closed_ = true;
    }
    not_empty_.notify_all();
  }

  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::closed() const
  {
    lock_guard<mutex> lk(mtx_);
    return closed(lk);
  }
  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::closed(unique_lock<mutex>&) const
  {
    return closed_;
  }
  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::closed(lock_guard<mutex>&) const
  {
    return closed_;
  }

  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::empty() const
  {
    lock_guard<mutex> lk(mtx_);
    return empty(lk);
  }
  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::full() const
  {
    return false;
  }

  template <class ValueType, class Queue>
  typename sync_queue_base<ValueType, Queue>::size_type sync_queue_base<ValueType, Queue>::size() const
  {
    lock_guard<mutex> lk(mtx_);
    return size(lk);
  }

  template <class ValueType, class Queue>
  void sync_queue_base<ValueType, Queue>::throw_if_closed(unique_lock<mutex>&)
  {
    if (closed_)
    {
      BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
    }
  }

  template <class ValueType, class Queue>
  void sync_queue_base<ValueType, Queue>::wait_until_not_empty(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (! empty(lk)) break;
      throw_if_closed(lk);
      ++waiting_empty_;
      not_empty_.wait(lk);
    }
  }
  template <class ValueType, class Queue>
  bool sync_queue_base<ValueType, Queue>::wait_until_not_empty_or_closed(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (! empty(lk)) break;
      if (closed_) return true;
      ++waiting_empty_;
      not_empty_.wait(lk);
    }
     return false;
  }

} // detail
} // concurrent
} // boost

#include <boost/config/abi_suffix.hpp>

#endif