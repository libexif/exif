# Security overview

## General

exif is a commandline tool to process EXIF datablobs, which are usually
embedded in JPEG files.

It allows reading, writing, changing, and extraction (binary and textual versions)
of this data.

Note if a vulnerability is found in the libexif library in use, please
refer to the libexif security handling and its SECURITY.md.

## Attack Surface

Any file put into the tool should be assumed untrusted and
potentially malicious.

The primary attack scenario is processing of files for EXIF content
extraction (displaying) via unattended services, up to and including
webservices where files can be uploaded by potential attackers.


## Bugs considered security issues

(Mostly for CVE assigments rules.)

Triggering memory corruption of any form is considered in scope.
Triggering endless loops is considered in scope. (would block services)

Crashes during writing out of data could be in scope.

## Bugs not considered security issues

Crashes caused by debugging functionality are not in scope.

Safe aborts (e.g. NULL ptr crashes) are annoying, but only considered bugs.
(The exif program would terminates anyway, so there is no additional denial of service impact.)

## Bugreports

Bugreports can be filed as github issues.

If you want to report an embargoed security bug report, reach out to dan@coneharvesters.com and marcus@jet.franken.de.
