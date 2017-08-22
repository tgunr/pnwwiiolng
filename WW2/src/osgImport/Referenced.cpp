#include "syscfg.h" // Path is project specific.
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
#include <stdlib.h>

#include "osgImport/Referenced.h"
#include "log/log.h"

#include <typeinfo>
#include <memory>
#include <set>

#include "osgImport/DeleteHandler.h"

//#define DEBUG_OBJECT_ALLOCATION_DESTRUCTION

// specialized smart pointer, used to get round auto_ptr<>'s lack of the destructor reseting itself to 0.
struct DeleteHandlerPointer
{
    DeleteHandlerPointer():
        _ptr(0) {}

    DeleteHandlerPointer(DeleteHandler* ptr):
        _ptr(ptr) {}

    ~DeleteHandlerPointer()
    {
        delete _ptr;
        _ptr = 0;
    }

    inline DeleteHandlerPointer& operator = (DeleteHandler* ptr)
    {
        if (_ptr==ptr) return *this;
        delete _ptr;
        _ptr = ptr;
        return *this;
    }

    void reset(DeleteHandler* ptr)
    {
        if (_ptr==ptr) return;
        delete _ptr;
        _ptr = ptr;
    }

    inline DeleteHandler& operator*()  { return *_ptr; }

    inline const DeleteHandler& operator*() const { return *_ptr; }

    inline DeleteHandler* operator->() { return _ptr; }

    inline const DeleteHandler* operator->() const   { return _ptr; }

    DeleteHandler* get() { return _ptr; }

    const DeleteHandler* get() const { return _ptr; }

    DeleteHandler* _ptr;
};

static DeleteHandlerPointer s_deleteHandler(0);

void Referenced::setDeleteHandler(DeleteHandler* handler)
{
    s_deleteHandler.reset(handler);
}

DeleteHandler* Referenced::getDeleteHandler()
{
    return s_deleteHandler.get();
}

Referenced::Referenced():
    _refCount(0),
    _observers(0)
{
}

Referenced::Referenced(const Referenced&):
    _refCount(0),
    _observers(0)
{
}

Referenced::~Referenced()
{
    if (_refCount>0)
    {
        Write_Log(LOG_OLD, LOG_WARN, "Warning: deleting still referenced object. Refcount = %d", _refCount);
    }
}

void Referenced::unref_nodelete() const
{
    --_refCount;
}

void Referenced::deleteUsingDeleteHandler() const
{
    getDeleteHandler()->requestDelete(this);
}
