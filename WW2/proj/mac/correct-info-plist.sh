#!/bin/bash

die() {
    echo $1
    exit 1
}

[ -n "${BUILT_PRODUCTS_DIR}" ] || die "Must be run from Xcode"

shaHash=$( xcrun git -c color.ui=never rev-parse HEAD )
[ $? -eq 0 ] || die "git rev-parse failed"

state=$( xcrun git -c color.ui=never status --porcelain -uno | wc -l )
[ $? -eq 0 ] || die "git status failed"
[ $state -eq 0 ] && status=clean || status=dirty

version=$( echo PN_WW2_{PRODUCT,VERSION,PATCH,BUILD}_NO. | xcrun clang -E -P -include "${SRCROOT}/proj/versionNo.h" - | sed -e 's/\.$//' -e 's/ //g' )

echo "Commit hash: $shaHash"
echo "Repo state: $status"
echo "Client version: $version"

INFO="${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/Contents/Info"

defaults write "$INFO" GitShaHash "$shaHash"
defaults write "$INFO" GitState "$status"
defaults write "$INFO" CFBundleShortVersionString "$version"
defaults write "$INFO" CFBundleVersion "$version"

# For some reason, defaults now forces binary plist format. Not good for an Info.plist
plutil -convert xml1 "$INFO.plist"

