set -euo pipefail

rsvg-convert -w 32 -h 32 $1 | convert - -depth 8 rgba:-
