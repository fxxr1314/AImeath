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
import { ref, onMounted, nextTick } from 'vue'
import { useRoute } from 'vue-router'
import { marked } from 'marked'
import mermaid from 'mermaid'

mermaid.initialize({
  startOnLoad: false,
  theme: 'dark',
  fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
  securityLevel: 'loose',
})

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

function wrapMermaid(el) {
  const source = el.getAttribute('data-source') || ''
  const wrapper = document.createElement('div')
  wrapper.className = 'mermaid-wrapper'
  el.before(wrapper)
  wrapper.appendChild(el)
  const src = document.createElement('pre')
  src.className = 'mermaid-source'
  src.textContent = source
  wrapper.appendChild(src)
  const btn = document.createElement('button')
  btn.className = 'mermaid-toggle'
  btn.textContent = '◇'
  btn.addEventListener('click', (e) => {
    e.stopPropagation()
    wrapper.classList.toggle('show-source')
  })
  wrapper.appendChild(btn)
  const svg = el.querySelector('svg')
  if (svg) {
    svg.style.cursor = 'pointer'
    svg.addEventListener('click', (e) => {
      e.stopPropagation()
      const blob = new Blob([svg.outerHTML], { type: 'image/svg+xml' })
      const url = URL.createObjectURL(blob)
      window.open(url, '_blank')
      setTimeout(() => URL.revokeObjectURL(url), 10000)
    })
  }
}

async function renderMermaid() {
  const container = document.querySelector('.fv-markdown')
  if (!container) return
  const blocks = container.querySelectorAll('pre > code.language-mermaid')
  if (!blocks.length) return
  blocks.forEach((el) => {
    const pre = el.parentElement
    if (!pre) return
    pre.classList.add('mermaid')
    pre.setAttribute('data-source', el.textContent || '')
    pre.innerHTML = el.textContent || ''
    el.remove()
  })
  try {
    const nodes = Array.from(container.querySelectorAll('.mermaid'))
    await mermaid.run({ nodes })
    nodes.forEach(wrapMermaid)
  } catch (e) {
    console.error('mermaid render error:', e)
  }
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
  if (kind.value === 'markdown') {
    await nextTick()
    await renderMermaid()
  }
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
.fv-markdown :deep(.mermaid-wrapper) {
  position: relative;
}
.fv-markdown :deep(.mermaid-toggle) {
  position: absolute;
  top: 4px;
  left: 4px;
  z-index: 1;
  background: rgba(255,255,255,0.12);
  border: 1px solid rgba(255,255,255,0.2);
  color: #cdd6f4;
  border-radius: 4px;
  padding: 2px 8px;
  cursor: pointer;
  font-size: 12px;
  line-height: 1.4;
  opacity: 0.5;
  transition: opacity 0.2s;
}
.fv-markdown :deep(.mermaid-wrapper:hover .mermaid-toggle) {
  opacity: 1;
}
.fv-markdown :deep(.mermaid-source) {
  display: none;
  margin: 0;
  padding: 12px 16px;
  background: #181825;
  border-radius: 8px;
  font-size: 13px;
  line-height: 1.5;
  overflow-x: auto;
  white-space: pre;
}
.fv-markdown :deep(.mermaid-wrapper.show-source .mermaid-source) {
  display: block;
}
.fv-markdown :deep(.mermaid-wrapper.show-source .mermaid > svg) {
  display: none;
}
</style>
