<?xml version='1.0'?>
<!DOCTYPE controlset SYSTEM "cfml.dtd">
<controlset version="1.1.0" language="english" keyboard="us">
	<control function="Gear handle">
		<joybutton stick="1" index="1">4</joybutton>
	</control>
	<control function="Toggle wep">
		<joybutton stick="1" index="1">2</joybutton>
	</control>
	<control function="Autopilot">
		<key>a</key>
	</control>
	<control function="Instrument view">
		<key index="1"></key>
	</control>
	<control function="Gunsight view">
		<key index="1"></key>
	</control>
	<control function="Use secondary weapon">
		<joybutton stick="1">3</joybutton>
		<key index="1">b</key>
	</control>
	<control function="Jettison Ordnance">
		<joybutton stick="1">5</joybutton>
		<key index="1"></key>
	</control>
	<control function="Yaw">
		<keyabsolute value="0.00">
			<key></key>
		</keyabsolute>
		<keyabsolute value="0.00" index="10">
			<key></key>
		</keyabsolute>
	</control>
	<control function="Throttle">
		<joyaxis stick="1">slider1</joyaxis>
		<keydelta value="120.00" per="sec" index="2">
			<key>,</key>
		</keydelta>
		<keydelta value="-100.00" per="sec" index="3">
			<key>.</key>
		</keydelta>
	</control>
	<control function="Flap control">
		<keydelta value="24.00" per="keypress">
			<joybutton stick="1">6</joybutton>
		</keydelta>
		<keyabsolute value="0.00">
			<joybutton stick="1">5</joybutton>
		</keyabsolute>
	</control>
	<control function="Elevator trim">
		<keydelta value="1.00" per="keypress">
			<key>k</key>
		</keydelta>
		<keydelta value="-1.00" per="keypress" index="1">
			<key>i</key>
		</keydelta>
	</control>
	<control function="Aileron trim">
		<joyaxis stick="0"></joyaxis>
	</control>
	<control function="Rudder trim">
		<joyaxis stick="0"></joyaxis>
	</control>
</controlset>
