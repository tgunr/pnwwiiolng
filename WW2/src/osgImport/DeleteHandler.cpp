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
#include "osgImport/DeleteHandler.h"

DeleteHandler::DeleteHandler(int numberOfFramesToRetainObjects):
    _numFramesToRetainObjects(numberOfFramesToRetainObjects),
    _currentFrameNumber(0)
{
}

DeleteHandler::~DeleteHandler()
{
    // flushAll();
}

void DeleteHandler::flush()
{
	using DeletionList = std::list<const Referenced*>;
    DeletionList deletionList;

    {
        // gather all the objects to delete whilst holding the mutex to the _objectsToDelete
        // list, but delete the objects outside this scoped lock so that if any objects deleted
        // unref their children then no deadlock happens.
        int frameNumberToClearTo = _currentFrameNumber - _numFramesToRetainObjects;

        ObjectsToDeleteList::iterator itr;
        for(itr = _objectsToDelete.begin();
            itr != _objectsToDelete.end();
            ++itr)
        {
            if (itr->first > frameNumberToClearTo) break;

            deletionList.push_back(itr->second);

            itr->second = 0;
        }

        _objectsToDelete.erase( _objectsToDelete.begin(), itr);
    }

    for(DeletionList::iterator ditr = deletionList.begin();
        ditr != deletionList.end();
        ++ditr)
    {
        doDelete(*ditr);
    }
}

void DeleteHandler::flushAll()
{
    int temp_numFramesToRetainObjects = _numFramesToRetainObjects;
    _numFramesToRetainObjects = 0;

	using DeletionList = std::list<const Referenced*>;
    DeletionList deletionList;

    {
        // gather all the objects to delete whilst holding the mutex to the _objectsToDelete
        // list, but delete the objects outside this scoped lock so that if any objects deleted
        // unref their children then no deadlock happens.
        ObjectsToDeleteList::iterator itr = _objectsToDelete.begin();

        for(; itr != _objectsToDelete.end(); ++itr)
        {
            deletionList.push_back(itr->second);
            itr->second = 0;
        }

        _objectsToDelete.erase( _objectsToDelete.begin(), _objectsToDelete.end());
    }

    DeletionList::iterator ditr = deletionList.begin();

    for(; ditr != deletionList.end(); ++ditr)
    {
        doDelete(*ditr);
    }

    _numFramesToRetainObjects = temp_numFramesToRetainObjects;
}

void DeleteHandler::requestDelete(const Referenced* object)
{
    if(_numFramesToRetainObjects==0)
    {
        doDelete(object);
    }
    else
    {
        _objectsToDelete.push_back(FrameNumberObjectPair(_currentFrameNumber,object));
    }
}
