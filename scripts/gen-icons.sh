#!/usr/bin/env bash
# Generates the Fluent-style monochrome icon set (dark + light theme variants)
# into resources/icons/{dark,light}. Run from the repo root: bash scripts/gen-icons.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DARK="$ROOT/resources/icons/dark"    # light strokes, for dark theme
LIGHT="$ROOT/resources/icons/light"  # dark strokes, for light theme
mkdir -p "$DARK" "$LIGHT"

svg() { # name stroke
  local name="$1" stroke="$2" body="$3"
  cat > "$DARK/$name.svg" <<EOF
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
  <g fill="none" stroke="$stroke" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
$body
  </g>
</svg>
EOF
  sed "s/$stroke/${LIGHT_STROKE:-#3B3B3B}/g" "$DARK/$name.svg" > "$LIGHT/$name.svg"
}

S="#E8E8E8"

svg zoom-in "$S" '<circle cx="10.5" cy="10.5" r="6.5"/><path d="M15.5 15.5 20 20"/><path d="M10.5 7.8v5.4M7.8 10.5h5.4"/>'
svg zoom-out "$S" '<circle cx="10.5" cy="10.5" r="6.5"/><path d="M15.5 15.5 20 20"/><path d="M7.8 10.5h5.4"/>'
svg fit "$S" '<path d="M4 9V5a1 1 0 0 1 1-1h4M15 4h4a1 1 0 0 1 1 1v4M20 15v4a1 1 0 0 1-1 1h-4M9 20H5a1 1 0 0 1-1-1v-4"/><rect x="8" y="8" width="8" height="8" rx="1"/>'
svg actual-size "$S" '<rect x="3.5" y="5.5" width="17" height="13" rx="1.5"/><path d="M8 15v-6M8 9h2.6a1.4 1.4 0 0 1 0 2.8H8M15.5 15v-6M15.5 9h.01M15.5 12.2h.01"/>'
svg rotate-left "$S" '<path d="M8.5 4.5 4 8.5l4.5 4"/><path d="M4 8.5h9a6.5 6.5 0 1 1 0 13H7"/>'
svg rotate-right "$S" '<path d="M15.5 4.5 20 8.5l-4.5 4"/><path d="M20 8.5h-9a6.5 6.5 0 1 0 0 13h6"/>'
svg flip-h "$S" '<path d="M12 3v18" stroke-dasharray="2.5 2.5"/><path d="M9 6 3 12l6 6V6z" fill="none"/><path d="M15 6l6 6-6 6V6z"/>'
svg flip-v "$S" '<path d="M3 12h18" stroke-dasharray="2.5 2.5"/><path d="M6 9 12 3l6 6H6z"/><path d="M6 15h12l-6 6-6-6z"/>'
svg slideshow "$S" '<rect x="3.5" y="5.5" width="17" height="13" rx="1.5"/><path d="M10 9.5v5l4.5-2.5L10 9.5z"/>'
svg info "$S" '<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5M12 8h.01"/>'
svg menu "$S" '<path d="M4 7h16M4 12h16M4 17h16"/>'
svg thumbnails "$S" '<rect x="3.5" y="3.5" width="7" height="7" rx="1"/><rect x="13.5" y="3.5" width="7" height="7" rx="1"/><rect x="3.5" y="13.5" width="7" height="7" rx="1"/><rect x="13.5" y="13.5" width="7" height="7" rx="1"/>'
svg minimize "$S" '<path d="M5 12h14"/>'
svg maximize "$S" '<rect x="5" y="5" width="14" height="14" rx="1"/>'
svg close "$S" '<path d="M6 6l12 12M18 6 6 18"/>'

# App icon: rounded-square gradient with a picture glyph.
cat > "$ROOT/resources/icons/fluentview.svg" <<'EOF'
<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#4CC2FF"/>
      <stop offset="1" stop-color="#005FB8"/>
    </linearGradient>
  </defs>
  <rect x="2" y="2" width="60" height="60" rx="14" fill="url(#bg)"/>
  <rect x="14" y="16" width="36" height="28" rx="4" fill="#FFFFFF" opacity="0.92"/>
  <circle cx="24" cy="25" r="4" fill="#FFB900"/>
  <path d="M16 40l10-11 7 8 5-5 10 8z" fill="#0067C0"/>
</svg>
EOF

echo "Generated icons in $DARK and $LIGHT"
