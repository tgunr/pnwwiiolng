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
		<joybutton stick="1" index="1">6</joybutton>
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
	<control function="Throttle">
		<joyaxis stick="1">slider1</joyaxis>
		<keydelta value="0.00" per="sec" index="2">
			<key></key>
		</keydelta>
		<keydelta value="0.00" per="sec" index="3">
			<key></key>
		</keydelta>
	</control>
	<control function="Elevator trim">
		<keydelta value="1.00" per="keypress">
			<key>k</key>
		</keydelta>
		<keydelta value="-1.00" per="keypress" index="1">
			<key>i</key>
		</keydelta>
	</control>
</controlset>
