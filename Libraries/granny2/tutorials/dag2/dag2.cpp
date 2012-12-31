// =======================================================================================
// blending.cpp: Demonstrates using control weights to blend animations
//
// Copyright 1999-2006 by RAD Game Tools, Inc., All Rights Reserved.
// =======================================================================================
#include "dag2.h"
#include <assert.h>
#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

// Our logging callback
GRANNY_CALLBACK(granny_file_info*) SourceBindingCallback(char const* SourceName, void* UserData);
GRANNY_CALLBACK(void) LogCallback(granny_log_message_type Type,
                                  granny_log_message_origin Origin,
                                  char const* File, granny_int32x Line,
                                  char const * Message,
                                  void * UserData);

// See the warning note in the Blend Graph document about source names.  This little bit
// of ugliness will be vanishing shortly.
char const* SourcePrefixToRemove = "D:\\dev\\rad\\granny\\tutorial\\dag2\\";

using namespace std;

char const* const GlobalSceneFilename = "phase_matched\\model.gr2";
char const* const GlobalDAGFilename   = "phase_matched.dg2";

// ---------------------------------------------------------
// Scene instance objects
extern DemoScene GlobalScene;

float Clamp(float const Input, float const Min, float const Max)
{
    assert(Min <= Max);

    if (Input < Min)
        return Min;
    else if (Input > Max)
        return Max;

    return Input;
}

/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_Introduction, The New DAG) */
/* This tutorial will introduce you to many of the new APIs for the updated Blend Graph.
   Most of the standard demo implementation details have been stripped out of the
   documentation, but as always, if you'd like to see exactly how the functions below are
   pieced together, you can find the full source in tutorials/dag2/dag2.cpp in your Granny
   distribution.  For more information on the functions you'll encounter here, you may
   want to read $BlendGraph in parallel with this tutorial.
*/

/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_LoadAndBind, LoadAndBindDAG) */
/* Since a Blend Graph lives in a $file container, the first part of this will look very
   familiar to you if you've ever loaded a $file_info structure.
 */
bool LoadAndBindDAG()
{
    GlobalScene.DagFile = GrannyReadEntireFile(GlobalDAGFilename);
    if (!GlobalScene.DagFile)
        return false;

    GlobalScene.DagInfo = GrannyGetDag2Info(GlobalScene.DagFile);
    if (!GlobalScene.DagInfo)
        return false;

    /* Now we get into some differences.  Out of the box, a $dag2_info doesn't contain any
       of the animations that were specified in the editor.  We have to plug those in
       ourselves.  See $BlendGraph for details.
     */
    if (!GrannyBindDag2Sources(GlobalScene.DagInfo->DAGs[0], SourceBindingCallback, 0))
        return false;

    /* By this point, we now have a fully functional Graph, that describes the immutable
       parts of the structure.  (Node linkages, animation data, defaults, etc.)  We need
       to instantiate the Graph, in basically the same way we instantiate $model objects,
       so we can impose character-specific changes.
    */
    GlobalScene.DagInstance =
        GrannyInstantiateDAG(GlobalScene.DagInfo->DAGs[0],
                             GrannyGetSourceModel(GlobalScene.Model->ModelInstance),
                             GrannyGetSourceModel(GlobalScene.Model->ModelInstance));
    if (!GlobalScene.DagInstance)
        return false;

    // Success!
    return true;
}


/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_SourceBindingCallback, SourceBindingCallback) */
/* In order to prevent the same animation data from being present in multiple Blend
   Graphs, each graph is loaded without any animation, model, or skeleton data present.
   Before instantiating the Graph for a specific character, all of that data must be
   filled in.  We do that with the $BindDag2Sources API, and a $dag2_source_callback.

   In the simplest case, the string comes in that represents the filename of the animation
   source, we load that file, and return the $file_info structure to the Graph API.  We're
   going to demonstrate the use of a backing map here to show how you would track and
   return the same $file_info across multiple loads.  The internals of that backing map
   aren't particularly interesting, but you can find them at the end of the dag2.cpp
   tutorial implementation file.
 */
// Tiny API for the Source map
void NoteSource(char const* SourceName,
                granny_file* File,
                granny_file_info* Info);
granny_file_info* LookupSource(char const* SourceName);
/*
 */
GRANNY_CALLBACK(granny_file_info*) SourceBindingCallback(char const* SourceName, void* /*UserData*/)
{
    /* See if we've already loaded this source */
    granny_file_info* Source = LookupSource(SourceName);
    if (Source)
        return Source;

    /* Ok, the look up failed so we have to actually read the file.  Note that the
       ugliness with the prefix string is due to the fact that source names currently are
       the absolute path of the source .gr2 on the creating machine.
    */
    char const* NameToLoad = SourceName;
    if (strncmp(NameToLoad, SourcePrefixToRemove, strlen(SourcePrefixToRemove)) == 0)
        NameToLoad += strlen(SourcePrefixToRemove);

    granny_file* File = GrannyReadEntireFile(NameToLoad);
    if (!File)
        return 0;

    Source = GrannyGetFileInfo(File);
    if (Source)
    {
        // Make the new entry in the source map so we don't reload this later
        NoteSource(SourceName, File, Source);
    }

    return Source;
}

/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_KeyEvent, KeyEvent) */
/* Once we have the Graph loaded and bound, it's not much good unless we play around with
   the parameters at runtime, obviously.  dag2_helper.cpp routes key events to this
   function, where we'll just detect a few mnemonics, and use that to change the state of
   the "Motions" state machine node in the example Graph.  If you've replaced the example
   graph with your own blend structure, you may want to plug in some manipulation tests
   here for convenience.
 */
void KeyEvent(char)
{
}

/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_RenderModel, SamplePosesForGraph) */
void SamplePosesForGraph()
{
    /* Here's the meat of the display process.  We call $SampleDAG, specifying that we
       want to sample the first (0 index) output edge.  In the present version of the
       library, the root node only has one valid output edge, so you can treat that
       parameter as fixed.

       The CurrentTime parameter must be passed in here, as opposed to the
       $SampleModelAnimations convention, which is to use the implicit clock set with
       either $SetModelClock, or one of the $SetControlClock family of APIs.

       A note about the lifetime of the returned pose object.  It is recommended that you
       use the returned pointer to construct a $world_pose immediately following the
       sample, and then don't reference it subsequently.  The $local_pose belongs to the
       cache, and you don't need to free it.  In technical terms, the pose will be valid
       until the next use of that specific pose cache, but don't rely on it!  If you need
       a persistent copy, use $CopyLocalPose to move it into an allocated $local_pose that
       you control.
     */
    granny_local_pose* Pose =
        GrannySampleDAG(GlobalScene.DagInstance, 0,
                        GlobalScene.CurrentTime,
                        GrannyGetGlobalDagPoseCache());

    granny_skeleton* Skeleton = GrannyGetSourceSkeleton(GlobalScene.Model->ModelInstance);
    GrannyBuildWorldPose(Skeleton,
                         0, Skeleton->BoneCount,
                         Pose, NULL, GlobalScene.SharedWorldPose);
}


/* DEFTUTORIAL EXPGROUP(TutorialDAG2) (TutorialDAG2_Summary, Summary) */
/* If you've looked at the source for this tutorial, you know we've elided a lot of glue
   code, but the small amount of Blend Graph specific material that sets this example
   apart from the basic DirectX rendering tutorial has been presented here in full.  Part
   of the redesign criteria for the Blend Graph was that it should be much easier to setup
   and work with the structures; there's simply not much more that you need to know to
   accurately sample the blend structures your artists create.  Please do read $BlendGraph
   for more information, and send us any suggestions!
 */

/* DEFTUTORIALEND */

GRANNY_CALLBACK(void) LogCallback(granny_log_message_type Type,
                                  granny_log_message_origin Origin,
                                  char const* File, granny_int32x Line,
                                  char const * Message,
                                  void* /*UserData*/)
{
    assert(Message);

    /* Granny provides helper functions to get printable strings for the message type and
       origin. */
    char const* TypeString   = GrannyGetLogMessageTypeString(Type);
    char const* OriginString = GrannyGetLogMessageOriginString(Origin);

    /* We're just going to dump to STDOUT in this demo.  You can route the messages to the
       filesystem, or a network interface if you need to. */
    char Buffer[512];
    sprintf(Buffer, "%s (%s)\n"
        "  %s(%d): %s\n", TypeString, OriginString, File, Line, Message);
    OutputDebugString(Buffer);
}


struct source_entry
{
    granny_file*      File;
    granny_file_info* FileInfo;
};

typedef map<string, source_entry> source_map;
source_map LoadedSources;

// Free and release
void FlushSourceMap()
{
    {for(source_map::iterator Itr = LoadedSources.begin(); Itr != LoadedSources.end(); ++Itr)
    {
        source_entry& Entry = Itr->second;
        GrannyFreeFile(Entry.File);
        Entry.File = 0;
        Entry.FileInfo = 0;
    }}

    LoadedSources.clear();
}

void NoteSource(char const*       SourceName,
                granny_file*      File,
                granny_file_info* Info)
{
    source_entry NewEntry = { File, Info };
    LoadedSources[SourceName] = NewEntry;
}

granny_file_info* LookupSource(char const* SourceName)
{
    source_map::iterator Itr = LoadedSources.find(SourceName);
    if (Itr != LoadedSources.end())
        return Itr->second.FileInfo;

    return 0;
}

INT WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, INT )
{
    if(!GrannyVersionsMatch)
    {
        printf("Warning: the Granny DLL currently loaded "
               "doesn't match the .h file used during compilation\n");
        return EXIT_FAILURE;
    }

    granny_log_callback NewCallback;
    NewCallback.Function = LogCallback;
    NewCallback.UserData = NULL;
    GrannySetLogCallback(&NewCallback);

    if (InitializeD3D() == false)
        return EXIT_FAILURE;

    char buffer[512];
    GetCurrentDirectory(sizeof(buffer), buffer);

    if ((GlobalScene.SceneFile     = GrannyReadEntireFile(GlobalSceneFilename)) == NULL ||
        (GlobalScene.SceneFileInfo = GrannyGetFileInfo(GlobalScene.SceneFile)) == NULL)
    {
        return EXIT_FAILURE;
    }

    if (CreateDemoTextures() == false || CreateDemoModel() == false ||
        CreateSharedPoses() == false  || InitCameraAndLights() == false)
    {
        return EXIT_FAILURE;
    }

    if (!LoadAndBindDAG())
        return EXIT_FAILURE;

    /* Enter our "game" loop.  This can be mostly ignored if you've already seen this loop in $TutorialDX9Rendering. */
    granny_system_clock StartClock;
    GrannyGetSystemSeconds(&StartClock);
    granny_system_clock LastClock = StartClock;

    GlobalRunning = true;
    while (GlobalRunning)
    {
        // Extract the current time and the frame delta
        {
            granny_system_clock CurrClock;
            GrannyGetSystemSeconds(&CurrClock);

            // Ignore clock recentering issues for this example
            float Speed = 1;
            GlobalScene.CurrentTime = Speed * GrannyGetSecondsElapsed(&StartClock, &CurrClock);
            GlobalScene.DeltaTime   = Speed * GrannyGetSecondsElapsed(&LastClock, &CurrClock);
            LastClock = CurrClock;
        }

        InvalidateRect(g_hwnd, 0, false);
        MSG Message;
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

    FlushSourceMap();
    CleanupD3D();
    return EXIT_SUCCESS;
}


