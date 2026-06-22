<template>
  <div class="chat-page">
    <header class="chat-header">
      <a class="chat-back" href="#/" target="_self">← 返回</a>
      <h1>Chat 喵</h1>
      <span class="chat-status" :class="statusClass">{{ statusText }}</span>
    </header>
    <main class="chat-main" ref="msgBox">
      <div
        v-for="(m, i) in messages"
        :key="i"
        :class="['msg', m.isSelf ? 'msg-self' : 'msg-other']"
      >
        <div v-if="m.type === 'embed'" class="bubble bubble-embed" :class="'bubble-'+m.kind">
          <div v-if="m.kind === 'image'" class="embed-body">
            <img :src="m.url" :alt="m.title" class="embed-img" @click="previewImg(m.url)" />
            <div v-if="m.title" class="embed-title">{{ m.title }}</div>
          </div>
          <div v-else-if="m.kind === 'video'" class="embed-body">
            <video :src="m.url" controls class="embed-video"></video>
            <div v-if="m.title" class="embed-title">{{ m.title }}</div>
          </div>
          <div v-else-if="m.kind === 'audio'" class="embed-body">
            <div class="audio-label">{{ m.title || '音频' }}</div>
            <audio :src="m.url" controls class="embed-audio"></audio>
          </div>
          <div v-else-if="m.kind === 'game'" class="embed-body embed-game">
            <div class="game-icon">🎮</div>
            <div class="game-info">
              <div class="game-name">{{ m.name }}</div>
              <a @click.prevent="$router.push(m.url)" class="game-link" href>开始游戏</a>
            </div>
          </div>
          <div v-else class="embed-body">
            {{ m.text }}
          </div>
        </div>
        <div v-else class="bubble" v-html="renderMarkdown(m.text)"></div>
      </div>
    </main>
    <footer class="chat-footer">
      <input
        v-model="input"
        class="chat-input"
        placeholder="输入消息..."
        @keydown.enter="send"
        :disabled="!connected"
      />
      <button class="chat-send" @click="send" :disabled="!connected || !input.trim()">发送</button>
    </footer>
  </div>
</template>

<script setup>
import { ref, computed, nextTick, onBeforeUnmount } from 'vue'
import { marked } from 'marked'

const WS_URL = `ws://${location.hostname}:3001/chat`

const input = ref('')
const messages = ref([])
const connected = ref(false)
const ws = ref(null)
let reconnectTimer = null
const streamingIdx = ref(-1)

const statusText = computed(() => connected.value ? '已连接' : '未连接')
const statusClass = computed(() => connected.value ? 'status-ok' : 'status-err')

const msgBox = ref(null)

function scrollBottom() {
  nextTick(() => {
    const el = msgBox.value
    if (el) el.scrollTop = el.scrollHeight
  })
}

function renderMarkdown(text) {
  if (!text) return ''
  return marked.parse(text)
}

function previewImg(url) {
  window.open(url, '_blank')
}

function send() {
  const text = input.value.trim()
  if (!text || !connected.value || !ws.value) {
    console.log('send blocked:', { text, connected: connected.value, hasWs: !!ws.value, readyState: ws.value?.readyState })
    return
  }
  if (ws.value.readyState !== WebSocket.OPEN) {
    console.log('send blocked: ws not open, state:', ws.value.readyState)
    return
  }
  ws.value.send(JSON.stringify({ text }))
  messages.value.push({ text, isSelf: true })
  input.value = ''
  scrollBottom()
}

function connect() {
  const sock = new WebSocket(WS_URL)
  ws.value = sock

  sock.onopen = () => {
    console.log('ws connected')
    connected.value = true
  }

  sock.onmessage = (e) => {
    try {
      const data = JSON.parse(e.data)
      if (data.type === 'embed') {
        messages.value.push({ isSelf: false, type: 'embed', kind: data.kind, url: data.url, title: data.title, name: data.name, text: data.text || '' })
        scrollBottom()
      } else if (data.type === 'stream_start') {
        messages.value.push({ text: '', isSelf: false })
        streamingIdx.value = messages.value.length - 1
        scrollBottom()
      } else if (data.type === 'delta') {
        if (streamingIdx.value >= 0) {
          messages.value[streamingIdx.value].text += data.text
          scrollBottom()
        }
      } else if (data.type === 'stream_end') {
        if (data.msg && streamingIdx.value >= 0) {
          messages.value[streamingIdx.value].text = '⚠️ ' + data.msg
        }
        streamingIdx.value = -1
        scrollBottom()
      } else if (data.text !== undefined) {
        messages.value.push({ text: data.text, isSelf: false })
        scrollBottom()
      }
    } catch (err) {
      console.error('ws message parse error:', err)
    }
  }

  sock.onerror = () => {
    connected.value = false
  }

  sock.onclose = () => {
    connected.value = false
    ws.value = null
    reconnectTimer = setTimeout(connect, 3000)
  }
}

connect()

onBeforeUnmount(() => {
  if (reconnectTimer) clearTimeout(reconnectTimer)
  if (ws.value) ws.value.close()
})
</script>

<style scoped>
.chat-page {
  display: flex;
  flex-direction: column;
  height: 100vh;
  max-width: 600px;
  margin: 0 auto;
  background: #f5f7fb;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans SC', sans-serif;
}

.chat-header {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px 20px;
  background: #fff;
  border-bottom: 1px solid #e5e7eb;
  flex-shrink: 0;
}

.chat-back {
  color: #2563eb;
  text-decoration: none;
  font-size: 14px;
  flex-shrink: 0;
}

.chat-back:hover {
  text-decoration: underline;
}

.chat-header h1 {
  font-size: 18px;
  font-weight: 600;
  flex: 1;
  margin: 0;
}

.chat-status {
  font-size: 12px;
  padding: 2px 10px;
  border-radius: 10px;
  flex-shrink: 0;
}

.status-ok {
  background: #d1fae5;
  color: #065f46;
}

.status-err {
  background: #fee2e2;
  color: #991b1b;
}

.chat-main {
  flex: 1;
  overflow-y: auto;
  padding: 16px 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.msg {
  display: flex;
}

.msg-self {
  justify-content: flex-end;
}

.msg-other {
  justify-content: flex-start;
}

.bubble {
  max-width: 70%;
  padding: 10px 16px;
  border-radius: 18px;
  font-size: 15px;
  line-height: 1.5;
  word-break: break-word;
}

.msg-self .bubble {
  background: #2563eb;
  color: #fff;
  border-bottom-right-radius: 4px;
}

.msg-other .bubble {
  background: #e5e7eb;
  color: #1f2937;
  border-bottom-left-radius: 4px;
}

.chat-footer {
  display: flex;
  gap: 8px;
  padding: 12px 20px;
  background: #fff;
  border-top: 1px solid #e5e7eb;
  flex-shrink: 0;
}

.chat-input {
  flex: 1;
  padding: 10px 14px;
  border: 1px solid #d1d5db;
  border-radius: 24px;
  font-size: 15px;
  outline: none;
  transition: border-color 0.2s;
}

.chat-input:focus {
  border-color: #2563eb;
}

.chat-input:disabled {
  background: #f3f4f6;
}

.chat-send {
  padding: 10px 20px;
  background: #2563eb;
  color: #fff;
  border: none;
  border-radius: 24px;
  font-size: 14px;
  cursor: pointer;
  transition: background 0.2s;
  flex-shrink: 0;
}

.chat-send:hover:not(:disabled) {
  background: #1d4ed8;
}

.chat-send:disabled {
  opacity: 0.4;
  cursor: default;
}

/* ---- Embed bubbles ---- */

.bubble-embed {
  max-width: 85%;
  padding: 8px;
}

.bubble-image,
.bubble-video,
.bubble-audio,
.bubble-game {
  background: #fff;
  border: 1px solid #e5e7eb;
  border-bottom-left-radius: 4px;
}

.msg-self .bubble-image,
.msg-self .bubble-video,
.msg-self .bubble-audio,
.msg-self .bubble-game {
  background: #eff6ff;
  border-color: #bfdbfe;
  border-bottom-left-radius: 18px;
  border-bottom-right-radius: 4px;
}

.embed-body {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.embed-img {
  max-width: 100%;
  max-height: 300px;
  border-radius: 10px;
  cursor: pointer;
  object-fit: contain;
  background: #f0f0f0;
}

.embed-video {
  max-width: 100%;
  max-height: 360px;
  border-radius: 10px;
  background: #000;
}

.embed-audio {
  width: 100%;
  height: 40px;
}

.embed-title {
  font-size: 12px;
  color: #6b7280;
  padding: 0 4px;
}

.audio-label {
  font-size: 14px;
  font-weight: 600;
  color: #374151;
  padding: 4px 0;
}

/* Game embed */
.embed-game {
  flex-direction: row;
  align-items: center;
  gap: 12px;
  padding: 8px 4px;
}

.game-icon {
  font-size: 32px;
  flex-shrink: 0;
}

.game-info {
  display: flex;
  flex-direction: column;
  gap: 6px;
  flex: 1;
}

.game-name {
  font-size: 15px;
  font-weight: 600;
  color: #1f2937;
}

.game-link {
  display: inline-block;
  padding: 6px 16px;
  background: #2563eb;
  color: #fff;
  border-radius: 20px;
  text-decoration: none;
  font-size: 13px;
  text-align: center;
  transition: background 0.2s;
}

.game-link:hover {
  background: #1d4ed8;
}

/* Markdown text styling */
.bubble :deep(p) {
  margin: 0 0 8px;
}
.bubble :deep(p:last-child) {
  margin-bottom: 0;
}
.bubble :deep(code) {
  background: rgba(0,0,0,0.07);
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 13px;
}
.bubble :deep(pre) {
  background: rgba(0,0,0,0.07);
  padding: 10px 14px;
  border-radius: 8px;
  overflow-x: auto;
  font-size: 13px;
  margin: 8px 0;
}
.bubble :deep(pre code) {
  background: none;
  padding: 0;
}
.bubble :deep(a) {
  color: inherit;
  text-decoration: underline;
}
.bubble :deep(ul),
.bubble :deep(ol) {
  padding-left: 20px;
  margin: 4px 0;
}
.bubble :deep(blockquote) {
  border-left: 3px solid #9ca3af;
  margin: 8px 0;
  padding: 4px 12px;
  color: #6b7280;
}
.bubble :deep(h1),
.bubble :deep(h2),
.bubble :deep(h3),
.bubble :deep(h4) {
  margin: 8px 0 4px;
  font-size: inherit;
  font-weight: 600;
}
.bubble :deep(img) {
  max-width: 100%;
  border-radius: 8px;
  margin: 4px 0;
}
</style>
