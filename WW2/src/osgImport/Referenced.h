#pragma once
/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

// When building OSG with Java need to derive from Noodle::CBridgable class,
// therefore so OSG_JAVA_BUILD must be defined. Also the thread-safe ref/unref test
// as built in for the NoodleGlue wrapping. NoodleGlue has a Garbage collector mechanism
// which is very similar to osg::DeletionManager. So these aspects of osg::Referenced
// have been removed
//#define OSG_JAVA_BUILD

// forward declare, declared after Referenced below.
class DeleteHandler;
class Observer;

/** Base class from providing referencing counted objects.*/
class Referenced
{
public:
    Referenced();

    explicit Referenced(bool threadSafeRefUnref);

    Referenced(const Referenced&);

    inline Referenced& operator = (const Referenced&) { return *this; }

	Referenced(Referenced&& rhs) = delete;
	Referenced& operator = (Referenced&&) = delete;

    /** Increment the reference count by one, indicating that
        this object has another pointer which is referencing it.*/
    inline void ref() const;

    /** Decrement the reference count by one, indicating that
        a pointer to this object is referencing it.  If the
        reference count goes to zero, it is assumed that this object
        is no longer referenced and is automatically deleted.*/
    inline void unref() const;

    /** Decrement the reference count by one, indicating that
        a pointer to this object is referencing it.  However, do
        not delete it, even if ref count goes to 0.  Warning, unref_nodelete()
        should only be called if the user knows exactly who will
        be responsible for, one should prefer unref() over unref_nodelete()
        as the later can lead to memory leaks.*/
    void unref_nodelete() const;

    /** Return the number pointers currently referencing this object. */
    inline int referenceCount() const { return _refCount; }

    /** Add a Observer that is observing this object, notify the Observer when this object gets deleted.*/
    void addObserver(Observer* observer);

    /** Add a Observer that is observing this object, notify the Observer when this object gets deleted.*/
    void removeObserver(Observer* observer);

public:

    /** Set whether reference counting should be use a mutex to create thread reference counting.*/
    static void setThreadSafeReferenceCounting(bool enableThreadSafeReferenceCounting);

    /** Get whether reference counting is active.*/
    static bool getThreadSafeReferenceCounting();

    friend class DeleteHandler;

    /** Set a DeleteHandler to which deletion of all referenced counted objects
      * will be delegated to.*/
    static void setDeleteHandler(DeleteHandler* handler);

    /** Get a DeleteHandler.*/
    static DeleteHandler* getDeleteHandler();


protected:

    virtual ~Referenced();

    void deleteUsingDeleteHandler() const;

    mutable int                     _refCount;

    void*                           _observers;
};

inline void Referenced::ref() const
{
    ++_refCount;
}

inline void Referenced::unref() const
{
    --_refCount;
    bool needDelete = _refCount<=0;

    if (needDelete)
    {
        if (getDeleteHandler()) deleteUsingDeleteHandler();
        else delete this;
    }
}

