# ppzsum

Program to convert between various hex formats used by EPROM programmers
and related embedded-systems software tools.
As a side-effect, it also generates a checksum which will match that
reported by a Stag PPZ or PP39 EPROM programmer.

The Stag PPZ EPROM programmer was a 6809-based system with built-in
CRT display and pluggable programming modules.
The PP39 was a smaller unit with a 14-segment VFD.

## Formats supported

The default input format to 'ppzsum' is binary.
This is a popular format for ROM images on the Internet,
but is rarely used by EPROM programmers because
it is difficult to transfer reliably over a serial (RS-232)
connection.

EPROM programmers generally use either Intel hex format, Motorola S-Records,
or MOS Technology hex format.
'ppzsum' can generate all of these.

## The Checksum

The checksum is simply the 16-bit sum of all the bytes contained in the
hex file (after decoding), expressed in hex.
It is a very weak checksum, suitable only for detection of transmission
or programming errors.
It is not in any way secure against malicious alteration.

## The Stag PPZ

<a data-flickr-embed="true"  href="https://www.flickr.com/photos/anachrocomputer/2185799066/in/album-72157603795019803/" title="Stag PPZ EPROM programmer"><img src="https://c1.staticflickr.com/3/2129/2185799066_d961d75a5c.jpg" width="500" height="333" alt="Stag PPZ EPROM programmer"></a><script async src="//embedr.flickr.com/assets/client-code.js" charset="utf-8"></script>

## The Stag PP39

<a data-flickr-embed="true"  href="https://www.flickr.com/photos/anachrocomputer/9262710311/in/photolist-fodtPk-fodtXH-fosKwC-fosKpC-fosKfQ-fosJWJ-fodtdn-fosysh-fosyp1-fezFQd-fezFL7-fekpaM-f7vP4V" title="Stag PP39 EPROM Programmer"><img src="https://c1.staticflickr.com/8/7383/9262710311_45208a03dc.jpg" width="500" height="375" alt="Stag PP39 EPROM Programmer"></a><script async src="//embedr.flickr.com/assets/client-code.js" charset="utf-8"></script>
