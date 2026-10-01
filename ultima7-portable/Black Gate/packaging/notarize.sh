#!/bin/bash
# Notarizes a signed app or disk image with Apple and staples the ticket to it.
#
#   notarize.sh <Ultima7.app | Ultima7.dmg>
#
# Needs APPLE_API_KEY (base64 of the .p8), APPLE_API_KEY_ID and APPLE_API_ISSUER_ID.
set -euo pipefail

target=$1
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

echo "$APPLE_API_KEY" | base64 --decode > "$work/AuthKey.p8"
auth=(--key "$work/AuthKey.p8" --key-id "$APPLE_API_KEY_ID" --issuer "$APPLE_API_ISSUER_ID")

# An app goes up zipped; a disk image goes up as it is.
upload=$target
if [ -d "$target" ]; then
	upload="$work/upload.zip"
	ditto -c -k --keepParent "$target" "$upload"
fi

result=$(xcrun notarytool submit "$upload" "${auth[@]}" --wait --output-format json)
echo "$result"
if [ "$(plutil -extract status raw - <<< "$result")" != "Accepted" ]; then
	xcrun notarytool log "$(plutil -extract id raw - <<< "$result")" "${auth[@]}"
	exit 1
fi
xcrun stapler staple "$target"
