#pragma once

#include "osgImport/Referenced.h"

#include <list>

class DeleteHandler
{
public:
    typedef std::pair<int, const Referenced*> FrameNumberObjectPair;
    typedef std::list<FrameNumberObjectPair> ObjectsToDeleteList;

    DeleteHandler(int numberOfFramesToRetainObjects=0);

    virtual ~DeleteHandler();

    /** Set the number of frames to retain objects that are have been requested for deletion.
      * When set to zero objects are deleted immediately, by set to 1 there are kept around for an extra frame etc.
      * The ability to retain objects for several frames is useful to prevent premature deletion when objects
      * are still be used the graphics threads that are using double buffering of rendering data structures with
      * non ref_ptr<> pointers to scene graph elements.*/
    void setNumFramesToRetainObjects(int numberOfFramesToRetainObjects) {  _numFramesToRetainObjects = numberOfFramesToRetainObjects; }

    int getNumFramesToRetainObjects() const { return _numFramesToRetainObjects; }

    /** Set the current frame number so that subsequent deletes get tagged as associated with this frame.*/
    void setFrameNumber(int frameNumber) { _currentFrameNumber = frameNumber; }

    /** Get the current frame number.*/
    int getFrameNumber() const { return _currentFrameNumber; }

    inline void doDelete(const Referenced* object) { delete object; }

    /** Flush objects that ready to be fully deleted.*/
    virtual void flush();

    /** Flush all objects that the DeleteHandler holds.
      * Note, this should only be called if there are no threads running with non ref_ptr<> pointers, such as graphics threads.*/
    virtual void flushAll();

    /** Request the deletion of an object.
      * Depending on users implementation of DeleteHandler, the delete of the object may occur
      * straight away or be delayed until doDelete is called.
      * The default implementation does a delete straight away.*/
    virtual void requestDelete(const Referenced* object);

protected:
    DeleteHandler(const DeleteHandler&)
        : _numFramesToRetainObjects(0),
          _currentFrameNumber(0) {}

    DeleteHandler operator = (const DeleteHandler&) {return *this;}

    int _numFramesToRetainObjects;
    int _currentFrameNumber;
    ObjectsToDeleteList _objectsToDelete;
};

