#!/usr/bin/env bash
# 安装图标 + 桌面/应用菜单入口，便于鼠标双击启动 HotSend Gui
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/Build/meson/Code/hotsend-gui"
ICON_SRC="$ROOT/Assets/icons/hicolor"
APP_ID="hotsend"
DESKTOP_NAME="hotsend.desktop"

if [[ ! -x "$BIN" ]]; then
  echo "missing binary: $BIN" >&2
  echo "run: cd \"$ROOT\" && meson setup Build/meson && meson compile -C Build/meson" >&2
  exit 1
fi

mkdir -p "$HOME/.local/share/icons/hicolor/48x48/apps" \
  "$HOME/.local/share/icons/hicolor/128x128/apps" \
  "$HOME/.local/share/icons/hicolor/256x256/apps" \
  "$HOME/.local/share/applications"

cp -f "$ICON_SRC/48x48/apps/hotsend.png" "$HOME/.local/share/icons/hicolor/48x48/apps/${APP_ID}.png"
cp -f "$ICON_SRC/128x128/apps/hotsend.png" "$HOME/.local/share/icons/hicolor/128x128/apps/${APP_ID}.png"
cp -f "$ICON_SRC/256x256/apps/hotsend.png" "$HOME/.local/share/icons/hicolor/256x256/apps/${APP_ID}.png"
cp -f "$ICON_SRC/128x128/apps/hotsend.png" \
  "$HOME/.local/share/icons/hicolor/128x128/apps/com.blocks.hotsend.png"

cat >"$HOME/.local/share/applications/${DESKTOP_NAME}" <<EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=HotSend
Name[zh_CN]=HotSend 热传
Comment=Incremental board file transfer (TCP / serial)
Comment[zh_CN]=板端差量热传（TCP / 串口）
Exec=${BIN}
Path=${ROOT}
Icon=${APP_ID}
Terminal=false
Categories=Development;System;
StartupNotify=true
StartupWMClass=com.blocks.hotsend
EOF
chmod 644 "$HOME/.local/share/applications/${DESKTOP_NAME}"

DESKTOP_DIR="${XDG_DESKTOP_DIR:-}"
if [[ -z "$DESKTOP_DIR" ]] && command -v xdg-user-dir >/dev/null 2>&1; then
  DESKTOP_DIR="$(xdg-user-dir DESKTOP)"
fi
if [[ -z "$DESKTOP_DIR" || ! -d "$DESKTOP_DIR" ]]; then
  if [[ -d "$HOME/桌面" ]]; then
    DESKTOP_DIR="$HOME/桌面"
  elif [[ -d "$HOME/Desktop" ]]; then
    DESKTOP_DIR="$HOME/Desktop"
  fi
fi
if [[ -n "${DESKTOP_DIR:-}" && -d "$DESKTOP_DIR" ]]; then
  cp -f "$HOME/.local/share/applications/${DESKTOP_NAME}" "$DESKTOP_DIR/${DESKTOP_NAME}"
  chmod +x "$DESKTOP_DIR/${DESKTOP_NAME}"
  # Ubuntu/GNOME：标为已信任，否则双击只提示「取消/启动」
  if command -v gio >/dev/null 2>&1; then
    gio set "$DESKTOP_DIR/${DESKTOP_NAME}" metadata::trusted true 2>/dev/null || true
  fi
  echo "desktop shortcut: $DESKTOP_DIR/${DESKTOP_NAME}"
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi

echo "menu entry: $HOME/.local/share/applications/${DESKTOP_NAME}"
echo "done. Open app menu → HotSend, or double-click Desktop/${DESKTOP_NAME}"
