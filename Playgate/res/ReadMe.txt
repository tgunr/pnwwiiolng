Playgate V1.2.0 alpha build 080326-001
======================================

It's coming round to that time of the century when we may have to build an update of playgate. In order to sweeten the deal of asking you to "please run this version of playgate that may crash and or not work because of compiler/language changes since we last built it 3 years ago", I'm going to offer you a chance to try out my "variation on a theme" version of playgate.

What I'm looking for:

- Does it run,
- Does it launch the game and connect to the server you ask it to?

What I'm not looking for:

- Commentary on the art treatment - it is coder art with decoration,
- Commentary on the new features,
- Feedback on new features,
- Suggestions for new features,

That doesn't mean you can't do the above, but this is not a release candidate, and the GUI/feature changes are not in the playgate baseline. Everything that's important in this test hasn't changed.

What we absolutely don't want you commenting on:

- The old playgate window,
- Will there be a Mac version? (The Mac version is already better than the PC version),
- Integrating the launcher into the game client

Known Issues:

- Several new error conditions were added which do not yet have corresponding website content - e.g. if you enter the wrong username/password it will take you to a page that says "?error.userpass?" or something,
- Always requires server selection (command line functionality is disabled)
- Crashes in weird ways if you run two copies at once (as does normal Playgate)
- Crashes in weird ways when it can't install a beta patch (as does normal Playgate)
- Still doesn't support https:// neccessary to download beta patches,

Installation:

1. Locate your Playgate folder (typically c:\program files\playnet\playgate)
2. Back up your existing PresenterII executable
3. Download the new playgate zip
4. Run the "setup.exe" file included in the zip archive.

Usage:

1. Try using the Desktop Shortcut
2. Try using the Start Menu > Program Files > CRS > Playgate BETA Shortcut
3. Try using it via your normal web launch (you will still have to select a server in playgate)