#pragma once
// Date: Fri Jan 25 18:11:04 CST 2008
//
// Encapsulation and state engine for capture actions.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class CPStruct;
class FacilityStruct;
class ObjectStruct;

//tolua_begin

struct CaptureAction
{
	CaptureAction(class FacilityStruct& withFacility, WWII_COUNTRY withNewCountry, bool withQuietly);
	void Execute( bool manualCapture = false);

	FacilityStruct& m_facility;
	CPStruct&		m_cp;
	WWII_COUNTRY	m_newCountry;
	WWII_SIDE		m_newSide;
	WWII_COUNTRY	m_oldCountry;
	WWII_SIDE		m_oldSide;
	bool			m_quietly;

	bool			m_enteredContention;
	bool			m_leftContention;
	bool			m_setControl;
	bool			m_setOwner;

	bool			m_reclamation;

//tolua_end
private:
	void Announce() const;
	void ApplyEffects(bool manualCapture = false) const;
	void ChangeOwner();
	void EnterContention() const;
	void LeaveContention() const;
	void SetControl() const;
//tolua_begin
};

//tolua_end

