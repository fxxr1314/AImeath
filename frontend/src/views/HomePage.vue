<template>
  <div class="desktop" @contextmenu.prevent>
    <div class="desktopCont">
      <div
        v-for="app in apps"
        :key="app.name"
        class="dskApp"
        tabindex="0"
        @click="openApp(app)"
      >
        <div class="dskIcon" v-html="app.icon"></div>
        <div class="appName">{{ app.name }}</div>
      </div>
    </div>

    <div class="windows-layer">
      <div
        v-for="(win, id) in windows"
        :key="id"
        class="win-window"
        :class="{ 'win-minimized': win.minimized, 'win-maximized': win.maximized }"
        :style="winStyle(win)"
        @mousedown="focusWindow(id)"
      >
        <div
          class="win-titlebar"
          @mousedown="startDrag(id, $event)"
          @dblclick="toggleMaximize(id)"
        >
          <span class="win-icon" v-html="win.icon"></span>
          <span class="win-title-text">{{ win.name }}</span>
          <div class="win-controls">
            <button class="ctrl-btn ctrl-min" @mousedown.stop @click="minimizeWindow(id)">─</button>
            <button class="ctrl-btn ctrl-max" @mousedown.stop @click="toggleMaximize(id)">☐</button>
            <button class="ctrl-btn ctrl-close" @mousedown.stop @click="closeWindow(id)">✕</button>
          </div>
        </div>
        <div class="win-body">
          <iframe :src="win.src" frameborder="0"></iframe>
        </div>
        <template v-if="!win.maximized">
          <div class="rh rh-n" @mousedown.stop="startResize(id, $event, 'n')"></div>
          <div class="rh rh-s" @mousedown.stop="startResize(id, $event, 's')"></div>
          <div class="rh rh-e" @mousedown.stop="startResize(id, $event, 'e')"></div>
          <div class="rh rh-w" @mousedown.stop="startResize(id, $event, 'w')"></div>
          <div class="rh rh-ne" @mousedown.stop="startResize(id, $event, 'ne')"></div>
          <div class="rh rh-nw" @mousedown.stop="startResize(id, $event, 'nw')"></div>
          <div class="rh rh-se" @mousedown.stop="startResize(id, $event, 'se')"></div>
          <div class="rh rh-sw" @mousedown.stop="startResize(id, $event, 'sw')"></div>
        </template>
      </div>
    </div>

    <div class="taskbar">
      <div class="taskcont">
        <div class="tasksCont">
          <div class="tsbar">
            <div class="tsIcon startBtn" @click="goHome">
              <svg viewBox="0 0 24 24" width="20" height="20">
                <rect x="2" y="2" width="9" height="9" rx="1.5" fill="currentColor" opacity="0.9"/>
                <rect x="13" y="2" width="9" height="9" rx="1.5" fill="currentColor" opacity="0.9"/>
                <rect x="2" y="13" width="9" height="9" rx="1.5" fill="currentColor" opacity="0.9"/>
                <rect x="13" y="13" width="9" height="9" rx="1.5" fill="currentColor" opacity="0.7"/>
              </svg>
            </div>
            <div
              v-for="(win, id) in windows"
              :key="id"
              class="tsIcon winBtn"
              :class="{ active: win.zIndex === topZ }"
              @click="toggleWindow(id)"
            >
              <span v-html="win.icon" class="tb-icon"></span>
            </div>
          </div>
        </div>
        <div class="taskright">
          <div class="taskDate">
            <div>{{ timeStr }}</div>
            <div>{{ dateStr }}</div>
          </div>
          <div class="graybd"></div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()

const apps = [
  {
    name: '贪食蛇', url: '/snake',
    icon: `<svg viewBox="0 0 48 48" width="36" height="36">
      <rect x="4" y="4" width="40" height="40" rx="4" fill="#1a1a2e" opacity="0.8"/>
      <circle cx="16" cy="16" r="4" fill="#4ade80"/><circle cx="24" cy="16" r="4" fill="#4ade80"/>
      <circle cx="32" cy="16" r="4" fill="#4ade80"/><circle cx="32" cy="24" r="4" fill="#4ade80"/>
      <circle cx="32" cy="32" r="4" fill="#4ade80"/><circle cx="24" cy="32" r="4" fill="#4ade80"/>
      <circle cx="16" cy="32" r="4" fill="#22c55e"/><circle cx="16" cy="24" r="4" fill="#22c55e"/>
      <rect x="13" y="21" width="6" height="6" rx="1" fill="#fff"/>
      <rect x="29" y="13" width="6" height="6" rx="1" fill="#fff"/>
    </svg>`,
  },
  {
    name: '吃豆豆', url: '/pacman',
    icon: `<svg viewBox="0 0 48 48" width="36" height="36">
      <rect x="4" y="4" width="40" height="40" rx="4" fill="#1a1a2e" opacity="0.8"/>
      <circle cx="22" cy="24" r="12" fill="#facc15"/>
      <polygon points="22,24 34,12 34,36" fill="#1a1a2e"/>
      <circle cx="34" cy="14" r="2" fill="#f87171"/><circle cx="34" cy="34" r="2" fill="#f87171"/>
      <circle cx="10" cy="18" r="2" fill="#fbbf24"/><circle cx="10" cy="30" r="2" fill="#fbbf24"/>
    </svg>`,
  },
  {
    name: '五子棋', url: '/gomoku',
    icon: `<svg viewBox="0 0 48 48" width="36" height="36">
      <rect x="4" y="4" width="40" height="40" rx="4" fill="#1a1a2e" opacity="0.8"/>
      <rect x="8" y="8" width="32" height="32" rx="2" fill="#d97706"/>
      <line x1="12" y1="16" x2="36" y2="16" stroke="#92400e" stroke-width="0.5"/>
      <line x1="12" y1="24" x2="36" y2="24" stroke="#92400e" stroke-width="0.5"/>
      <line x1="12" y1="32" x2="36" y2="32" stroke="#92400e" stroke-width="0.5"/>
      <line x1="16" y1="12" x2="16" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <line x1="24" y1="12" x2="24" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <line x1="32" y1="12" x2="32" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <circle cx="20" cy="20" r="3" fill="#1f2937"/><circle cx="28" cy="28" r="3" fill="#fafafa"/>
    </svg>`,
  },
  {
    name: '围棋', url: '/go',
    icon: `<svg viewBox="0 0 48 48" width="36" height="36">
      <rect x="4" y="4" width="40" height="40" rx="4" fill="#1a1a2e" opacity="0.8"/>
      <rect x="8" y="8" width="32" height="32" rx="2" fill="#d97706"/>
      <line x1="12" y1="16" x2="36" y2="16" stroke="#92400e" stroke-width="0.5"/>
      <line x1="12" y1="24" x2="36" y2="24" stroke="#92400e" stroke-width="0.5"/>
      <line x1="12" y1="32" x2="36" y2="32" stroke="#92400e" stroke-width="0.5"/>
      <line x1="16" y1="12" x2="16" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <line x1="24" y1="12" x2="24" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <line x1="32" y1="12" x2="32" y2="36" stroke="#92400e" stroke-width="0.5"/>
      <circle cx="16" cy="16" r="3" fill="#1f2937"/><circle cx="24" cy="16" r="3" fill="#fafafa"/>
      <circle cx="24" cy="24" r="3" fill="#1f2937"/><circle cx="32" cy="24" r="3" fill="#fafafa"/>
      <circle cx="16" cy="32" r="3" fill="#fafafa"/>
    </svg>`,
  },
  {
    name: 'Chat 喵', url: '/chat',
    icon: `<svg viewBox="0 0 48 48" width="36" height="36">
      <rect x="4" y="4" width="40" height="40" rx="4" fill="#1a1a2e" opacity="0.8"/>
      <path d="M10 18a8 8 0 0 1 8-8h12a8 8 0 0 1 8 8v6a8 8 0 0 1-8 8h-4l-6 5v-5h-2a8 8 0 0 1-8-8v-6z" fill="#60a5fa"/>
      <circle cx="20" cy="22" r="2" fill="#fff"/><circle cx="28" cy="22" r="2" fill="#fff"/>
      <path d="M20 28a4 4 0 0 0 8 0" stroke="#fff" stroke-width="1.5" fill="none" stroke-linecap="round"/>
    </svg>`,
  },
]

const windows = reactive({})
let winIdSeq = 1
let zSeq = 1

const topZ = computed(() => {
  let max = 0
  for (const id in windows) {
    if (windows[id].zIndex > max) max = windows[id].zIndex
  }
  return max
})

function iframeSrc(url) {
  const base = window.location.origin + window.location.pathname.replace(/\/?$/, '')
  return `${base}/#${url}`
}

function openApp(app) {
  const count = Object.keys(windows).length
  const cascade = (count * 30) % 240
  const id = `w${winIdSeq++}`
  windows[id] = {
    name: app.name,
    icon: app.icon,
    url: app.url,
    src: iframeSrc(app.url),
    x: 40 + cascade,
    y: 40 + cascade,
    w: 820,
    h: 540,
    zIndex: ++zSeq,
    minimized: false,
    maximized: false,
  }
}

function closeWindow(id) {
  delete windows[id]
}

function minimizeWindow(id) {
  windows[id].minimized = !windows[id].minimized
}

function toggleMaximize(id) {
  const w = windows[id]
  if (w.maximized) {
    w.x = w._x
    w.y = w._y
    w.w = w._w
    w.h = w._h
    w.maximized = false
  } else {
    w._x = w.x
    w._y = w.y
    w._w = w.w
    w._h = w.h
    w.x = 0
    w.y = 0
    w.maximized = true
  }
  w.zIndex = ++zSeq
}

function toggleWindow(id) {
  const w = windows[id]
  if (w.minimized) {
    w.minimized = false
    w.zIndex = ++zSeq
  } else if (w.zIndex === topZ.value) {
    w.minimized = true
  } else {
    w.zIndex = ++zSeq
  }
}

function focusWindow(id) {
  const w = windows[id]
  w.zIndex = ++zSeq
  if (w.minimized) w.minimized = false
}

function winStyle(w) {
  if (w.maximized) return { zIndex: w.zIndex }
  return {
    left: `${w.x}px`,
    top: `${w.y}px`,
    width: `${w.w}px`,
    height: `${w.h}px`,
    zIndex: w.zIndex,
  }
}

const drag = ref(null)

function startDrag(id, e) {
  const w = windows[id]
  if (w.maximized || e.target.closest('.win-controls')) return
  drag.value = { id, t: 'move', sx: e.clientX, sy: e.clientY, lx: w.x, ly: w.y }
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', endDrag)
}

function startResize(id, e, dir) {
  const w = windows[id]
  if (w.maximized) return
  drag.value = { id, t: 'resize', dir, sx: e.clientX, sy: e.clientY, lx: w.x, ly: w.y, lw: w.w, lh: w.h }
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', endDrag)
}

function onDrag(e) {
  const d = drag.value
  if (!d) return
  const w = windows[d.id]
  if (!w) return
  const dx = e.clientX - d.sx
  const dy = e.clientY - d.sy
  if (d.t === 'move') {
    w.x = Math.max(0, d.lx + dx)
    w.y = Math.max(0, d.ly + dy)
  } else {
    let { lx, ly, lw, lh } = d
    let nx = lx, ny = ly, nw = lw, nh = lh
    if (d.dir.includes('e')) nw = Math.max(420, lw + dx)
    if (d.dir.includes('w')) { nw = Math.max(420, lw - dx); nx = lx + lw - nw }
    if (d.dir.includes('s')) nh = Math.max(300, lh + dy)
    if (d.dir.includes('n')) { nh = Math.max(300, lh - dy); ny = ly + lh - nh }
    w.x = nx; w.y = ny; w.w = nw; w.h = nh
  }
}

function endDrag() {
  drag.value = null
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', endDrag)
}

const timeStr = ref('')
const dateStr = ref('')
let timer = null

function updateClock() {
  const now = new Date()
  timeStr.value = now.toLocaleTimeString('en-US', { hour: 'numeric', minute: 'numeric', hour12: false })
  dateStr.value = now.toLocaleDateString('en-US', { year: '2-digit', month: '2-digit', day: 'numeric' })
}

function goHome() { router.push('/') }

onMounted(() => { updateClock(); timer = setInterval(updateClock, 1000) })

onUnmounted(() => {
  if (timer) clearInterval(timer)
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', endDrag)
})
</script>

<style>
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
}
</style>

<style scoped>
.desktop {
  position: relative;
  width: 100vw;
  height: 100vh;
  user-select: none;
  background:
    radial-gradient(ellipse 80% 60% at 30% 20%, rgba(59,130,246,0.15) 0%, transparent 60%),
    radial-gradient(ellipse 60% 50% at 70% 30%, rgba(139,92,246,0.12) 0%, transparent 60%),
    radial-gradient(ellipse 50% 40% at 50% 80%, rgba(59,130,246,0.08) 0%, transparent 50%),
    linear-gradient(160deg, #0b0e1a 0%, #111827 40%, #1e1b4b 70%, #0f172a 100%);
  overflow: hidden;
}

.desktopCont {
  position: absolute;
  top: 0; left: 0;
  width: 0;
  height: calc(100% - 48px);
  display: flex;
  flex-direction: column;
  flex-wrap: wrap;
  padding: 12px 0;
  align-content: flex-start;
  z-index: 1;
}

.dskApp {
  margin: 4px 12px;
  height: 84px;
  width: 74px;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-size: 0.8em;
  transition: all ease-in-out 200ms;
  justify-content: center;
  border: 1px solid transparent;
  cursor: pointer;
  outline: none;
}

.dskApp:focus {
  background: rgba(255,255,255,0.24);
  border: 1px dotted white;
}

.dskApp:hover {
  background: rgba(255,255,255,0.12);
}

.dskIcon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 48px;
  height: 48px;
  pointer-events: none;
}

.appName {
  text-align: center;
  color: #fafafa;
  margin-top: 4px;
  text-shadow: 0 0 4px rgba(0,0,0,0.6);
  font-size: 12px;
  line-height: 1.2;
  max-width: 74px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  pointer-events: none;
}

/* ---- Windows layer ---- */
.windows-layer {
  position: absolute;
  top: 0; left: 0;
  width: 100%;
  height: calc(100% - 48px);
  z-index: 2;
  pointer-events: none;
}

.win-window {
  position: absolute;
  display: flex;
  flex-direction: column;
  border-radius: 8px;
  overflow: hidden;
  background: #1e1e2e;
  box-shadow: 0 8px 32px rgba(0,0,0,0.45), 0 0 0 1px rgba(255,255,255,0.06);
  pointer-events: auto;
  transition: opacity 0.2s, transform 0.2s;
}

.win-window.win-minimized {
  transform: scale(0);
  opacity: 0;
  pointer-events: none;
}

.win-window.win-maximized {
  top: 0 !important;
  left: 0 !important;
  width: 100% !important;
  height: 100% !important;
  border-radius: 0;
}

.win-titlebar {
  display: flex;
  align-items: center;
  height: 32px;
  min-height: 32px;
  background: #2a2a3e;
  padding: 0 8px;
  cursor: default;
  flex-shrink: 0;
}

.win-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  margin-right: 6px;
}

.win-icon svg {
  width: 18px;
  height: 18px;
}

.win-title-text {
  flex: 1;
  font-size: 12px;
  color: #ccc;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.win-controls {
  display: flex;
  gap: 4px;
}

.ctrl-btn {
  width: 36px;
  height: 24px;
  border: none;
  background: transparent;
  color: #aaa;
  font-size: 12px;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  transition: background 0.15s;
}

.ctrl-btn:hover {
  background: rgba(255,255,255,0.1);
  color: #fff;
}

.ctrl-close:hover {
  background: #e81123;
  color: #fff;
}

.win-body {
  flex: 1;
  display: flex;
  background: #111;
  min-height: 0;
}

.win-body iframe {
  width: 100%;
  height: 100%;
  border: none;
}

/* Resize handles */
.rh {
  position: absolute;
  z-index: 10;
}
.rh-n { top: -4px; left: 0; right: 0; height: 8px; cursor: n-resize; }
.rh-s { bottom: -4px; left: 0; right: 0; height: 8px; cursor: s-resize; }
.rh-e { right: -4px; top: 0; bottom: 0; width: 8px; cursor: e-resize; }
.rh-w { left: -4px; top: 0; bottom: 0; width: 8px; cursor: w-resize; }
.rh-ne { top: -4px; right: -4px; width: 12px; height: 12px; cursor: ne-resize; }
.rh-nw { top: -4px; left: -4px; width: 12px; height: 12px; cursor: nw-resize; }
.rh-se { bottom: -4px; right: -4px; width: 12px; height: 12px; cursor: se-resize; }
.rh-sw { bottom: -4px; left: -4px; width: 12px; height: 12px; cursor: sw-resize; }

/* ---- Taskbar ---- */
.taskbar {
  position: absolute;
  width: 100vw;
  height: 48px;
  color: #fafafa;
  background: rgba(32,32,32,0.8);
  -webkit-backdrop-filter: saturate(3) blur(20px);
  backdrop-filter: saturate(3) blur(20px);
  bottom: 0;
  z-index: 10000;
}

.taskcont {
  position: relative;
  width: 100%;
  height: 100%;
}

.tasksCont {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.tsbar {
  width: auto;
  height: 100%;
  display: flex;
  align-items: center;
  transition: all ease-in-out 200ms;
}

.tsIcon {
  position: relative;
  width: 38px;
  height: 38px;
  margin: auto 3px;
  box-sizing: border-box;
  border-radius: 4px;
  background: transparent;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}

.tsIcon:hover {
  background: rgba(255,255,255,0.1);
}

.tsIcon.active::after {
  content: '';
  position: absolute;
  bottom: 2px;
  left: 50%;
  transform: translateX(-50%);
  width: 16px;
  height: 3px;
  border-radius: 2px;
  background: #60a5fa;
}

.startBtn {
  cursor: pointer;
}

.startBtn svg {
  color: #fafafa;
}

.winBtn {
  cursor: pointer;
}

.tb-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
}

.tb-icon svg {
  width: 20px;
  height: 20px;
}

.taskright {
  position: absolute;
  top: 0;
  right: 0;
  width: auto;
  height: 100%;
  margin-left: 10px;
  display: flex;
  align-items: center;
}

.taskDate {
  display: flex;
  padding: 0 8px;
  font-size: 11px;
  flex-direction: column;
  justify-content: center;
  cursor: pointer;
}

.taskDate div {
  width: 100%;
  text-align: center;
  font-weight: 400;
}

.graybd {
  border: solid 1px transparent;
  height: 1rem;
  width: 6px;
  margin: 0 4px;
}

.graybd:hover {
  border: solid 1px #a1a1a1;
  border-width: 0 0 0 2px;
}
</style>
