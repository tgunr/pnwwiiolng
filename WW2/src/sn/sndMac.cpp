#include "syscfg.h" // Path is project specific.

#include "sn/sndsSoundControlAL.h"
#include "sn/sndsSoundTypes.h"
#include "include/vars.h"
#include "sn/OpenALAudio.h"

#include <stdio.h>


FILE* mDebugStream;

OpenALAudio* audio;

void soundsTest();
void test(const char* name);

LOGICAL sndsPlatformInit()
{
    SHOW_DEBUG( mDebugStream = fopen("/Users/awulf/XCode/test.log", "w");

               fprintf( mDebugStream, "sndsPlatformInit\n" ); fflush( mDebugStream ); )

    audio = new OpenALAudio(mDebugStream, MaxNumberOfSounds);

    if ( audio )
    {
        OSErr err = audio->init();
        if ( err == noErr ) err = audio->start();

        DEBUGSTR2( "sndsPlatformInit err was %d\n", err );

        //soundsTest();
        return err == noErr;
    }
    return TRUE;
}

void sndsPlatformKill()
{
    SHOW_DEBUG( fprintf( mDebugStream, "sndsPlatformKill()\n" ); fflush( mDebugStream ); fclose( mDebugStream ); )

    if ( audio )
    {
        OSErr err = audio->kill();
        (void)err ;

        DEBUGSTR2( "sndsPlatformKill err was %d\n", err );

        delete audio;
    }
}

UINT32 sndsGetMaxNumberOfSounds()
{
    return MaxNumberOfSounds;
}

LOGICAL sndsPlatformGetVolume(snSound* snd, SINT32* vol)
{
    *vol = 0; // this is never called
    return TRUE;
}

LOGICAL sndsPlatformCommitDeferredSettings()
{
    return TRUE; // PC only
}

LOGICAL sndsPlatformOverLimit(UINT32 numPlaying)
{
    //DEBUGSTR2( "sndsPlatformOverLimit(%d)\n", numPlaying );
	return numPlaying > sndsGetMaxNumberOfSounds();
}

LOGICAL sndsPlatformDriverConfig(void* hints)
{
    return TRUE; // not called
}

LOGICAL sndsPlatformReset()
{
    return TRUE; // PC only
}

LOGICAL sndsPlatformSetPanPosition(snSound* snd)
{
	return TRUE; // use 3D instead
}

// CALLED API ---------------------------------------------------------------------------

#define CALLAUDIO(x) (audio ? x : 0 )

LOGICAL sndsPlatformCreateSound(snSound* lpOld, snSound* lpNew)
{
    DEBUGSTR3( "sndsPlatformCreateSound(%s,%s)\n", lpOld->name, lpNew->name );

	return CALLAUDIO(audio->createSound(lpOld,lpNew)) == noErr;
}

LOGICAL sndsPlatformDuplicateSound(snSound* to, const snSound* from)
{
    DEBUGSTR3( "sndsPlatformDuplicateSound(%s,%s)\n", to->name, from->name );

	return CALLAUDIO(audio->duplicateSound(from,to)) == noErr;
}

LOGICAL sndsPlatformFreeSound(snSound* snd)
{
    DEBUGSTR2( "sndsPlatformFreeSound(%s)\n", snd->name );

    return CALLAUDIO(audio->freeSound(snd)) == noErr;
}

LOGICAL sndsPlatformIsPlaying(snSound* snd)
{
    //DEBUGSTR2( "sndsPlatformIsPlaying(%s)\n", snd->name );

    return CALLAUDIO(audio->isPlaying(snd));
}

LOGICAL sndsPlatformLoadSound(snSound* snd)
{
    DEBUGSTR2( "sndsPlatformLoadSound(%s)\n", snd->name );

    OSStatus err = CALLAUDIO(audio->loadSound(snd));
    if ( err != noErr )
    {
        DEBUGSTR3( "**** sndsPlatformLoadSound(%s) error was %d\n", snd->name, (int)err );
    }

    return err == noErr;
}

LOGICAL sndsPlatformReloadSound(snSound* oldSound, snSound* newSound)
{
    DEBUGSTR3( "sndsPlatformReloadSound(%s,%s)\n", oldSound->name, newSound->name );

    return CALLAUDIO(audio->reloadSound(oldSound,newSound)) == noErr;
}

LOGICAL sndsPlatformSetConeAngles(snSound* snd, UINT32 unused)
{
    return FALSE;
}

LOGICAL sndsPlatformSetConeOrientation(snSound* snd, UINT32 unused)
{
    return FALSE;
}

LOGICAL sndsPlatformSetCurrentPosition(snSound* snd)
{
    return TRUE;
}

LOGICAL sndsPlatformSetFrequency(snSound* snd)
{
    DEBUGSTR3( "sndsPlatformSetFrequency(%s, %lf)\n", snd->name, snd->freqShift );

    return CALLAUDIO(audio->setFrequency(snd)) == noErr;
}

LOGICAL sndsPlatformSetListener3DPos(piVECTOR* r, UINT32 unused)
{
    //DEBUGSTR4( "sndsPlatformSetListener3DPos(%f, %f, %f)\n", r->x, r->y, r->z );

    return CALLAUDIO(audio->setListener3DPos(r)) == noErr;
}

LOGICAL sndsPlatformSetListener3DVel(piVECTOR* v, UINT32 unused)
{
    //DEBUGSTR4( "sndsPlatformSetListener3DVel(%f, %f, %f)\n", v->x, v->y, v->z );

    return CALLAUDIO(audio->setListener3DVel(v)) == noErr;
}

LOGICAL sndsPlatformSetMaxDistance(snSound* snd, UINT32 unused)
{
    DEBUGSTR3( "sndsPlatformSetMaxDistance(%s, %f)\n", snd->name, snd->fMaxDistance );

    return CALLAUDIO(audio->setMaxDistance(snd)) == noErr;
}

LOGICAL sndsPlatformSetMinDistance(snSound* snd, UINT32 unused)
{
    DEBUGSTR3( "sndsPlatformSetMinDistance(%s, %f)\n", snd->name, snd->fMinDistance );

    return CALLAUDIO(audio->setMinDistance(snd)) == noErr;
}

LOGICAL sndsPlatformSetOrientation(piVECTOR* at, piVECTOR* up, UINT32 unused)
{
    //DEBUGSTR2( "sndsPlatformSetOrientation(%x,%x)\n", at, up );

    return CALLAUDIO(audio->setOrientation(at, up)) == noErr;
}

LOGICAL sndsPlatformSetSound3DPos(snSound* snd, UINT32 unused)
{
    //DEBUGSTR5( "sndsPlatformSetSound3DPos(%s (%lf, %lf, %lf)\n", snd->name, snd->position.x, snd->position.y, snd->position.z );

    return CALLAUDIO(audio->setSound3DPos(snd)) == noErr;
}

LOGICAL sndsPlatformSetSound3DVel(snSound* snd, UINT32 unused)
{
    //DEBUGSTR5( "sndsPlatformSetSound3DVel(%s (%lf, %lf, %lf)\n", snd->name, snd->velocity.x, snd->velocity.y, snd->velocity.z );

    return CALLAUDIO(audio->setSound3DVel(snd)) == noErr;
}

LOGICAL sndsPlatformSetSound3DMode(snSound* snd, UINT32 unused)
{
    return TRUE;
}

LOGICAL sndsPlatformSetVolume(snSound* snd)
{
    DEBUGSTR3( "sndsPlatformSetVolume(%s, %lf)\n", snd->name, snd->volume );

    return CALLAUDIO(audio->setVolume(snd,VolumeMultiplier))==noErr;
}

LOGICAL sndsPlatformStartSound(snSound* snd)
{
    DEBUGSTR2( "sndsPlatformStartSound(%s)\n", snd->name );

    return CALLAUDIO(audio->startSound(snd)) == noErr;
}

LOGICAL sndsPlatformStopSound(snSound* snd)
{
    DEBUGSTR2( "sndsPlatformStopSound(%s)\n", snd->name );

    return CALLAUDIO(audio->stopSound(snd)) == noErr;
}

void soundsTest()
{
    test( "sounds/20mml.wav" );
    test( "sounds/20mml1.wav" );
    test( "sounds/20mmr.wav" );
    test( "sounds/20mmr1.wav" );
    test( "sounds/2lbs.wav" );
    test( "sounds/2lbsxgun.wav" );
    test( "sounds/37mm.wav" );
    test( "sounds/37mmhv.wav" );
    test( "sounds/37mmxgun.wav" );
    test( "sounds/3dbomb.wav" );
    test( "sounds/50cal600rpm.wav" );
    test( "sounds/50cal600rpm_trail.wav" );
    test( "sounds/50calh811.wav" );
    test( "sounds/50calh812.wav" );
    test( "sounds/5inch.wav" );
    test( "sounds/a13/2lbsxgun.wav" );
    test( "sounds/a13/coax.wav" );
    test( "sounds/a13/engine.wav" );
    test( "sounds/a13/idle.wav" );
    test( "sounds/a13/xeng.wav" );
    test( "sounds/a13/xgun2.wav" );
    test( "sounds/airraidsiren.wav" );
    test( "sounds/airstart.wav" );
    test( "sounds/ak-1/engine.wav" );
    test( "sounds/ambience/bird1.wav" );
    test( "sounds/ambience/bird2.wav" );
    test( "sounds/ambience/bird3.wav" );
    test( "sounds/ambience/bird4.wav" );
    test( "sounds/ambience/bird5.wav" );
    test( "sounds/ambience/bird6.wav" );
    test( "sounds/ambience/bird7.wav" );
    test( "sounds/ambience/bird8.wav" );
    test( "sounds/ambience/bird9.wav" );
    test( "sounds/ambience/cricket1.wav" );
    test( "sounds/ambience/cricket2.wav" );
    test( "sounds/ambience/dog1.wav" );
    test( "sounds/ambience/dog2.wav" );
    test( "sounds/ambience/fly1.wav" );
    test( "sounds/ambience/fly2.wav" );
    test( "sounds/ambience/fly3.wav" );
    test( "sounds/ambience/frog1.wav" );
    test( "sounds/ambience/gust1.wav" );
    test( "sounds/ambience/gust2.wav" );
    test( "sounds/ambience/ocean.wav" );
    test( "sounds/ambience/owl1.wav" );
    test( "sounds/ambience/owl2.wav" );
    test( "sounds/autogun-34.wav" );
    test( "sounds/autoloader.wav" );
    test( "sounds/bedford/idle.wav" );
    test( "sounds/bedford/starter.wav" );
    test( "sounds/bedford/xeng.wav" );
    test( "sounds/bell.wav" );
    test( "sounds/bell2.wav" );
    test( "sounds/bf109e3/engine.wav" );
    test( "sounds/bf109e3/horn.wav" );
    test( "sounds/bf109e3/idle.wav" );
    test( "sounds/bf109e3/mg17l.wav" );
    test( "sounds/bf109e3/mg17r.wav" );
    test( "sounds/bf109e3/mgffl.wav" );
    test( "sounds/bf109e3/mgffr.wav" );
    test( "sounds/bf109e3/prop.wav" );
    test( "sounds/bf109e3/slats.wav" );
    test( "sounds/bf109e3/xeng.wav" );
    test( "sounds/bf109e3/xgun1.wav" );
    test( "sounds/bf109e3/xgun2.wav" );
    test( "sounds/bf110c/engine.wav" );
    test( "sounds/bf110c/idle.wav" );
    test( "sounds/bf110c/prop.wav" );
    test( "sounds/bf110c/prop2.wav" );
    test( "sounds/bf110c/xeng.wav" );
    test( "sounds/bleniv/engine.wav" );
    test( "sounds/bleniv/idle.wav" );
    test( "sounds/bleniv/prop.wav" );
    test( "sounds/bleniv/xeng.wav" );
    test( "sounds/bleniv/xgun.wav" );
    test( "sounds/bof.wav" );
    test( "sounds/bofloader.wav" );
    test( "sounds/boforsxgun.wav" );
    test( "sounds/boys55atr.wav" );
    test( "sounds/bulletby.wav" );
    test( "sounds/canopy_close.wav" );
    test( "sounds/canopy_closeh.wav" );
    test( "sounds/canopy_closer.wav" );
    test( "sounds/canopy_open.wav" );
    test( "sounds/canopy_openc.wav" );
    test( "sounds/canopy_openh.wav" );
    test( "sounds/canopy_openr.wav" );
    test( "sounds/canopy_slidec.wav" );
    test( "sounds/canopy_slideo.wav" );
    test( "sounds/casing.wav" );
    test( "sounds/Charb1/47mm.wav" );
    test( "sounds/Charb1/75mm.wav" );
    test( "sounds/Charb1/engine.wav" );
    test( "sounds/Charb1/idle.wav" );
    test( "sounds/Charb1/manloader.wav" );
    test( "sounds/Charb1/mle_m31.wav" );
    test( "sounds/Charb1/xeng.wav" );
    test( "sounds/Charb1/xgun1.wav" );
    test( "sounds/Charb1/xgun1_distant.wav" );
    test( "sounds/Charb1/xgun2.wav" );
    test( "sounds/Charb1/xgun3.wav" );
    test( "sounds/clipreload.wav" );
    test( "sounds/crashexp.wav" );
    test( "sounds/crusader3/57mmtgun.wav" );
    test( "sounds/d520/idle.wav" );
    test( "sounds/d520/xeng.wav" );
    test( "sounds/db7/engine.wav" );
    test( "sounds/db7/idle.wav" );
    test( "sounds/db7/prop.wav" );
    test( "sounds/destroy.wav" );
    test( "sounds/electric_turret_elevate.wav" );
    test( "sounds/electric_turret_traverse.wav" );
    test( "sounds/enfieldreload.wav" );
    test( "sounds/enginedamage1.wav" );
    test( "sounds/enginedamage2.wav" );
    test( "sounds/enginedamage3.wav" );
    test( "sounds/enginedamage4.wav" );
    test( "sounds/exp1.wav" );
    test( "sounds/fairmileb/20mm.wav" );
    test( "sounds/fairmileb/20mmx2l.wav" );
    test( "sounds/fairmileb/20mmx2r.wav" );
    test( "sounds/fairmileb/engine.wav" );
    test( "sounds/fairmileb/idle.wav" );
    test( "sounds/fairmileb/xeng.wav" );
    test( "sounds/fairmileb/xgun1.wav" );
    test( "sounds/fairmileb/xgun2.wav" );
    test( "sounds/fairmileb/xgun3.wav" );
    test( "sounds/falldead.wav" );
    test( "sounds/fizz.wav" );
    test( "sounds/flak30/reload.wav" );
    test( "sounds/flak36/88mm.wav" );
    test( "sounds/flak36/88mmxgun.wav" );
    test( "sounds/flak36/88mmxgun_distant.wav" );
    test( "sounds/flak36/xgun.wav" );
    test( "sounds/flak36/xgun_distant.wav" );
    test( "sounds/flapdam.wav" );
    test( "sounds/flapwind.wav" );
    test( "sounds/fpshit1.wav" );
    test( "sounds/fpshit2.wav" );
    test( "sounds/fpshit3.wav" );
    test( "sounds/geardmg.wav" );
    test( "sounds/genericknife.wav" );
    test( "sounds/germmg.wav" );
    test( "sounds/h75/engine.wav" );
    test( "sounds/h75/idle.wav" );
    test( "sounds/h75/prop.wav" );
    test( "sounds/h75/xeng.wav" );
    test( "sounds/h81a2/engine.wav" );
    test( "sounds/h81a2/idle.wav" );
    test( "sounds/h81a2/prop.wav" );
    test( "sounds/h81a2/xeng.wav" );
    test( "sounds/hatch_close.wav" );
    test( "sounds/hatch_close2.wav" );
    test( "sounds/hatch_closea13.wav" );
    test( "sounds/hatch_closer35.wav" );
    test( "sounds/hatch_open.wav" );
    test( "sounds/hatch_open2.wav" );
    test( "sounds/hatch_openr35.wav" );
    test( "sounds/he111/engine.wav" );
    test( "sounds/he111/idle.wav" );
    test( "sounds/he111/prop.wav" );
    test( "sounds/he111/xeng.wav" );
    test( "sounds/hgear.wav" );
    test( "sounds/hgeardam.wav" );
    test( "sounds/hishot.wav" );
    test( "sounds/hishot1.wav" );
    test( "sounds/hishot2.wav" );
    test( "sounds/hishot3.wav" );
    test( "sounds/hit1.wav" );
    test( "sounds/hit2.wav" );
    test( "sounds/hit3.wav" );
    test( "sounds/hitconcrete1.wav" );
    test( "sounds/hitconcrete2.wav" );
    test( "sounds/hitconcrete3.wav" );
    test( "sounds/hitdirt1.wav" );
    test( "sounds/hitdirt2.wav" );
    test( "sounds/hitdirt3.wav" );
    test( "sounds/hitfabric1.wav" );
    test( "sounds/hitfabric2.wav" );
    test( "sounds/hitfabric3.wav" );
    test( "sounds/hitglass.wav" );
    test( "sounds/hitwater1.wav" );
    test( "sounds/hitwater2.wav" );
    test( "sounds/hitwood1.wav" );
    test( "sounds/hitwood2.wav" );
    test( "sounds/hitwood3.wav" );
    test( "sounds/hurri2/engine.wav" );
    test( "sounds/hurri2/idle.wav" );
    test( "sounds/igflag.wav" );
    test( "sounds/ju87b/37mm_l.wav" );
    test( "sounds/ju87b/37mm_r.wav" );
    test( "sounds/ju87b/bombrel.wav" );
    test( "sounds/ju87b/engine.wav" );
    test( "sounds/ju87b/idle.wav" );
    test( "sounds/ju87b/prop.wav" );
    test( "sounds/ju87b/tailgun.wav" );
    test( "sounds/ju87b/xeng.wav" );
    test( "sounds/ju87b/xgun1.wav" );
    test( "sounds/ju87b/xgun2.wav" );
    test( "sounds/ju87b/xgun3.wav" );
    test( "sounds/ju87b/xsiren1.wav" );
    test( "sounds/jumpdown.wav" );
    test( "sounds/k98bolt.wav" );
    test( "sounds/k98reload.wav" );
    test( "sounds/laffly/engine.wav" );
    test( "sounds/laffly/idle.wav" );
    test( "sounds/laffly/xeng.wav" );
    test( "sounds/lebelreload.wav" );
    test( "sounds/loshot1.wav" );
    test( "sounds/loshot2.wav" );
    test( "sounds/loshot3.wav" );
    test( "sounds/lrgcloseexp.wav" );
    test( "sounds/lrgfarexp.wav" );
    test( "sounds/lugerreload.wav" );
    test( "sounds/m10/3inchm7.wav" );
    test( "sounds/m10/xgun.wav" );
    test( "sounds/m1m1a3/57mmUSM1.wav" );
    test( "sounds/m4a2/75mm.wav" );
    test( "sounds/m4a2/engine.wav" );
    test( "sounds/m4a2/idle.wav" );
    test( "sounds/m4a2/mg.wav" );
    test( "sounds/m4a2/mg_trail.wav" );
    test( "sounds/m4a2/xeng.wav" );
    test( "sounds/m4a2/xgun.wav" );
    test( "sounds/m4a2/xgun2.wav" );
    test( "sounds/m4a2/xgun2_trail.wav" );
    test( "sounds/m4a2/xgun_distant.wav" );
    test( "sounds/macair39.wav" );
    test( "sounds/macair392.wav" );
    test( "sounds/macair393.wav" );
    test( "sounds/macair394.wav" );
    test( "sounds/macair395.wav" );
    test( "sounds/macair396.wav" );
    test( "sounds/manloader.wav" );
    test( "sounds/mas-reload.wav" );
    test( "sounds/masmodel38.wav" );
    test( "sounds/masmodel38_trail.wav" );
    test( "sounds/matilda2/coax.wav" );
    test( "sounds/matilda2/engine.wav" );
    test( "sounds/matilda2/idle.wav" );
    test( "sounds/matilda2/xeng.wav" );
    test( "sounds/matilda2/xgun2.wav" );
    test( "sounds/medcloseexp.wav" );
    test( "sounds/medfarexp.wav" );
    test( "sounds/mgreload.wav" );
    test( "sounds/mle25/mle25mm.wav" );
    test( "sounds/mle25/xgun.wav" );
    test( "sounds/mle47/47mm.wav" );
    test( "sounds/mle47/xgun.wav" );
    test( "sounds/morris/xeng.wav" );
    test( "sounds/mp40-reload.wav" );
    test( "sounds/oplblitz/idle.wav" );
    test( "sounds/oplblitz/starter.wav" );
    test( "sounds/oplblitz/xeng.wav" );
    test( "sounds/oplcanclose.wav" );
    test( "sounds/p38t/coax.wav" );
    test( "sounds/p38t/engine.wav" );
    test( "sounds/p38t/idle.wav" );
    test( "sounds/p38t/xeng.wav" );
    test( "sounds/p38t/xgun2.wav" );
    test( "sounds/p38t/xgun3.wav" );
    test( "sounds/p39canclose.wav" );
    test( "sounds/p39canopen.wav" );
    test( "sounds/painsound.wav" );
    test( "sounds/pak36/37mmpak36.wav" );
    test( "sounds/pak36/pak36.wav" );
    test( "sounds/pak36/xgun.wav" );
    test( "sounds/pak38/50mmPak38.wav" );
    test( "sounds/panhard/xeng.wav" );
    test( "sounds/pflaps.wav" );
    test( "sounds/pgear.wav" );
    test( "sounds/pgeardam.wav" );
    test( "sounds/pop.wav" );
    test( "sounds/push.wav" );
    test( "sounds/pzb39atr.wav" );
    test( "sounds/pzb39reload.wav" );
    test( "sounds/pziic/20mm.wav" );
    test( "sounds/pziic/20mm_trail.wav" );
    test( "sounds/pziic/engine.wav" );
    test( "sounds/pziic/idle.wav" );
    test( "sounds/pziic/xeng.wav" );
    test( "sounds/pziic/xgun1.wav" );
    test( "sounds/pziic/xgun2.wav" );
    test( "sounds/pziiif/dual_mg34.wav" );
    test( "sounds/pziiif/engine.wav" );
    test( "sounds/pziiif/idle.wav" );
    test( "sounds/pziiif/mg34_1.wav" );
    test( "sounds/pziiif/mg34_2.wav" );
    test( "sounds/pziiif/xeng.wav" );
    test( "sounds/pziiif/xgun2.wav" );
    test( "sounds/pziiif/xgun3.wav" );
    test( "sounds/pzivd/engine.wav" );
    test( "sounds/pzivd/idle.wav" );
    test( "sounds/pzivd/xeng.wav" );
    test( "sounds/pzivg/75mmkwk40.wav" );
    test( "sounds/pzivg/engine.wav" );
    test( "sounds/pzivg/idle.wav" );
    test( "sounds/pzivg/xeng.wav" );
    test( "sounds/pzivg/xgun.wav" );
    test( "sounds/pzivg/xgun_distant.wav" );
    test( "sounds/pzvie/88mml56tgun.wav" );
    test( "sounds/pzvie/engine.wav" );
    test( "sounds/pzvie/idle.wav" );
    test( "sounds/pzvie/starter.wav" );
    test( "sounds/pzvie/xeng.wav" );
    test( "sounds/qf2pounder/2lbs.wav" );
    test( "sounds/qf6pdr/57mm.wav" );
    test( "sounds/qf6pdr/57mm6pdr.wav" );
    test( "sounds/qf6pdr/57mmxgun.wav" );
    test( "sounds/r35/engine.wav" );
    test( "sounds/r35/idle.wav" );
    test( "sounds/r35/mle_m31.wav" );
    test( "sounds/r35/xeng.wav" );
    test( "sounds/r35/xgun2.wav" );
    test( "sounds/radioxmit.wav" );
    test( "sounds/rattle.wav" );
    test( "sounds/s35/engine.wav" );
    test( "sounds/s35/idle.wav" );
    test( "sounds/s35/xeng.wav" );
    test( "sounds/sdkfz232/engine.wav" );
    test( "sounds/sdkfz232/idle.wav" );
    test( "sounds/sdkfz232/xeng.wav" );
    test( "sounds/sdkfz7/engine.wav" );
    test( "sounds/sdkfz7/idle.wav" );
    test( "sounds/sdkfz7/xeng.wav" );
    test( "sounds/shraphit1.wav" );
    test( "sounds/shraphit2.wav" );
    test( "sounds/shraphit3.wav" );
    test( "sounds/slide_hatch.wav" );
    test( "sounds/smlcloseexp.wav" );
    test( "sounds/smlfarexp.wav" );
    test( "sounds/spit1/30cal_1.wav" );
    test( "sounds/spit1/30cal_2.wav" );
    test( "sounds/spit1/30cal_3.wav" );
    test( "sounds/spit1/30cal_4.wav" );
    test( "sounds/spit1/30cal_5.wav" );
    test( "sounds/spit1/30cal_6.wav" );
    test( "sounds/spit1/30cal_7.wav" );
    test( "sounds/spit1/30cal_8.wav" );
    test( "sounds/spit1/8x30cal.wav" );
    test( "sounds/spit1/engine.wav" );
    test( "sounds/spit1/idle.wav" );
    test( "sounds/spit1/prop.wav" );
    test( "sounds/spit1/xeng.wav" );
    test( "sounds/spit1/xgun.wav" );
    test( "sounds/squeek.wav" );
    test( "sounds/stabknife.wav" );
    test( "sounds/stall.wav" );
    test( "sounds/startercrank.wav" );
    test( "sounds/stuart/engine.wav" );
    test( "sounds/stuart/idle.wav" );
    test( "sounds/stuart/xeng.wav" );
    test( "sounds/stug3b/75mm.wav" );
    test( "sounds/stug3b/autoloader.wav" );
    test( "sounds/stug3g/75mmkwk40.wav" );
    test( "sounds/stug3g/engine.wav" );
    test( "sounds/stug3g/idle.wav" );
    test( "sounds/stug3g/xeng.wav" );
    test( "sounds/stug3g/xgun.wav" );
    test( "sounds/stug3g/xgun_distant.wav" );
    test( "sounds/tank_burn.wav" );
    test( "sounds/tex/avhit1.wav" );
    test( "sounds/tex/avhit2.wav" );
    test( "sounds/tex/avhit3.wav" );
    test( "sounds/tex/fpshit1.wav" );
    test( "sounds/tex/fpshit2.wav" );
    test( "sounds/tex/fpshit3.wav" );
    test( "sounds/tex/gvhit1.wav" );
    test( "sounds/tex/gvhit2.wav" );
    test( "sounds/tex/gvhit3.wav" );
    test( "sounds/tex/hground1.wav" );
    test( "sounds/tex/hground2.wav" );
    test( "sounds/tex/hground3.wav" );
    test( "sounds/thomp-reload.wav" );
    test( "sounds/torp_fire.wav" );
    test( "sounds/torp_loop.wav" );
    test( "sounds/track_ext.wav" );
    test( "sounds/track_int.wav" );
    test( "sounds/troop/aim-breath.wav" );
    test( "sounds/troop/bar_trail.wav" );
    test( "sounds/troop/boys55atr.wav" );
    test( "sounds/troop/bren.wav" );
    test( "sounds/troop/bren_trail.wav" );
    test( "sounds/troop/chuteopen.wav" );
    test( "sounds/troop/concrete/leftjog.wav" );
    test( "sounds/troop/concrete/leftrun.wav" );
    test( "sounds/troop/concrete/leftsprint.wav" );
    test( "sounds/troop/concrete/leftwalk.wav" );
    test( "sounds/troop/concrete/rightjog.wav" );
    test( "sounds/troop/concrete/rightrun.wav" );
    test( "sounds/troop/concrete/rightsprint.wav" );
    test( "sounds/troop/concrete/rightwalk.wav" );
    test( "sounds/troop/crawl.wav" );
    test( "sounds/troop/creep.wav" );
    test( "sounds/troop/creepaim.wav" );
    test( "sounds/troop/cupattach.wav" );
    test( "sounds/troop/cupremove.wav" );
    test( "sounds/troop/dirt/leftjog.wav" );
    test( "sounds/troop/dirt/leftsprint.wav" );
    test( "sounds/troop/dirt/leftwalk.wav" );
    test( "sounds/troop/dirt/rightjog.wav" );
    test( "sounds/troop/dirt/rightsprint.wav" );
    test( "sounds/troop/dirt/rightwalk.wav" );
    test( "sounds/troop/dirt_road/leftjog.wav" );
    test( "sounds/troop/dirt_road/leftsprint.wav" );
    test( "sounds/troop/dirt_road/leftwalk.wav" );
    test( "sounds/troop/dirt_road/rightjog.wav" );
    test( "sounds/troop/dirt_road/rightsprint.wav" );
    test( "sounds/troop/dirt_road/rightwalk.wav" );
    test( "sounds/troop/enfieldmk2no1.wav" );
    test( "sounds/troop/fall-hurt.wav" );
    test( "sounds/troop/fall-normal.wav" );
    test( "sounds/troop/fallw-hurt.wav" );
    test( "sounds/troop/fallw.wav" );
    test( "sounds/troop/gear_loop/jog.wav" );
    test( "sounds/troop/gear_loop/sprint.wav" );
    test( "sounds/troop/gear_loop/walk.wav" );
    test( "sounds/troop/grenade-throw.wav" );
    test( "sounds/troop/grenade.wav" );
    test( "sounds/troop/k98.wav" );
    test( "sounds/troop/landingground.wav" );
    test( "sounds/troop/landingwater.wav" );
    test( "sounds/troop/lebel.wav" );
    test( "sounds/troop/LMG_ATR_dryfire.wav" );
    test( "sounds/troop/luger.wav" );
    test( "sounds/troop/m1935s.wav" );
    test( "sounds/troop/mas.wav" );
    test( "sounds/troop/mas_trail.wav" );
    test( "sounds/troop/masmodel38.wav" );
    test( "sounds/troop/masmodel38_trail.wav" );
    test( "sounds/troop/masrgreload.wav" );
    test( "sounds/troop/metal/leftjog.wav" );
    test( "sounds/troop/metal/leftsprint.wav" );
    test( "sounds/troop/metal/leftwalk.wav" );
    test( "sounds/troop/metal/rightjog.wav" );
    test( "sounds/troop/metal/rightsprint.wav" );
    test( "sounds/troop/metal/rightwalk.wav" );
    test( "sounds/troop/mg34.wav" );
    test( "sounds/troop/mg34_trail.wav" );
    test( "sounds/troop/mg42_trail.wav" );
    test( "sounds/troop/millscupattach.wav" );
    test( "sounds/troop/millscupremove.wav" );
    test( "sounds/troop/millsrgreload.wav" );
    test( "sounds/troop/mk4no1.wav" );
    test( "sounds/troop/mp40.wav" );
    test( "sounds/troop/mp40_trail.wav" );
    test( "sounds/troop/parachutewind.wav" );
    test( "sounds/troop/Pistol_Dryfire.wav" );
    test( "sounds/troop/pzb39atr.wav" );
    test( "sounds/troop/rgreload.wav" );
    test( "sounds/troop/Rifle_Dryfire.wav" );
    test( "sounds/troop/sand/leftjog.wav" );
    test( "sounds/troop/sand/leftrun.wav" );
    test( "sounds/troop/sand/leftsprint.wav" );
    test( "sounds/troop/sand/leftwalk.wav" );
    test( "sounds/troop/sand/rightjog.wav" );
    test( "sounds/troop/sand/rightrun.wav" );
    test( "sounds/troop/sand/rightsprint.wav" );
    test( "sounds/troop/sand/rightwalk.wav" );
    test( "sounds/troop/satchel-place.wav" );
    test( "sounds/troop/satexp.wav" );
    test( "sounds/troop/scream.wav" );
    test( "sounds/troop/scream2.wav" );
    test( "sounds/troop/sprint.wav" );
    test( "sounds/troop/sprinttps.wav" );
    test( "sounds/troop/thompson1928.wav" );
    test( "sounds/troop/thompson1928_trail.wav" );
    test( "sounds/troop/tps_death/tps_death_1.wav" );
    test( "sounds/troop/tps_death/tps_death_10.wav" );
    test( "sounds/troop/tps_death/tps_death_2.wav" );
    test( "sounds/troop/tps_death/tps_death_3.wav" );
    test( "sounds/troop/tps_death/tps_death_4.wav" );
    test( "sounds/troop/tps_death/tps_death_5.wav" );
    test( "sounds/troop/tps_death/tps_death_6.wav" );
    test( "sounds/troop/tps_death/tps_death_7.wav" );
    test( "sounds/troop/tps_death/tps_death_8.wav" );
    test( "sounds/troop/tps_death/tps_death_9.wav" );
    test( "sounds/troop/trot.wav" );
    test( "sounds/troop/trotaim.wav" );
    test( "sounds/troop/trottps.wav" );
    test( "sounds/troop/vbcupattach.wav" );
    test( "sounds/troop/vbcupremove.wav" );
    test( "sounds/troop/waterwalk.wav" );
    test( "sounds/troop/wood/leftjog.wav" );
    test( "sounds/troop/wood/leftrun.wav" );
    test( "sounds/troop/wood/leftsprint.wav" );
    test( "sounds/troop/wood/leftwalk.wav" );
    test( "sounds/troop/wood/rightjog.wav" );
    test( "sounds/troop/wood/rightrun.wav" );
    test( "sounds/troop/wood/rightsprint.wav" );
    test( "sounds/troop/wood/rightwalk.wav" );
    test( "sounds/turret/electric_turret.wav" );
    test( "sounds/turret/electric_turret_trail.wav" );
    test( "sounds/turret/hand_turret_move.wav" );
    test( "sounds/turret/hydrolic_turret.wav" );
    test( "sounds/turret/hydrolic_turret_trail.wav" );
    test( "sounds/turret/turret_traverse_general.wav" );
    test( "sounds/turret_traverse_general.wav" );
    test( "sounds/w154/idle.wav" );
    test( "sounds/w154/starter.wav" );
    test( "sounds/w154/xeng.wav" );
    test( "sounds/waterwalk.wav" );
    test( "sounds/wheelclose.wav" );
    test( "sounds/wheelload.wav" );
    test( "sounds/wheelopen.wav" );
    test( "sounds/whine.wav" );
    test( "sounds/wind.wav" );
    test( "sounds/z34/engine.wav" );
    test( "sounds/z34/idle.wav" );
    test( "sounds/z34/xeng.wav" );
    test( "ui/sounds/allied_loop.wav" );
    test( "ui/sounds/allied_main.wav" );
    test( "ui/sounds/ambiance.wav" );
    test( "ui/sounds/axis_loop.wav" );
    test( "ui/sounds/axis_main.wav" );
    test( "ui/sounds/btnup.wav" );
    test( "ui/sounds/click.wav" );
    test( "ui/sounds/error.wav" );
    test( "ui/sounds/flag.wav" );
    test( "ui/sounds/persona.wav" );
    test( "ui/sounds/select.wav" );
    test( "ui/sounds/tab.wav" );
    test( "ui/sounds/zoominloop.wav" );
}

void test(const char* name)
{
    SNDSCREATE sndsCreate;

    sndsCreate.pszName = (char *)name;
    sndsCreate.bIs3D = false;

    snSound* snd = sndsLoadSound( &sndsCreate );
    (void)snd ;

    SHOW_DEBUG( if ( snd==NULL) {fprintf( mDebugStream, "snd is null for %s\n", name ); fflush( mDebugStream );})
}
