<template>
  <div class="fv-page" :class="`fv-${kind}`">
    <div class="fv-header">
      <span class="fv-icon">{{ icon }}</span>
      <span class="fv-name">{{ name }}</span>
    </div>
    <div class="fv-body">
      <div v-if="loading" class="fv-status">加载中...</div>
      <div v-else-if="error" class="fv-status fv-error">{{ error }}</div>
      <div v-else-if="kind === 'text'" class="fv-text"><pre>{{ text }}</pre></div>
      <div v-else-if="kind === 'markdown'" class="fv-markdown" v-html="html"></div>
      <img v-else-if="kind === 'image'" :src="fileUrl" class="fv-image" @click="openNewTab" />
      <video v-else-if="kind === 'video'" :src="fileUrl" controls autoplay class="fv-video"></video>
      <audio v-else-if="kind === 'audio'" :src="fileUrl" controls autoplay class="fv-audio"></audio>
      <iframe v-else-if="kind === 'pdf'" :src="fileUrl" class="fv-pdf"></iframe>
      <div v-else class="fv-unknown">
        <p>无法预览此文件</p>
        <a :href="fileUrl" target="_blank" class="fv-link">下载 / 在新标签页中打开</a>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { marked } from 'marked'

const route = useRoute()

const name = ref('')
const fileUrl = ref('')
const kind = ref('')
const icon = ref('📄')
const text = ref('')
const html = ref('')
const loading = ref(true)
const error = ref('')

function extIcon(n) {
  if (!n) return '📄'
  const ext = n.split('.').pop().toLowerCase()
  const map = {
    txt:'📝', md:'📝', csv:'📊', pdf:'📄',
    jpg:'🖼️', jpeg:'🖼️', png:'🖼️', gif:'🖼️', webp:'🖼️', svg:'🖼️',
    mp4:'🎬', avi:'🎬', mov:'🎬', mkv:'🎬',
    mp3:'🎵', wav:'🎵', flac:'🎵',
    html:'🌐', css:'🎨', js:'⚡', ts:'⚡',
    cpp:'⚙️', hpp:'⚙️', c:'⚙️', h:'⚙️',
    py:'🐍', java:'☕', rs:'🦀',
    deb:'📦', zip:'📦', tar:'📦', gz:'📦',
    docx:'📋', doc:'📋',
  }
  return map[ext] || '📄'
}

function openNewTab() {
  window.open(fileUrl.value, '_blank')
}

onMounted(async () => {
  name.value = route.query.name || ''
  fileUrl.value = route.query.url || ''
  kind.value = route.query.kind || 'unknown'
  icon.value = extIcon(name.value)

  if (kind.value === 'text' || kind.value === 'markdown') {
    try {
      const resp = await fetch(fileUrl.value)
      if (!resp.ok) throw new Error(resp.statusText)
      const raw = await resp.text()
      if (kind.value === 'markdown') {
        html.value = marked.parse(raw, { breaks: true, gfm: true })
      } else {
        text.value = raw
      }
    } catch (e) {
      error.value = '读取失败: ' + (e.message || '')
    }
  }
  loading.value = false
})
</script>

<style scoped>
.fv-page {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: #1e1e2e;
  color: #cdd6f4;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}
.fv-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 16px;
  background: #181825;
  border-bottom: 1px solid #313244;
  font-size: 14px;
  font-weight: 600;
}
.fv-icon { font-size: 20px; }
.fv-name { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.fv-body {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: auto;
  padding: 16px;
}
.fv-status { color: #6c7086; font-size: 14px; }
.fv-error { color: #f38ba8; }
.fv-text {
  width: 100%;
  height: 100%;
}
.fv-text pre {
  margin: 0;
  font-size: 13px;
  line-height: 1.5;
  white-space: pre-wrap;
  word-break: break-all;
  color: #cdd6f4;
}
.fv-markdown {
  width: 100%;
  height: 100%;
  overflow: auto;
  padding: 16px 24px;
  color: #cdd6f4;
  line-height: 1.7;
}
.fv-markdown h1, .fv-markdown h2, .fv-markdown h3,
.fv-markdown h4, .fv-markdown h5, .fv-markdown h6 {
  margin: 1em 0 0.5em;
  color: #f5f5f5;
}
.fv-markdown h1 { font-size: 1.6em; border-bottom: 1px solid #313244; padding-bottom: 0.3em; }
.fv-markdown h2 { font-size: 1.3em; border-bottom: 1px solid #313244; padding-bottom: 0.2em; }
.fv-markdown h3 { font-size: 1.1em; }
.fv-markdown p { margin: 0.6em 0; }
.fv-markdown a { color: #89b4fa; }
.fv-markdown code {
  background: #181825;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 0.9em;
}
.fv-markdown pre {
  background: #181825;
  padding: 12px 16px;
  border-radius: 8px;
  overflow: auto;
}
.fv-markdown pre code { background: none; padding: 0; }
.fv-markdown blockquote {
  border-left: 4px solid #89b4fa;
  margin: 0.6em 0;
  padding: 4px 16px;
  color: #a6adc8;
}
.fv-markdown ul, .fv-markdown ol { padding-left: 24px; }
.fv-markdown li { margin: 0.3em 0; }
.fv-markdown table {
  border-collapse: collapse;
  margin: 0.6em 0;
}
.fv-markdown th, .fv-markdown td {
  border: 1px solid #313244;
  padding: 6px 12px;
}
.fv-markdown th { background: #181825; }
.fv-markdown img { max-width: 100%; border-radius: 4px; }
.fv-markdown hr { border: none; border-top: 1px solid #313244; margin: 1em 0; }
.fv-image {
  max-width: 100%;
  max-height: calc(100vh - 60px);
  object-fit: contain;
  cursor: pointer;
  border-radius: 8px;
}
.fv-video {
  max-width: 100%;
  max-height: calc(100vh - 60px);
}
.fv-audio {
  width: 100%;
  background: #181825;
  border-radius: 8px;
}
.fv-audio .fv-body {
  padding: 0;
}
.fv-pdf .fv-body {
  padding: 0;
  position: relative;
  min-height: 0;
}
iframe.fv-pdf {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  border: none;
}
.fv-unknown {
  text-align: center;
  color: #6c7086;
  display: flex;
  flex-direction: column;
  gap: 12px;
  font-size: 14px;
}
.fv-link {
  color: #89b4fa;
  text-decoration: underline;
}
</style>
