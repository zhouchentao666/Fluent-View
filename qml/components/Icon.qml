import QtQuick

// Lightweight glyph icon. Uses geometric Unicode so we ship no image assets
// (keeps the install small). Swap for SVG / icon-font later if desired.
Text {
    id: glyph
    property string name
    property int size: 18
    // `color` is inherited from Text; assign a theme-aware default here.
    color: ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F"

    text: ({
        "back": "‹", "next": "›",
        "rotateL": "⟲", "rotateR": "⟳",
        "flipH": "⇄", "flipV": "⇅",
        "fit": "⤢", "actual": "⤡",
        "fullscreen": "⛶", "info": "ⓘ",
        "settings": "⚙", "close": "✕",
        "min": "—", "max": "▢", "restore": "❐",
        "menu": "☰", "open": "🗁",
        "copy": "⧉", "wallpaper": "🖼",
        "folder": "📂", "theme": "🌓",
        "prev": "◀", "next2": "▶"
    })[name] || ""

    font.pixelSize: size
    font.family: "Segoe UI Symbol, Segoe MDL2 Assets, Apple Symbols, sans-serif"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
