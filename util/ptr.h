/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef NDNFD_UTIL_PTR_H_
#define NDNFD_UTIL_PTR_H_

#include <stdint.h>
#include "assert.h"

namespace ndnfd {

/**
 * \ingroup core
 * \defgroup ptr Smart Pointer
 */
/**
 * \ingroup ptr
 *
 * \brief smart pointer class similar to boost::intrusive_ptr
 *
 * This smart-pointer class assumes that the underlying
 * type provides a pair of Ref and Unref methods which are
 * expected to increment and decrement the internal refcount
 * of the object instance.
 *
 * This implementation allows you to manipulate the smart pointer
 * as if it was a normal pointer: you can compare it with zero,
 * compare it against other pointers, assign zero to it, etc.
 *
 * It is possible to extract the raw pointer from this
 * smart pointer with the GetPointer and PeekPointer methods.
 *
 * If you want to store a newed object into a smart pointer,
 * we recommend you to use the Create template functions
 * to create the object and store it in a smart pointer to avoid
 * memory leaks. These functions are really small convenience
 * functions and their goal is just is save you a small
 * bit of typing.
 */
template <typename T>
class Ptr 
{
private:
  T *m_ptr;
  class Tester {
private:
    void operator delete (void *);
  };
  friend class Ptr<const T>;
  template <typename U>
  friend U *GetPointer (const Ptr<U> &p);
  template <typename U>
  friend U *PeekPointer (const Ptr<U> &p);

  inline void Acquire (void) const;
public:
  /**
   * Create an empty smart pointer
   */
  Ptr ();
  /**
   * \param ptr raw pointer to manage
   *
   * Create a smart pointer which points to the object pointed to by
   * the input raw pointer ptr. This method creates its own reference
   * to the pointed object. The caller is responsible for Unref()'ing
   * its own reference, and the smart pointer will eventually do the
   * same, so that object is deleted if no more references to it
   * remain.
   */
  Ptr (T *ptr);
  /**
   * \param ptr raw pointer to manage
   * \param ref if set to true, this method calls Ref, otherwise,
   *        it does not call Ref.
   *
   * Create a smart pointer which points to the object pointed to by
   * the input raw pointer ptr.
   */
  Ptr (T *ptr, bool ref);
  Ptr (Ptr const&o);
  // allow conversions from T to T const.
  template <typename U>
  Ptr (Ptr<U> const &o);
  ~Ptr ();
  Ptr<T> &operator = (Ptr const& o);

  T *operator -> () const;
  T *operator -> ();
  const T &operator * () const;
  T &operator * ();
  // allow if (!sp)
  bool operator! ();
  // allow if (sp)
  // disable delete sp
  operator Tester * () const;
};


/**
 * \relates Ptr
 * \param p smart pointer
 * \return the pointer managed by this smart pointer.
 *
 * The underlying refcount is not incremented prior
 * to returning to the caller so the caller is not
 * responsible for calling Unref himself.
 */
template <typename T>
T * PeekPointer (const Ptr<T> &p);

/**
 * \relates Ptr
 * \param p smart pointer
 * \return the pointer managed by this smart pointer.
 *
 * The underlying refcount is incremented prior
 * to returning to the caller so the caller is
 * responsible for calling Unref himself.
 */
template <typename T>
T * GetPointer (const Ptr<T> &p);



// allow if (sp == 0)
template <typename T1, typename T2>
bool operator == (Ptr<T1> const &lhs, T2 const *rhs);

// allow if (0 == sp)
template <typename T1, typename T2>
bool operator == (T1 const *lhs, Ptr<T2> &rhs);

// allow if (sp != 0)
template <typename T1, typename T2>
bool operator != (Ptr<T1> const &lhs, T2 const *rhs);

// allow if (0 != sp)
template <typename T1, typename T2>
bool operator != (T1 const *lhs, Ptr<T2> &rhs);

// allow if (sp0 == sp1)
template <typename T1, typename T2>
bool operator == (Ptr<T1> const &lhs, Ptr<T2> const &rhs);

// allow if (sp0 != sp1)
template <typename T1, typename T2>
bool operator != (Ptr<T1> const &lhs, Ptr<T2> const &rhs);


} // namespace ndnfd


namespace ndnfd {

/*************************************************
 *  friend non-member function implementations
 ************************************************/

template <typename T>
T * PeekPointer (const Ptr<T> &p)
{
  return p.m_ptr;
}

template <typename T>
T * GetPointer (const Ptr<T> &p)
{
  p.Acquire ();
  return p.m_ptr;
}

template <typename T1, typename T2>
bool 
operator == (Ptr<T1> const &lhs, T2 const *rhs)
{
  return PeekPointer (lhs) == rhs;
}

template <typename T1, typename T2>
bool 
operator == (T1 const *lhs, Ptr<T2> &rhs)
{
  return lhs == PeekPointer (rhs);
}

template <typename T1, typename T2>
bool 
operator != (Ptr<T1> const &lhs, T2 const *rhs)
{
  return PeekPointer (lhs) != rhs;
}

template <typename T1, typename T2>
bool 
operator != (T1 const *lhs, Ptr<T2> &rhs)
{
  return lhs != PeekPointer (rhs);
}

template <typename T1, typename T2>
bool 
operator == (Ptr<T1> const &lhs, Ptr<T2> const &rhs)
{
  return PeekPointer (lhs) == PeekPointer (rhs);
}

template <typename T1, typename T2>
bool 
operator != (Ptr<T1> const &lhs, Ptr<T2> const &rhs)
{
  return PeekPointer (lhs) != PeekPointer (rhs);
}

/****************************************************
 *      Member method implementations.
 ***************************************************/

template <typename T>
void 
Ptr<T>::Acquire (void) const
{
  if (m_ptr != 0)
    {
      m_ptr->Ref ();
    }
}

template <typename T>
Ptr<T>::Ptr ()
  : m_ptr (0)
{
}

template <typename T>
Ptr<T>::Ptr (T *ptr)
  : m_ptr (ptr)
{
  Acquire ();
}

template <typename T>
Ptr<T>::Ptr (T *ptr, bool ref)
  : m_ptr (ptr)
{
  if (ref)
    {
      Acquire ();
    }
}

template <typename T>
Ptr<T>::Ptr (Ptr const&o) 
  : m_ptr (PeekPointer (o))
{
  Acquire ();
}
template <typename T>
template <typename U>
Ptr<T>::Ptr (Ptr<U> const &o)
  : m_ptr (PeekPointer (o))
{
  Acquire ();
}

template <typename T>
Ptr<T>::~Ptr () 
{
  if (m_ptr != 0) 
    {
      m_ptr->Unref ();
    }
}

template <typename T>
Ptr<T> &
Ptr<T>::operator = (Ptr const& o) 
{
  if (&o == this)
    {
      return *this;
    }
  if (m_ptr != 0) 
    {
      m_ptr->Unref ();
    }
  m_ptr = o.m_ptr;
  Acquire ();
  return *this;
}

template <typename T>
T *
Ptr<T>::operator -> () 
{
  return m_ptr;
}

template <typename T>
T *
Ptr<T>::operator -> () const
{
  return m_ptr;
}

template <typename T>
const T &
Ptr<T>::operator * () const
{
  return *m_ptr;
}

template <typename T>
T &
Ptr<T>::operator * ()
{
  return *m_ptr;
}

template <typename T>
bool 
Ptr<T>::operator! () 
{
  return m_ptr == 0;
}

template <typename T>
Ptr<T>::operator Tester * () const
{
  if (m_ptr == 0) 
    {
      return 0;
    }
  static Tester test;
  return &test;
}


} // namespace ndnfd

#endif//NDNFD_UTIL_PTR_H_
